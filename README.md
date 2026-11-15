# Qt + OpenCV 视觉平台

这是一个面向 Ubuntu 22.04 的 Qt 5.12.4/qmake C++ 工程，包含 OpenCV PCA 人脸识别、节点流程和 YOLOv8 工作台。

首页的 **OpenCV 工作台** 包含“PCA 人脸识别”和“OpenCV 节点流程”两个标签页。
人脸识别支持至少 10 人的人员库导入/录入、直方图均衡化预处理、PCA 训练、平均脸与特征脸展示、全体人员距离比对、阈值拒识、独立测试集评估和 CSV 导出。
数据格式与使用方法见 [PCA 人脸识别说明](docs/PCA_FACE_RECOGNITION.md)。已从官方整理 ORL / AT&T 数据库：40 人、400 张照片，按训练 280 张、验证 40 张、测试 80 张划分；直接导入 `data/att_faces/train` 即可训练。详见 [数据集说明](data/att_faces/README.md)。

当前第 2 阶段已加入基础 OpenCV 节点流程：灰度化、高斯滤波、Canny、固定阈值和 Haar 人脸检测。流程页支持演示图像、外部图像、节点级预览、完整结果和代码片段查看。

界面布局保存在 `ui/*.ui`，可以用 Qt Designer 直接调整控件、尺寸、布局和文字；对应的 C++ 页面只负责信号连接、状态更新和业务逻辑。qmake 会通过 `FORMS` 自动运行 `uic` 生成表单头文件。

## 在虚拟机 Linux 中构建

```bash
qmake -v
chmod +x scripts/check_opencv.sh
./scripts/check_opencv.sh
qmake VisionPlatform.pro
make -j"$(nproc)"
./VisionPlatform
```

第 3 阶段的 YOLOv8 依赖可以在 Linux 虚拟机中自动检测和安装：

```bash
chmod +x scripts/prepare_yolov8_env.sh
./scripts/prepare_yolov8_env.sh
```

脚本会选择 `python3`/`python`，检查并安装 `ultralytics`，检测 `torch.cuda.is_available()`，最后输出推荐的 `cpu` 或 `cuda` 设备。默认使用清华 PyPI 镜像，并设置 300 秒超时和 10 次重试；可用 `YOLO_PIP_INDEX_URL`、`YOLO_PIP_TIMEOUT`、`YOLO_ULTRALYTICS_SPEC` 覆盖。训练、导出和量化仍由 Qt 通过 `QProcess` 调用外部 Python 工具链。

欢迎页中的 YOLOv8 工作台会调用 `scripts/yolov8_runner.py`，提供训练、验证、摄像头/视频推理、ONNX 导出和 OpenVINO INT8 量化入口。训练和量化需要分别提供数据集 YAML；摄像头源可以填写 `0`。

如果系统同时安装多个 Qt 版本，请使用 Qt 5.12.4 对应的 `qmake`。构建文件固定使用 qmake，不使用 CMake。

## OpenCV 链接边界

虚拟机检查已确认工程链接所需的 `core`、`imgproc`、`imgcodecs`、`highgui`、`face`、`objdetect`、`videoio` 和 `dnn` 库，路径分别为 `/usr/local/include/opencv4` 与 `/usr/local/lib`。

## 后续实现约束

- 流程编辑器使用节点注册表管理算子，节点保存参数、输入输出图像和可查看的 OpenCV 代码片段。
- 课程人脸识别使用显式的 PCA / Eigenfaces 和最近邻距离阈值；人员库按“根目录 / 姓名或学号 / 图片”组织，训练至少需要 10 位有效人员。
- YOLOv8 官方训练与导出通常由 Python 工具链完成；如果接入，将由 Qt 通过 `QProcess` 管理外部进程、参数和日志，不把 Python 代码伪装成 C++。
