#include "noderegistry.h"

QVector<NodeDescriptor> NodeRegistry::descriptors()
{
    QVector<NodeDescriptor> result;
    result.append(NodeDescriptor{VisionNode::Grayscale, QStringLiteral("grayscale"), QStringLiteral("灰度化")});
    result.append(NodeDescriptor{VisionNode::GaussianBlur, QStringLiteral("gaussian_blur"), QStringLiteral("高斯滤波")});
    result.append(NodeDescriptor{VisionNode::Canny, QStringLiteral("canny"), QStringLiteral("Canny 边缘")});
    result.append(NodeDescriptor{VisionNode::Threshold, QStringLiteral("threshold"), QStringLiteral("固定阈值")});
    result.append(NodeDescriptor{VisionNode::FaceDetect, QStringLiteral("face_detect"), QStringLiteral("人脸检测")});
    return result;
}

VisionNode NodeRegistry::create(VisionNode::Type type)
{
    return VisionNode(type);
}
