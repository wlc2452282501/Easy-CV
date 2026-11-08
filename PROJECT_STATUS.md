# 项目状态

## 当前阶段

第 3 阶段：YOLOv8 工作台基础功能和外部 Python 进程接口。

## 已完成

- 建立 `VisionPlatform.pro`，固定 Qt 5.12.4/qmake 目标和 Ubuntu Linux OpenCV 路径。
- 完成欢迎页、项目标题、内置 SVG 图片和三个模块入口。
- 完成模块占位页与返回导航。
- 增加 `scripts/check_opencv.sh`，检查 `/usr/local/include/opencv4` 头文件、`/usr/local/lib` 必需库；虚拟机检查已确认 `dnn` 存在。
- 写入 Linux 虚拟机中的构建和运行说明。
- 补充 `docs/PHASE_2_SCOPE.md`，列出首期算子和人脸检测/身份识别边界。
- 加入 `core/visionnode.*` 和 `core/pipeline.*`，支持灰度、高斯滤波、Canny、阈值及 Haar 人脸检测。
- 加入 `core/noderegistry.*`，通过节点描述和工厂创建流程节点，保留后续算子扩展边界。
- 加入流程编辑页，支持图像载入、演示图像、节点列表、节点级预览、完整流程结果和 OpenCV 代码片段。
- 增加 `scripts/prepare_yolov8_env.sh`，自动检查 Python、安装 Ultralytics 并检测 CUDA。
- 补充 `docs/YOLOV8_INTEGRATION.md`，明确 Qt/QProcess 与 Python 工具链的数据流、量化格式和校准要求。
- 虚拟机已通过 OpenCV 依赖检查，确认 `opencv_dnn` 存在，并已生成 qmake/Makefile 编译产物；当前构建阶段无错误。
- 虚拟机已成功安装 Ultralytics 8.4.205，检测结果为无 NVIDIA/CUDA，推荐设备为 CPU；安装脚本已调整为新环境优先使用 CPU-only PyTorch。
- 加入 `scripts/yolov8_runner.py`，显式封装环境检查、训练、验证、推理、ONNX 导出和 OpenVINO INT8 量化。
- 加入 YOLOv8 工作台页面，支持参数编辑、文件选择、环境准备、日志查看和 QProcess 任务状态。
- 将主窗口、欢迎页、流程页、YOLOv8 页和模块占位页改为 Qt Designer `.ui` 表单；布局修改不再需要从头阅读页面业务代码。
- 为页面类、节点执行器、节点注册表和 Python 桥接脚本补充了类级、关键流程级和边界说明注释。

## 关键决定

- 节点流程先使用单张图像输入和单张图像输出，流程按节点顺序执行；节点错误会停止后续节点并在界面显示。
- OpenCV `dnn` 已根据虚拟机检查结果加入 qmake 链接项，为第 3 阶段 ONNX 推理保留依赖。
- 节点流程和 YOLOv8 工作台通过独立模块边界扩展，避免把 Python 训练流程混入 C++。
- Qt 只负责参数、进程和日志；YOLOv8 训练与导出保留在明确的 Python 桥接脚本中。
- `.ui` 文件负责可视化布局，`.cpp` 文件负责行为，`VisionPlatform.pro` 的 `FORMS` 负责 uic 生成。

## 待确认问题

- “人脸身份识别”按已确认的决定暂缓，当前只实现人脸检测；未来启用时仍需指定算法、模型来源、人员数据集格式和授权范围。
- YOLOv8 环境采用自动检测 CPU/GPU、Python、Ultralytics 和 CUDA 的方案；进入第 3 阶段前仍需确认数据集格式和量化目标格式。

## 下一步

1. 在 Ubuntu 22.04 虚拟机执行 `scripts/check_opencv.sh`、qmake 构建并反馈结果。
2. 使用带镜像和重试的 `scripts/prepare_yolov8_env.sh` 完成 Ultralytics 安装。
3. 在虚拟机重建工程，检查 YOLOv8 工作台的环境检查、训练、验证、推理和导出动作。
