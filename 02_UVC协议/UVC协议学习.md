这是为您整理的阶段二学习计划的 Markdown 格式文档。您可以直接复制保存为 `.md` 文件：

```markdown
# 阶段二：UVC 协议深度剖析与参数控制

**预计课时**：2 课时

## 🎯 教学目标
深入理解 USB 摄像头内部工作原理，学会控制硬件参数，实现从“被动”接收图像数据到“主动”控制摄像头硬件的进阶。

## 📖 核心内容

### 1. UVC 规范解读与硬件拓扑结构
现代 USB 摄像头绝大多数遵循 UVC（USB Video Class）规范。在 UVC 协议中，设备的功能被明确划分为两类核心接口：
*   **VideoControl (VC) 控制接口**：用于调节摄像头的硬件参数（如亮度、对比度、焦距等）。
*   **VideoStreaming (VS) 数据接口**：用于传输视频流数据。

在硬件拓扑上，数据流转过程包含以下核心概念：
*   **输入终端 (IT, Input Terminal)**：如相机的物理传感器，负责接收外部信号。
*   **处理单元 (PU, Processing Unit)**：负责对数字图像进行色彩和画质处理（调整亮度、对比度、色调等）。
*   **输出终端 (OT, Output Terminal)**：将处理后的数据通过 USB 接口发送到电脑主机。
*注：我们在应用层进行的所有“调参”操作，本质上都是在向硬件的**处理单元 (PU)**下发指令。*

### 2. V4L2 Control API (动态控制属性)
在应用层代码中，V4L2 框架为参数控制提供了一套统一的 Control API。核心流程依赖以下三个 `ioctl` 命令：

*   **步骤 A：查询控制项属性 (`VIDIOC_QUERYCTRL`)**
    查询某项特定参数的能力，以获取该参数在当前硬件上的**最小值**、**最大值**、**步长**以及**默认值**。
    ```c
    struct v4l2_queryctrl queryctrl;
    queryctrl.id = V4L2_CID_BRIGHTNESS;
    ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl);
    ```

*   **步骤 B：读取当前参数值 (`VIDIOC_G_CTRL`)**
    获取摄像头当前正在使用的属性值。
    ```c
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_BRIGHTNESS;
    ioctl(fd, VIDIOC_G_CTRL, &ctrl);
    // 此时 ctrl.value 中存储的即为当前亮度值
    ```

*   **步骤 C：动态设置新参数 (`VIDIOC_S_CTRL`)**
    修改参数值并下发给底层驱动，以改变摄像头的画面效果。
    ```c
    struct v4l2_control ctrl;
    ctrl.id = V4L2_CID_BRIGHTNESS; 
    ctrl.value = 128; // 新的亮度值（需在 QUERYCTRL 查询出的合法范围内）
    ioctl(fd, VIDIOC_S_CTRL, &ctrl);
    ```

### 3. 命令行调试辅助
在将控制代码写入 C 语言之前，推荐使用 `v4l-utils` 工具链进行硬件验证：
*   **列出当前硬件所有支持的控制项（Controls）**：
    ```bash
    v4l2-ctl --list-ctrls
    ```
*   **通过命令行动态调节亮度**（立即生效）：
    ```bash
    v4l2-ctl -c brightness=128
    ```

## 💻 实验任务
**实战目标**：在阶段一的基础数据采集实验上，**增加命令行调节亮度的功能**。

**任务拆解**：
1. 在之前编写的 `video_test.c` 代码中，增加参数解析功能，允许用户通过命令行传入亮度值（例如运行 `./video_test -b 100`）。
2. 在打开设备节点后，利用 `VIDIOC_S_CTRL` 将摄像头的亮度动态设定为用户传入的值。
3. 执行缓冲区申请、入队与采集流，抓取并保存一张图像。
4. **效果验证**：分别传入极低和极高的亮度值执行程序，对比生成的图片，验证底层硬件参数控制是否成功生效。
```
