#!/usr/bin/env bash
set -eu

# Detect the local Python/Ultralytics/CUDA state and install Ultralytics when it is absent.
PIP_INDEX_URL="${YOLO_PIP_INDEX_URL:-https://pypi.tuna.tsinghua.edu.cn/simple}"
PIP_TIMEOUT="${YOLO_PIP_TIMEOUT:-300}"
ULTRALYTICS_SPEC="${YOLO_ULTRALYTICS_SPEC:-ultralytics}"
NVIDIA_AVAILABLE="false"
if command -v nvidia-smi > /dev/null 2>&1; then
  NVIDIA_AVAILABLE="true"
fi
PYTHON_BIN=""
for candidate in python3 python; do
  if command -v "${candidate}" > /dev/null 2>&1; then
    PYTHON_BIN="${candidate}"
    break
  fi
done

if [ -z "${PYTHON_BIN}" ]; then
  if command -v apt-get > /dev/null 2>&1; then
    echo "Python 3 is missing; attempting to install python3 and python3-pip."
    if [ "$(id -u)" -eq 0 ]; then
      apt-get update
      apt-get install -y python3 python3-pip
    elif command -v sudo > /dev/null 2>&1; then
      sudo apt-get update
      sudo apt-get install -y python3 python3-pip
    else
      echo "ERROR: sudo is required to install Python 3 automatically." >&2
      exit 1
    fi
    PYTHON_BIN="python3"
  else
    echo "ERROR: Python 3 is required and apt-get is unavailable." >&2
    exit 1
  fi
fi

echo "Python: $(${PYTHON_BIN} --version 2>&1)"

if ! "${PYTHON_BIN}" -m pip --version > /dev/null 2>&1; then
  if command -v apt-get > /dev/null 2>&1; then
    echo "pip is missing; attempting to install python3-pip."
    if [ "$(id -u)" -eq 0 ]; then
      apt-get update
      apt-get install -y python3-pip
    elif command -v sudo > /dev/null 2>&1; then
      sudo apt-get update
      sudo apt-get install -y python3-pip
    else
      echo "ERROR: sudo is required to install pip automatically." >&2
      exit 1
    fi
  else
    echo "ERROR: pip is not available and apt-get is unavailable." >&2
    exit 1
  fi
fi

pip_install_mirror() {
  "${PYTHON_BIN}" -m pip install --user --no-cache-dir --prefer-binary \
    --index-url "${PIP_INDEX_URL}" --timeout "${PIP_TIMEOUT}" --retries 10 "$@"
}

pip_install_official() {
  "${PYTHON_BIN}" -m pip install --user --no-cache-dir --prefer-binary \
    --index-url "https://pypi.org/simple" --timeout "${PIP_TIMEOUT}" --retries 10 "$@"
}

if "${PYTHON_BIN}" -c "import ultralytics" > /dev/null 2>&1; then
  echo "Ultralytics: $(${PYTHON_BIN} -c 'import ultralytics; print(ultralytics.__version__)')"
else
  echo "Ultralytics is missing; installing ${ULTRALYTICS_SPEC} with pip."
  echo "Using package index: ${PIP_INDEX_URL}; timeout: ${PIP_TIMEOUT}s; retries: 10"
  if [ "${NVIDIA_AVAILABLE}" = "true" ]; then
    if ! pip_install_mirror "${ULTRALYTICS_SPEC}"; then
      echo "Mirror installation failed; retrying with the official PyPI index."
      pip_install_official "${ULTRALYTICS_SPEC}"
    fi
  else
    echo "No NVIDIA driver detected; installing CPU-only PyTorch before Ultralytics."
    "${PYTHON_BIN}" -m pip install --user --no-cache-dir --prefer-binary \
      --index-url "https://download.pytorch.org/whl/cpu" \
      --extra-index-url "${PIP_INDEX_URL}" \
      --timeout "${PIP_TIMEOUT}" --retries 10 torch torchvision
    pip_install_mirror --no-deps "${ULTRALYTICS_SPEC}"
    if ! pip_install_mirror \
      "numpy>=1.23.0" "matplotlib>=3.3.0" "opencv-python-headless>=4.7.0" \
      "pillow>=7.1.2" "pyyaml>=5.3.1" "requests>=2.23.0" "scipy>=1.4.1" \
      "psutil>=5.8.0" "polars>=0.20.0" "ultralytics-thop>=2.2.4"; then
      echo "Mirror dependency installation failed; retrying with the official PyPI index."
      pip_install_official \
        "numpy>=1.23.0" "matplotlib>=3.3.0" "opencv-python-headless>=4.7.0" \
        "pillow>=7.1.2" "pyyaml>=5.3.1" "requests>=2.23.0" "scipy>=1.4.1" \
        "psutil>=5.8.0" "polars>=0.20.0" "ultralytics-thop>=2.2.4"
    fi
  fi
  echo "Ultralytics: $(${PYTHON_BIN} -c 'import ultralytics; print(ultralytics.__version__)')"
fi

CUDA_AVAILABLE="$(${PYTHON_BIN} -c "import importlib.util; m=importlib.util.find_spec('torch'); print('false' if m is None else ('true' if __import__('torch').cuda.is_available() else 'false'))" 2>/dev/null || true)"
DEVICE="cpu"
if [ "${CUDA_AVAILABLE}" = "true" ]; then
  DEVICE="cuda"
fi

echo "NVIDIA driver tool available: ${NVIDIA_AVAILABLE}"
echo "CUDA available: ${CUDA_AVAILABLE:-false}"
echo "Recommended YOLOv8 device: ${DEVICE}"
echo "YOLO_DEVICE=${DEVICE}"
