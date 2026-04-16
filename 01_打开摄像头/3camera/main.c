#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <unistd.h>

/*
 * 【修改点 1：错误逻辑修正】
 * ---------------------------------------------------------
 * 之前写法：if (ret < 0) printf("失败"); printf("成功");
 * 错误原因：逻辑不完整，即便失败了也会打印“成功”，导致无法正确排查。
 * 正确写法：失败时打印系统错误原因 (perror) 并 exit
 * 停止程序，确保不再执行错误逻辑。
 * ---------------------------------------------------------
 */
static void prerr(int video_ret, char *buf) {
    if (video_ret < 0) {
        perror(buf);
        exit(-1);
    }
    printf("%s 成功\n", buf);
}

#define VIDEO_PATH   "/dev/video2"
#define VIDEO_WIDTH  800
#define VIDEO_HEIGHT 1280

struct v4l2_format         video_fmt;
struct v4l2_capability     video_cap;
struct v4l2_requestbuffers video_req;
struct v4l2_buffer         video_buf;
enum v4l2_buf_type         video_buf_type;

int main() {
    int            video_fd;
    int            video_ret;
    int            file_save_fd;
    unsigned char *fb_mem;

    // 1.打开摄像头
    video_fd = open(VIDEO_PATH, O_RDWR);
    if (video_fd < 0) {
        printf("摄像头打开失败 \n");
        return -1;
    }

    // 2. capacity查询
    video_ret = ioctl(video_fd, VIDIOC_QUERYCAP, &video_cap);
    prerr(video_ret, "cap查询");
    printf("摄像头名称为 %s \n", video_cap.card);

    // 3. fmt查询
    memset(&video_fmt, 0, sizeof(video_fmt));
    /*
     * 【修改点 2：查询条件缺失】
     * ---------------------------------------------------------
     * 之前写法：memset后直接 ioctl(VIDIOC_G_FMT)
     * 错误原因：未给 video_fmt.type
     * 赋值，内核不知道你想查哪种格式，会导致宽高返回 0*0。 正确写法：在 ioctl
     * 之前，必须设置 type = V4L2_BUF_TYPE_VIDEO_CAPTURE。
     * ---------------------------------------------------------
     */
    video_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_ret = ioctl(video_fd, VIDIOC_G_FMT, &video_fmt);
    prerr(video_ret, "参数查询");
    printf("摄像头的宽高为 %d * %d \n", video_fmt.fmt.pix.width, video_fmt.fmt.pix.height);

    // 4.申请缓冲区
    memset(&video_req, 0, sizeof(video_req));
    video_req.count = 2;
    video_req.memory = V4L2_MEMORY_MMAP;
    video_req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_ret = ioctl(video_fd, VIDIOC_REQBUFS, &video_req);
    prerr(video_ret, "缓冲区申请");

    // 5. 缓冲区定义（查询具体一个缓冲区的信息）
    memset(&video_buf, 0, sizeof(video_buf));
    video_buf.memory = V4L2_MEMORY_MMAP;
    video_buf.index = 0;
    video_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_ret = ioctl(video_fd, VIDIOC_QUERYBUF, &video_buf);
    prerr(video_ret, "缓冲区定义");

    // 6.mmap 映射内存
    fb_mem =
        (unsigned char *)mmap(NULL, video_buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, video_buf.m.offset);
    if (fb_mem == MAP_FAILED) {
        printf("mmap失败 \n");
        return -1;
    }
    printf("mmap成功 \n");

    // 7. 入队
    /*
     * 【修改点 3：指令用法错误（最关键）】
     * ---------------------------------------------------------
     * 之前写法：ioctl(video_fd, VIDIOC_QUERYBUF, &video_buf);
     * 错误原因：你误用了 QUERYBUF（询问状态），实际上需要的是 QBUF（入队）。
     * 正确写法：使用 VIDIOC_QBUF
     * 将缓冲区真正提交给内核，开始等待摄像头填入数据。
     * ---------------------------------------------------------
     */
    video_ret = ioctl(video_fd, VIDIOC_QBUF, &video_buf);
    prerr(video_ret, "入队");

    // 8. 流开启
    video_buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_ret = ioctl(video_fd, VIDIOC_STREAMON, &video_buf_type);
    prerr(video_ret, "流开启");

    // 9. 出队（从队列取出填满数据的缓冲区）
    video_ret = ioctl(video_fd, VIDIOC_DQBUF, &video_buf);
    prerr(video_ret, "出队");

    // 10. 保存画面
    file_save_fd = open("test.raw", O_RDWR | O_CREAT, 0666);
    if (file_save_fd < 0) {
        printf("文件创建失败\n");
        return -1;
    }
    // 使用实际获取的字节数 video_buf.bytesused 写入
    write(file_save_fd, fb_mem, video_buf.bytesused);
    printf("test.raw 保存成功\n");
    close(file_save_fd);

    // 11. 关闭流
    /*
     * 【修改点 4：接口参数类型错误】
     * ---------------------------------------------------------
     * 之前写法：ioctl(video_fd, VIDIOC_STREAMOFF, &video_buf);
     * 错误原因：STREAMOFF
     * 需要的是缓冲区类型的指针，而不是具体的缓冲区结构体指针。 正确写法：传入
     * &video_buf_type。
     * ---------------------------------------------------------
     */
    video_ret = ioctl(video_fd, VIDIOC_STREAMOFF, &video_buf_type);
    prerr(video_ret, "流关闭");

    // 12. unmap
    munmap(fb_mem, video_buf.length);

    close(video_fd);

    return 0;
}

/*
 * 【进阶演示：如何使用循环处理多个缓冲区 (video_req.count > 1)】
 * ---------------------------------------------------------
 * 如果你申请了多个缓冲区（比如 video_req.count = 2），
 * 建议使用数组和循环来管理，这样可以提高采集的连续性和稳定性。
 *
 * 示例逻辑如下：
 *
 * // 1. 定义一个结构体来保存每个缓冲区的信息
 * struct buffer {
 *     void   *start;
 *     size_t  length;
 * } *buffers;
 *
 * // 2. 根据实际申请到的数量分配内存
 * buffers = calloc(video_req.count, sizeof(*buffers));
 *
 * // 3. 循环映射并入队
 * for (int i = 0; i < video_req.count; ++i) {
 *     struct v4l2_buffer buf;
 *     memset(&buf, 0, sizeof(buf));
 *     buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 *     buf.memory = V4L2_MEMORY_MMAP;
 *     buf.index  = i;
 *
 *     // 查询每个缓冲区的信息
 *     ioctl(video_fd, VIDIOC_QUERYBUF, &buf);
 *
 *     // 为每个缓冲区进行 mmap
 *     buffers[i].length = buf.length;
 *     buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
 *                             MAP_SHARED, video_fd, buf.m.offset);
 *
 *     // 将所有缓冲区全部放入内核队列
 *     ioctl(video_fd, VIDIOC_QBUF, &buf);
 * }
 *
 * // 4. 开启流后，内核会自动在这些缓冲区之间轮换填充数据
 * // 5. 退出时别忘了循环 munmap
 * ---------------------------------------------------------
 */
