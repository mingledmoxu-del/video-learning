#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define VIDEO_WIDTH 1280
#define VIDEO_HEIGHT 800
#define VIDEO_PATH "/dev/video0"

int main() {
    // --- 1. 环境准备 ---
    int video_fd = open(VIDEO_PATH, O_RDWR); // 打开摄像头

    struct v4l2_format         video_fmt;
    struct v4l2_requestbuffers video_req;
    struct v4l2_buffer         video_buf;

    // --- 2. 格式设置 ---
    memset(&video_fmt, 0, sizeof(video_fmt));
    video_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_fmt.fmt.pix.width = VIDEO_WIDTH;
    video_fmt.fmt.pix.height = VIDEO_HEIGHT;
    video_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    video_fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    ioctl(video_fd, VIDIOC_S_FMT, &video_fmt); // 告知驱动我们需要这种格式

    // --- 3. 申请并查询内核对应的缓冲区 ---
    memset(&video_req, 0, sizeof(video_req));
    video_req.count = 1;
    video_req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_req.memory = V4L2_MEMORY_MMAP;
    ioctl(video_fd, VIDIOC_REQBUFS, &video_req); // 申请 1 个视频帧缓冲区

    memset(&video_buf, 0, sizeof(video_buf));
    video_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_buf.memory = V4L2_MEMORY_MMAP;
    video_buf.index = 0;
    ioctl(video_fd, VIDIOC_QUERYBUF,
          &video_buf); // 查询申请到的缓冲区具体信息（长度、地址等）

    // --- 4. 内存映射 (mmap) ---
    // fb_addr 是我们在 C
    // 语言程序里可以直接访问的指针，它指向量刚才内核申请的那块物理内存
    unsigned char *fb_addr =
        (unsigned char *)mmap(NULL, video_buf.length, PROT_READ | PROT_WRITE,
                              MAP_SHARED, video_fd, video_buf.m.offset);

    // --- 5. 开始运转数据流 ---
    ioctl(video_fd, VIDIOC_QBUF,
          &video_buf); // 把空的缓冲区丢给内核（排队等待装载数据）

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(video_fd, VIDIOC_STREAMON, &type); // 开启摄像头数据流，传送带开始工作

    // --- 6. 捕捉一帧图像 ---
    // 程序会在这里等，直到驱动程序把第一帧画面“填满”缓冲区并送到终端。
    ioctl(video_fd, VIDIOC_DQBUF,
          &video_buf); // 从内核队列中取回那个“装满画面”的缓冲区

    // --- 7. 保存到本地文件 ---
    printf("抓取完成，数据大小: %u 字节\n", video_buf.bytesused);
    FILE *fp = fopen("capture.raw", "wb");
    if (fp) {
        fwrite(fb_addr, 1, video_buf.bytesused,
               fp); // 将内存中的这一帧像素直接写进硬盘
        fclose(fp);
    }

    // --- 8. 收尾 ---
    ioctl(video_fd, VIDIOC_STREAMOFF, &type); // 停止传送带
    munmap(fb_addr, video_buf.length);        // 解除内存映射
    close(video_fd);                          // 关闭设备

    printf("程序运行成功，已捕捉一帧图像至 capture.raw。\n");
    return 0;
}