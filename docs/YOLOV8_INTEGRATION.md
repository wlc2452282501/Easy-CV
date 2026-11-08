# YOLOv8 集成边界

## 环境检测

Qt 工作台通过 `QProcess` 启动 `scripts/prepare_yolov8_env.sh`，读取标准输出中的 Python 版本、Ultralytics 版本、CUDA 可用性和推荐设备。缺少 Python/pip 时脚本尝试使用 Ubuntu `apt-get` 安装；缺少 Ultralytics 时使用 pip 安装。没有 NVIDIA 硬件或 PyTorch CUDA 不可用时，工作台选择 CPU。

环境脚本在检测不到 `nvidia-smi` 时先安装 CPU 版 PyTorch，再以无依赖方式安装 Ultralytics 和其余 Python 依赖，避免在纯 CPU 虚拟机中拉取数 GB 的 CUDA 运行库。已有环境不会重复安装。

## 进程接口与数据流

Qt 不嵌入 Python 训练实现。工作台创建临时参数文件或直接传递命令行参数，通过 `QProcess` 启动外部 Python/Ultralytics 命令，并实时读取标准输出和标准错误显示日志。训练、验证和实时推理使用不同的动作入口：

- 训练：数据集 YAML、模型权重、epochs、image size、batch、device -> 权重和日志目录。
- 验证：数据集 YAML + 已训练权重 -> mAP、precision、recall 等评估结果。
- 实时推理：权重 + 摄像头/视频源 -> 检测框和帧结果，不混入训练评估指标。
- ONNX 导出：权重 + opset/input size -> ONNX 文件，导出完成后由 Qt 检查文件和模型元数据。

## 量化约束

首期只规划 FP16 和 INT8 两种导出/部署方向。FP16 需要支持半精度的运行时或硬件；INT8 静态量化使用 Ultralytics 的 OpenVINO 导出路径，需要 `openvino`/`nncf`、代表性校准图像集和数据集 YAML。动态量化不作为默认目标。量化后的模型需要用独立验证集比较精度，再允许在 OpenCV DNN 或其他运行时中加载。

训练、验证、实时推理、导出和量化的具体 Python 命令将在虚拟机确认 Python、Ultralytics 和 CUDA 环境后实现，避免把不存在的版本参数写死。
