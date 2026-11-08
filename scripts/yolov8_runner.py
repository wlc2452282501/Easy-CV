#!/usr/bin/env python3
"""Small, explicit bridge between the Qt workbench and Ultralytics Python APIs."""

import argparse
import json
import platform
import sys
def load_ultralytics():
    try:
        import ultralytics  # noqa: F401
        from ultralytics import YOLO
        return YOLO
    except ImportError as exc:
        raise RuntimeError("Ultralytics is not installed. Run prepare_yolov8_env.sh first.") from exc


def resolve_device(value):
    if value and value != "auto":
        return value
    try:
        import torch
        return "0" if torch.cuda.is_available() else "cpu"
    except ImportError:
        return "cpu"


def run_env():
    import importlib.util

    result = {
        "python": platform.python_version(),
        "platform": platform.platform(),
        "ultralytics": None,
        "torch": None,
        "cuda_available": False,
        "device": "cpu",
    }
    try:
        import ultralytics
        result["ultralytics"] = ultralytics.__version__
    except ImportError:
        pass
    if importlib.util.find_spec("torch") is not None:
        import torch
        result["torch"] = torch.__version__
        result["cuda_available"] = bool(torch.cuda.is_available())
        result["device"] = "0" if result["cuda_available"] else "cpu"
    print(json.dumps(result, ensure_ascii=False))


def require_argument(value, name):
    if not value:
        raise RuntimeError("Missing required argument: %s" % name)
    return value


def run_task(args):
    # Keep training and export in Python; Qt only owns process control and logs.
    YOLO = load_ultralytics()
    model_path = require_argument(args.model, "--model")
    device = resolve_device(args.device)
    model = YOLO(model_path)

    if args.task == "train":
        data = require_argument(args.data, "--data")
        model.train(data=data, epochs=args.epochs, imgsz=args.imgsz,
                    batch=args.batch, device=device, project=args.output_dir)
    elif args.task == "val":
        data = require_argument(args.data, "--data")
        metrics = model.val(data=data, imgsz=args.imgsz, device=device,
                            project=args.output_dir)
        print(json.dumps({"map50": float(metrics.box.map50),
                          "map50_95": float(metrics.box.map)}, ensure_ascii=False))
    elif args.task == "predict":
        source = require_argument(args.source, "--source")
        model.predict(source=source, imgsz=args.imgsz, conf=args.conf,
                      iou=args.iou, device=device, save=True,
                      project=args.output_dir, name="predict", stream=False)
    elif args.task == "export":
        model.export(format="onnx", imgsz=args.imgsz, half=args.half,
                     project=args.output_dir)
    elif args.task == "quantize":
        data = require_argument(args.data, "--data")
        model.export(format="openvino", imgsz=args.imgsz, int8=True,
                     data=data, project=args.output_dir)
    else:
        raise RuntimeError("Unknown task: %s" % args.task)


def parse_args():
    parser = argparse.ArgumentParser(description="Qt bridge for Ultralytics YOLOv8")
    parser.add_argument("--task", choices=["env", "train", "val", "predict", "export", "quantize"], required=True)
    parser.add_argument("--model", default="yolov8n.pt")
    parser.add_argument("--data")
    parser.add_argument("--source")
    parser.add_argument("--device", default="auto")
    parser.add_argument("--epochs", type=int, default=100)
    parser.add_argument("--imgsz", type=int, default=640)
    parser.add_argument("--batch", type=int, default=16)
    parser.add_argument("--conf", type=float, default=0.25)
    parser.add_argument("--iou", type=float, default=0.45)
    parser.add_argument("--half", action="store_true")
    parser.add_argument("--output-dir", default="runs/vision_platform")
    return parser.parse_args()


if __name__ == "__main__":
    arguments = parse_args()
    try:
        if arguments.task == "env":
            run_env()
        else:
            run_task(arguments)
    except Exception as error:
        print("ERROR: %s" % error, file=sys.stderr)
        sys.exit(1)
