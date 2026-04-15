/*
 *  V4L2 (Video for Linux Two) 核心头文件 (应用开发中文精选参考版)
 *  
 *  【设计目的】
 *  本文件旨在为 Linux 视频应用开发者提供一个清晰、易读的接口参考。
 *  不仅包含了核心结构体的中文对照，还详细解释了每个成员的来源与实际物理意义。
 *
 *  【数据格式说明】
 *  - [字符串] : __u8 数组，固定长度，超出部分填 \0。
 *  - [数值]   : __u32 整数，代表像素尺寸、数量、循环索引等。
 *  - [位掩码] : __u32 整数，通过位运算 (flag & MASK) 来判断是否支持某功能。
 *  - [FOURCC] : 4 字符代码，底层是 32 位整数，用于唯一标识像素颜色格式。
 *
 *  【特别提醒】
 *  - 实际编码时建议直接引用原头文件 <linux/videodev2.h>，本文件用于学习理解及注释参考。
 */

#ifndef __LINUX_VIDEODEV2_CN_H
#define __LINUX_VIDEODEV2_CN_H

#include <linux/types.h>
#include <linux/time.h>
#include <linux/videodev2.h>

/* =========================================================================
 * 1. 四字符代码 (FOURCC) 宏定义
 * =========================================================================
 * 用于将 4 个字符组合成一个 32 位的像素格式 ID。
 */
#define v4l2_fourcc_cn(a, b, c, d) \
	((__u32)(a) | ((__u32)(b) << 8) | ((__u32)(c) << 16) | ((__u32)(d) << 24))

/* 常见视频像素格式定义 */
#define V4L2_PIX_FMT_YUYV_CN    v4l2_fourcc_cn('Y', 'U', 'Y', 'V') /* YUV 4:2:2 (每个像素 2 字节) */
#define V4L2_PIX_FMT_MJPEG_CN   v4l2_fourcc_cn('M', 'J', 'P', 'G') /* Motion-JPEG 压缩格式 */
#define V4L2_PIX_FMT_JPEG_CN    v4l2_fourcc_cn('J', 'P', 'E', 'G') /* 标准 JPEG */
#define V4L2_PIX_FMT_NV12_CN    v4l2_fourcc_cn('N', 'V', '1', '2') /* Y/CbCr 4:2:0 (Android/嵌入式常用) */
#define V4L2_PIX_FMT_RGB24_CN   v4l2_fourcc_cn('R', 'G', 'B', '3') /* RGB 8-8-8 (每像素 3 字节) */


/* =========================================================================
 * 2. 核心枚举值 (Enums)
 * =========================================================================
 */

/* 缓冲区类型 (Buffer Types) */
enum v4l2_buf_type_cn {
	V4L2_BUF_TYPE_VIDEO_CAPTURE_CN        = 1, /* 视频采集 (摄像头输入) */
	V4L2_BUF_TYPE_VIDEO_OUTPUT_CN         = 2, /* 视频输出 (屏幕/编码器输出) */
	V4L2_BUF_TYPE_VIDEO_OVERLAY_CN        = 3, /* 视频叠加/预览 */
};

/* 内存交换方式 (Memory Types) */
enum v4l2_memory_cn {
	V4L2_MEMORY_MMAP_CN             = 1, /* 常用：由内核分配，应用 mmap 映射 */
	V4L2_MEMORY_USERPTR_CN          = 2, /* 应用层分配内存，传给内核 */
	V4L2_MEMORY_DMABUF_CN           = 4, /* 高级：零拷贝，不同设备间共享内存 (如 GPU/VPU) */
};

/* 扫描模式 (Field Order) */
enum v4l2_field_cn {
	V4L2_FIELD_ANY_CN           = 0, /* 驱动自动选择 */
	V4L2_FIELD_NONE_CN          = 1, /* 逐行扫描 (Progressive) - 现代摄像头最常用 */
	V4L2_FIELD_INTERLACED_CN    = 4, /* 隔行扫描 (Interlaced) */
};


/* =========================================================================
 * 3. 核心结构体定义 (Structures)
 * =========================================================================
 */

/* --- [3.1] 设备能力结构体 --- */
/* 调用: ioctl(fd, VIDIOC_QUERYCAP, &cap) */
struct v4l2_capability_cn {
	__u8	driver[16];	/* [字符串] 驱动名称 (如: "uvcvideo") */
	__u8	card[32];	/* [字符串] 设备名称 (如: "Integrated Camera") */
	__u8	bus_info[32];	/* [字符串] 总线信息 (用于区分多个同型号摄像头) */
	__u32   version;	/* [数值]   驱动版本号 */
	__u32	capabilities;	/* [位掩码] 硬件支持的总物理能力 */
	__u32	device_caps;	/* [位掩码] 当前视频节点支持的特定能力 (建议以此为准) */
	__u32	reserved[3];	/* [保留] */
};

/* 常用能力标志位 */
#define V4L2_CAP_VIDEO_CAPTURE_CN	0x00000001  /* 是否支持采集视频 */
#define V4L2_CAP_STREAMING_CN		0x04000000  /* 是否支持流控制 (VIDIOC_STREAMON/OFF) */


/* --- [3.2] 格式描述查询 --- */
/* 用于枚举摄像头支持的所有像素格式
 * 调用: ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) */
struct v4l2_fmtdesc_cn {
	__u32		    index;             /* [数值]   格式索引，从 0 开始递增查询 */
	__u32		    type;              /* [枚举]   必须设为 V4L2_BUF_TYPE_VIDEO_CAPTURE */
	__u32               flags;             /* [位掩码] 如 V4L2_FMT_FLAG_COMPRESSED (是否为压缩格式) */
	__u8		    description[32];   /* [字符串] 人类可读的格式描述 (如 "YUYV 4:2:2") */
	__u32		    pixelformat;       /* [FOURCC] 核心：格式对应的四字符代码 */
	__u32		    reserved[4];
};


/* --- [3.3] 分辨率与像素格式设置 --- */
/* 调用: ioctl(fd, VIDIOC_S_FMT, &fmt) */
struct v4l2_pix_format_cn {
	__u32         		width;		/* [数值]   期望的图像宽度 (像素) */
	__u32         		height;		/* [数值]   期望的图像高度 (像素) */
	__u32         		pixelformat;	/* [FOURCC] 对应的格式代码 (如 V4L2_PIX_FMT_YUYV) */
	__u32			field;		/* [枚举]   扫描方式，建议填 V4L2_FIELD_NONE */
	__u32            	bytesperline;	/* [数值]   【返回】每行图像占据的字节数 (含对齐补位) */
	__u32          		sizeimage;	/* [数值]   【返回】一帧图像总大小 (单位: 字节) */
	__u32			colorspace;	/* [数值]   色彩空间定义 */
	__u32			priv;		/* [数值]   私密数据 */
};

struct v4l2_format_cn {
	__u32	 type;	/* [枚举]   必须填 V4L2_BUF_TYPE_VIDEO_CAPTURE */
	union {
		struct v4l2_pix_format_cn	pix;     /* 常用：单平面视频 */
		__u8	raw_data[200];		         /* 占位保留 */
	} fmt;
};


/* --- [3.4] 缓冲区请求 (内核空间分配) --- */
/* 调用: ioctl(fd, VIDIOC_REQBUFS, &req) */
struct v4l2_requestbuffers_cn {
	__u32			count;	 /* [数值]   请求申请的缓冲区数量 (通常 3~4 个，用于周转) */
	__u32			type;	 /* [枚举]   类型 (同上) */
	__u32			memory;	 /* [枚举]   映射方式 (如 V4L2_MEMORY_MMAP) */
	__u32			reserved[2];
};


/* --- [3.5] 缓冲区详情与数据状态 --- */
/* 调用: ioctl(fd, VIDIOC_QUERYBUF, &buf) 获取信息
 * 调用: ioctl(fd, VIDIOC_DQBUF, &buf) 抓取数据 */
struct v4l2_buffer_cn {
	__u32			index;		/* [数值]   缓冲区的编号 (从 0 开始) */
	__u32			type;		/* [枚举]   类型 (同上) */
	__u32			bytesused;	/* [数值]   【核心】本帧采集到的实际数据量 (单位: 字节) */
	__u32			flags;		/* [位掩码] 缓冲区状态标志 (如是否已入队) */
	__u32			field;		/* [枚举]   实际所用的扫描模式 */
	struct timeval		timestamp;	/* [结构体] 采集到图像时的精确系统时间 */
	__u32			sequence;	/* [数值]   帧序号，由驱动递增，可用于检测丢帧 */

	__u32			memory;		/* [枚举]   同上 */
	union {
		__u32           offset;		/* [数值]   【核心】用于 mmap 的偏移量 */
		unsigned long   userptr;	/* [指针]   用户空间指针 (若 memory == USERPTR) */
	} m;
	__u32			length;		/* [数值]   缓冲区总容量 (长度) */
	__u32			reserved2;
	__u32			reserved;
};


/* --- [3.6] 视频输入查询 --- */
/* 调用: ioctl(fd, VIDIOC_ENUMINPUT, &input) */
struct v4l2_input_cn {
	__u32	     index;		/* [数值]   输入索引 (如某些设备支持多个摄像头插口) */
	__u8	     name[32];		/* [字符串] 输入名称 (如 "Camera 1") */
	__u32	     type;		/* [数值]   类型 (V4L2_INPUT_TYPE_CAMERA 等) */
	__u32	     status;		/* [位掩码] 当前输入链路状态 (是否有信号、断电等) */
	__u32	     reserved[4];
};


/* --- [3.7] 帧率设置参数 --- */
/* 调用: ioctl(fd, VIDIOC_S_PARM, &parm) */
struct v4l2_captureparm_cn {
	__u32		   capability;	  /* [位掩码] 是否支持帧率调节 (V4L2_CAP_TIMEPERFRAME) */
	__u32		   capturemode;	  /* [数值]   采集模式 */
	struct v4l2_fract  timeperframe;  /* [分数]   【核心】每帧耗时 (1/30 代表 30fps) */
	__u16   numerator;                /* 对应 timeperframe.numerator (分子) */
	__u16   denominator;              /* 对应 timeperframe.denominator (分母) */
	__u32		   reserved[4];
};

struct v4l2_streamparm_cn {
	__u32	 type;
	union {
		struct v4l2_captureparm_cn	capture;
		__u8	raw_data[200];
	} parm;
};


/* --- [3.8] 亮度对比度调节控制 --- */
/* 调用: ioctl(fd, VIDIOC_S_CTRL, &ctrl) - 设置
 * 调用: ioctl(fd, VIDIOC_G_CTRL, &ctrl) - 获取 */
struct v4l2_control_cn {
	__u32		     id;     /* [数值]   控制项 ID (如 V4L2_CID_BRIGHTNESS) */
	__s32		     value;  /* [数值]   实际设定的数值 */
};

/* 常用控制项 ID 定义 (由 <linux/v4l2-controls.h> 提供) */
#define V4L2_CID_BRIGHTNESS_CN     (0x00980900) /* 亮度控制 */
#define V4L2_CID_CONTRAST_CN       (0x00980901) /* 对比度控制 */
#define V4L2_CID_SATURATION_CN     (0x00980902) /* 饱和度控制 */
#define V4L2_CID_HUE_CN            (0x00980903) /* 色度/色相控制 */
#define V4L2_CID_AUTOGAIN_CN       (0x00980912) /* 自动增益控制 */


/* =========================================================================
 * 4. IOCTL 系统调用宏 (常用汇总)
 * =========================================================================
 * VIDIOC_XXXX 定义了各种操作命令，通过 ioctl(fd, COMMAND, &STRUCT) 调用。
 */

/* 1. 全局能力/格式操作 */
// #define VIDIOC_QUERYCAP_CN	  _IOR('V',  0, v4l2_capability)   // 查询设备能力
// #define VIDIOC_ENUM_FMT_CN      _IOWR('V',  2, v4l2_fmtdesc)    // 枚举支持格式
// #define VIDIOC_G_FMT_CN		_IOWR('V',  4, v4l2_format)     // 获取当前格式
// #define VIDIOC_S_FMT_CN		_IOWR('V',  5, v4l2_format)     // 设置采集格式

/* 2. 内存与缓冲区操作 */
// #define VIDIOC_REQBUFS_CN	_IOWR('V',  8, v4l2_requestbuffers) // 申请缓冲区
// #define VIDIOC_QUERYBUF_CN	_IOWR('V',  9, v4l2_buffer)         // 查询映射信息
// #define VIDIOC_QBUF_CN		_IOWR('V', 15, v4l2_buffer)         // 将缓冲区放入队列
// #define VIDIOC_DQBUF_CN		_IOWR('V', 17, v4l2_buffer)         // 从队列取出数据帧

/* 3. 采集流控制 */
// #define VIDIOC_STREAMON_CN	 _IOW('V', 18, int)                 // 启动采集
// #define VIDIOC_STREAMOFF_CN	 _IOW('V', 19, int)                 // 停止采集

#endif /* __LINUX_VIDEODEV2_CN_H */
