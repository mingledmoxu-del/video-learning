#include <stdio.h>      // 标准输入输出头文件，提供 printf, perror 等函数
#include <stdlib.h>     // 标准库头文件，提供 exit, malloc, free 等函数
#include <string.h>     // 字符串操作头文件，提供 memset 等函数
#include <fcntl.h>      // 文件控制头文件，提供 open() 函数及 O_RDWR 等宏
#include <unistd.h>     // UNIX 标准头文件，提供 close(), read(), write() 等函数
#include <sys/ioctl.h>  // I/O 控制头文件，提供 ioctl() 函数用于与驱动交互
#include <sys/mman.h>   // 内存映射头文件，提供 mmap(), munmap() 函数
#include <linux/videodev2.h> // V4L2 核心头文件，定义了视频采集接口的所有结构体和宏
#include <errno.h>      // 错误码头文件，用于获取系统调用的错误信息

/**
 * 嵌入式 Linux V4L2 摄像头采集示例 (单帧抓取)
 * V4L2 (Video for Linux Two) 是 Linux 内核中关于视频设备的内核驱动标准。
 * 流程：打开设备 -> 查询能力 -> 设置格式 -> 申请缓冲区 -> 内存映射 -> 缓冲区入队 -> 启动流 -> 采集数据 -> 停止流 -> 释放资源
 */

#define WIDTH  640      // 定义期望采集的视频宽度
#define HEIGHT 480      // 定义期望采集的视频高度
#define VIDEO_DEV "/dev/video0" // 指定摄像头设备节点路径

int main() {
    int fd;             // 文件描述符，用于索引打开的摄像头设备

    /**
     * 【结构体：v4l2_capability】
     * 作用：查询设备的能力和信息。
     * 来源：<linux/videodev2.h>
     * 关键成员：
     *   - driver: 驱动程序的名称。
     *   - card: 设备的名称（通常是硬件型号）。
     *   - bus_info: 总线信息。
     *   - device_caps: 设备具体支持的能力标志位（如 V4L2_CAP_VIDEO_CAPTURE 为支持采集）。
     */
    struct v4l2_capability cap;

    /**
     * 【结构体：v4l2_format】
     * 作用：设置或获取视频的像素格式、分辨率等。
     * 来源：<linux/videodev2.h>
     * 关键成员：
     *   - type: 缓冲流类型，通常为 V4L2_BUF_TYPE_VIDEO_CAPTURE。
     *   - fmt.pix: 包含像素格式信息。
     *     - width/height: 分辨率。
     *     - pixelformat: 像素格式（如 YUYV, MJPEG, RGB）。
     *     - field: 扫描模式（逐行、隔行）。
     */
    struct v4l2_format fmt;

    /**
     * 【结构体：v4l2_requestbuffers】
     * 作用：向驱动程序请求缓冲区空间。
     * 来源：<linux/videodev2.h>
     * 关键成员：
     *   - count: 请求的缓冲区数量。
     *   - type: 流类型。
     *   - memory: 内存访问方式，V4L2_MEMORY_MMAP 为内存映射方式。
     */
    struct v4l2_requestbuffers req;

    /**
     * 【结构体：v4l2_buffer】
     * 作用：代表内核中的一个缓冲区实例，描述其属性和状态。
     * 来源：<linux/videodev2.h>
     * 关键成员：
     *   - index: 缓冲区的索引编号（0 ~ count-1）。
     *   - type: 流类型。
     *   - memory: 内存访问方式。
     *   - length: 缓冲区大小。
     *   - m.offset: 缓冲区在设备内存中的偏移（用于 mmap）。
     *   - bytesused: 已使用的数据长度（采集后有效）。
     */
    struct v4l2_buffer buf;

    void *buffer_start; // 指向用户空间映射后的缓冲区起始地址
    enum v4l2_buf_type type; // 定义流类型变量，用于启动/关闭流

    // 1. 打开摄像头设备
    // 以可读写、阻塞模式打开视频设备
    fd = open(VIDEO_DEV, O_RDWR);
    if (fd < 0) {
        // 如果打开失败，打印错误原因
        perror("打开摄像头失败，请检查设备是否存在或是否有权限(sudo)");
        return -1;
    }

    // 2. 查询设备能力
    // VIDIOC_QUERYCAP 指令用于获取设备信息，结果存入 cap 结构体中
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        perror("查询设备能力失败");
        close(fd); // 出错后关闭文件描述符
        return -1;
    }
    // 打印驱动名称，确认是否正确识别
    printf("驱动名称: %s\n", cap.driver);

    // 3. 设置视频格式
    // 将 fmt 结构体清零，防止脏数据干扰
    memset(&fmt, 0, sizeof(fmt));
    // 设置为视频采集模式
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // 设置期望的宽度
    fmt.fmt.pix.width = WIDTH;
    // 设置期望的高度
    fmt.fmt.pix.height = HEIGHT;
    // 设置像素格式为 YUYV格式 (每4个字节代表2个像素)
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; 
    // 设置场扫描方式为隔行扫描（取决于摄像头支持，通常设为 V4L2_FIELD_ANY 或具体值）
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    // VIDIOC_S_FMT 指令设置格式，驱动可能根据硬件调整实际参数并回填到 fmt 中
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("设置格式失败");
        close(fd);
        return -1;
    }
    // 打印最终生效的分辨率
    printf("当前设置分辨率: %d x %d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);

    // 4. 申请缓冲区 (Request Buffers)
    // 清零结构体
    memset(&req, 0, sizeof(req));
    // 申请 1 个缓冲区
    req.count = 1;
    // 流类型：视频采集
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // 访问方式：内存映射 (mmap)
    req.memory = V4L2_MEMORY_MMAP;

    // VIDIOC_REQBUFS 指令请求驱动在内核空间预留内存
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("申请缓冲区失败");
        close(fd);
        return -1;
    }

    // 5. 查询缓冲区信息并建立内存映射 (mmap)
    // 清零结构体
    memset(&buf, 0, sizeof(buf));
    // 类型需与前文一致
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // 模式需与前文一致
    buf.memory = V4L2_MEMORY_MMAP;
    // 指明要操作第 0 个缓冲区
    buf.index = 0;

    // VIDIOC_QUERYBUF 指令查询该缓冲区的具体信息（如长度、偏移量）
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        perror("查询缓冲区信息失败");
        close(fd);
        return -1;
    }

    /**
     * mmap 将内核空间中的设备缓冲区映射到用户进程地址空间
     * 参数说明：
     *   - NULL: 让内核自动选择映射起始地址
     *   - buf.length: 映射长度（来自 VIDIOC_QUERYBUF）
     *   - PROT_READ | PROT_WRITE: 读写权限
     *   - MAP_SHARED: 多个进程共享此映射，对内存的修改会同步到内核
     *   - fd: 摄像头设备描述符
     *   - buf.m.offset: 偏移量（来自 VIDIOC_QUERYBUF）
     */
    buffer_start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer_start == MAP_FAILED) {
        perror("mmap 失败");
        close(fd);
        return -1;
    }

    // 6. 缓冲区入队 (Queue Buffer)
    // VIDIOC_QBUF 指令将缓冲区放入驱动程序的空闲队列中，等待填充图像数据
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        perror("入队失败");
        munmap(buffer_start, buf.length); // 出错需释放映射
        close(fd);
        return -1;
    }

    // 7. 开启视频流
    // 设置操作类型为采集流
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // VIDIOC_STREAMON 执行启动流操作，硬件开始向缓冲区填充数据
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("开启视频流失败");
        munmap(buffer_start, buf.length);
        close(fd);
        return -1;
    }

    printf("采集开始，等待数据...\n");

    // 8. 取出缓冲区 (Dequeue Buffer)
    // VIDIOC_DQBUF 指令从“已填满”队列中取出一个缓冲区
    // 默认情况下此调用会阻塞，直到驱动把数据采集完成并填入缓冲区
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        perror("获取图像数据失败");
    } else {
        // 9. 保存数据到本地文件
        // 以二进制写入模式创建 output.yuv
        FILE *fp = fopen("output.yuv", "wb");
        if (fp) {
            // buf.bytesused 记录了本次采集实际产生的数据量
            // 将映射地址 buffer_start 处的数据写入文件
            fwrite(buffer_start, buf.bytesused, 1, fp);
            // 关闭文件
            fclose(fp);
            printf("成功抓取一帧！保存到: output.yuv (大小: %d 字节)\n", buf.bytesused);
        } else {
            perror("创建文件失败");
        }
    }

    // 10. 收尾清理
    // 停止视频流
    ioctl(fd, VIDIOC_STREAMOFF, &type);
    // 接触内存映射
    munmap(buffer_start, buf.length);
    // 关闭设备文件
    close(fd);

    printf("程序退出。\n");
    return 0; // 返回成功
}

