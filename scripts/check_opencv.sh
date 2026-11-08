#!/usr/bin/env bash
set -eu

INCLUDE_DIR="/usr/local/include/opencv4"
LIB_DIR="/usr/local/lib"
required_headers=(
  "opencv2/core.hpp"
  "opencv2/imgproc.hpp"
  "opencv2/imgcodecs.hpp"
  "opencv2/highgui.hpp"
  "opencv2/face.hpp"
  "opencv2/objdetect.hpp"
  "opencv2/videoio.hpp"
)
required_libs=(
  "opencv_core"
  "opencv_imgproc"
  "opencv_imgcodecs"
  "opencv_highgui"
  "opencv_face"
  "opencv_objdetect"
  "opencv_videoio"
)

echo "Checking OpenCV headers in ${INCLUDE_DIR}"
for header in "${required_headers[@]}"; do
  if [ ! -f "${INCLUDE_DIR}/${header}" ]; then
    echo "ERROR: missing header ${INCLUDE_DIR}/${header}" >&2
    exit 1
  fi
done

echo "Checking OpenCV libraries in ${LIB_DIR}"
for library in "${required_libs[@]}"; do
  found="false"
  for candidate in "${LIB_DIR}/lib${library}.so" "${LIB_DIR}/lib${library}.so."*; do
    if [ -e "${candidate}" ]; then
      found="true"
      break
    fi
  done
  if [ "${found}" != "true" ]; then
    echo "ERROR: missing library ${LIB_DIR}/lib${library}.so*" >&2
    exit 1
  fi
done

if [ -e "${LIB_DIR}/libopencv_dnn.so" ] || compgen -G "${LIB_DIR}/libopencv_dnn.so.*" > /dev/null; then
  echo "OpenCV module: opencv_dnn found (available for YOLOv8 ONNX stage)."
else
  echo "Optional module: opencv_dnn not found (required before ONNX inference)."
fi

echo "OpenCV prerequisites are available for stage 2."
