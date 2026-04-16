#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/unistd.h>
#include <unistd.h>

static int prerr(int ret, char *buf) {
    if (ret < 0) {
        printf("%s失败 \n", buf);
        return -1;
    }
    printf("%s成功 \n", buf);
}

#define VIDEO_PATH   "/dev/video0"
#define VIDEO_WIDTH  1280
#define VIDEO_HEIGHT 800

// struct v4l2_fmtdesc video_fmt;
struct v4l2_format         video_fmt;
struct v4l2_capability     video_cap;
struct v4l2_requestbuffers video_req;
struct v4l2_buffer         video_buf;
enum v4l2_buf_type         video_buf_type;

int main() {
    int video_fd;
    int video_ret;
    int file_save_fd;

    video_fd = open(VIDEO_PATH, O_RDWR);
    if (video_fd < 0) {
        printf("摄像头打开失败\n");
        return -1;
    }
    printf("摄像头打开成功\n");

    video_ret = ioctl(video_fd, VIDIOC_QUERYCAP, &video_cap);
    prerr(video_ret, "摄像头capacity查询");
    printf("摄像头的名称为 %s \n", video_cap.card);
    printf("摄像头的设备号为 %s \n", video_cap.bus_info);

    //   memset(&video_fmt, 0, sizeof(video_fmt));
    video_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_ret = ioctl(video_fd, VIDIOC_G_FMT, &video_fmt);
    prerr(video_ret, "像素格式查询");
    printf("摄像头像素的宽为%d \n", video_fmt.fmt.pix.width);
    printf("摄像头像素的高为%d \n", video_fmt.fmt.pix.height);
    // printf("摄像头像素的格式为%s \n", (char *)video_fmt.fmt.pix.pixelformat);

    memset(&video_req, 0, sizeof(video_req));
    // video_req.capabilities = 1;
    video_req.memory = V4L2_MEMORY_MMAP;
    video_req.count = 1;
    video_req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_ret = ioctl(video_fd, VIDIOC_REQBUFS, &video_req);
    prerr(video_ret, "申请缓冲区");
    //   printf("缓冲区的容量为%d \n", video_req.capabilities);
    printf("缓冲区的数量为%d \n", video_req.count);

    memset(&video_buf, 0, sizeof(video_buf));
    video_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_buf.memory = V4L2_MEMORY_MMAP;
    video_buf.index = 0;
    video_ret = ioctl(video_fd, VIDIOC_QUERYBUF, &video_buf);
    prerr(video_ret, "缓冲区查询");
    printf("缓冲区长度为 %d, 偏移量为 %d \n", video_buf.length, video_buf.m.offset);

    unsigned char *fb_mem;
    fb_mem = mmap(NULL, video_buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, video_buf.m.offset);
    if (fb_mem == MAP_FAILED) {
        printf("map失败\n");
        return -1;
    }
    printf("mmap成功\n");

    video_ret = ioctl(video_fd, VIDIOC_QBUF, &video_buf);
    prerr(video_ret, "入队");

    video_buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_ret = ioctl(video_fd, VIDIOC_STREAMON, &video_buf_type);
    prerr(video_ret, "流开启");

    video_ret = ioctl(video_fd, VIDIOC_DQBUF, &video_buf);
    prerr(video_ret, "出队");

    printf("字节大小为%d\n", video_buf.bytesused);

    file_save_fd = open("test.jpg", O_RDWR | O_CREAT, 0666);
    if (file_save_fd < 0) {
        printf("打开失败\n");
        return -1;
    } else {
        write(file_save_fd, fb_mem, video_buf.bytesused);
        printf("保存成功！文件名为 test.jpg \n");
    }
    close(file_save_fd);

    video_ret = ioctl(video_fd, VIDIOC_STREAMOFF, &video_buf);
    prerr(video_ret, "流关闭");

    /*------------------------------------------------------*/
    munmap(fb_mem, video_buf.length);
    printf("unmap成功 \n");
    close(video_fd);
    printf("摄像头关闭成功 \n");
    return 0;
}