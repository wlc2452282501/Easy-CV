#include "visionnode.h"

#include <QFileInfo>
#include <QStringList>
#include <QtGlobal>

#include <vector>

#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

namespace {

cv::Mat toGray(const cv::Mat &input)
{
    if (input.channels() == 1) {
        return input;
    }

    cv::Mat gray;
    cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    return gray;
}

QString defaultCascadePath()
{
    const QStringList candidates = QStringList()
        << QStringLiteral("/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml")
        << QStringLiteral("/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_default.xml");
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return QString();
}

} // namespace

VisionNode::VisionNode(Type type)
    : m_type(type)
{
    switch (m_type) {
    case GaussianBlur:
        m_parameters.insert(QStringLiteral("kernelSize"), 5);
        break;
    case Canny:
        m_parameters.insert(QStringLiteral("lowThreshold"), 80);
        m_parameters.insert(QStringLiteral("highThreshold"), 160);
        break;
    case Threshold:
        m_parameters.insert(QStringLiteral("threshold"), 128);
        break;
    case FaceDetect:
        m_parameters.insert(QStringLiteral("cascadePath"), defaultCascadePath());
        m_parameters.insert(QStringLiteral("scaleFactor"), 1.1);
        m_parameters.insert(QStringLiteral("minNeighbors"), 4);
        break;
    case Grayscale:
        break;
    }
}

VisionNode::Type VisionNode::type() const
{
    return m_type;
}

QString VisionNode::name() const
{
    switch (m_type) {
    case Grayscale:
        return QStringLiteral("灰度化");
    case GaussianBlur:
        return QStringLiteral("高斯滤波");
    case Canny:
        return QStringLiteral("Canny 边缘");
    case Threshold:
        return QStringLiteral("固定阈值");
    case FaceDetect:
        return QStringLiteral("人脸检测");
    }
    return QStringLiteral("未知节点");
}

void VisionNode::setParameter(const QString &key, const QVariant &value)
{
    m_parameters.insert(key, value);
}

QVariant VisionNode::parameter(const QString &key) const
{
    return m_parameters.value(key);
}

QString VisionNode::codeSnippet() const
{
    switch (m_type) {
    case Grayscale:
        return QStringLiteral("cv::cvtColor(input, output, cv::COLOR_BGR2GRAY);");
    case GaussianBlur:
        return QStringLiteral("cv::GaussianBlur(input, output, cv::Size(%1, %1), 0);")
            .arg(m_parameters.value(QStringLiteral("kernelSize")).toInt());
    case Canny:
        return QStringLiteral("cv::Canny(gray, output, %1, %2);")
            .arg(m_parameters.value(QStringLiteral("lowThreshold")).toInt())
            .arg(m_parameters.value(QStringLiteral("highThreshold")).toInt());
    case Threshold:
        return QStringLiteral("cv::threshold(gray, output, %1, 255, cv::THRESH_BINARY);")
            .arg(m_parameters.value(QStringLiteral("threshold")).toInt());
    case FaceDetect:
        return QStringLiteral(
            "cv::CascadeClassifier classifier(\"%1\");\n"
            "classifier.detectMultiScale(gray, faces, %2, %3);")
            .arg(m_parameters.value(QStringLiteral("cascadePath")).toString())
            .arg(m_parameters.value(QStringLiteral("scaleFactor")).toDouble(), 0, 'f', 2)
            .arg(m_parameters.value(QStringLiteral("minNeighbors")).toInt());
    }
    return QString();
}

cv::Mat VisionNode::process(const cv::Mat &input, QString *errorMessage) const
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (input.empty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("输入图像为空");
        }
        return cv::Mat();
    }

    try {
        switch (m_type) {
        case Grayscale: {
            return toGray(input).clone();
        }
        case GaussianBlur: {
            const int kernel = qMax(1, m_parameters.value(QStringLiteral("kernelSize")).toInt());
            const int normalizedKernel = (kernel % 2 == 0) ? kernel + 1 : kernel;
            cv::Mat output;
            cv::GaussianBlur(input, output, cv::Size(normalizedKernel, normalizedKernel), 0);
            return output;
        }
        case Canny: {
            cv::Mat output;
            cv::Canny(toGray(input), output,
                      m_parameters.value(QStringLiteral("lowThreshold")).toDouble(),
                      m_parameters.value(QStringLiteral("highThreshold")).toDouble());
            return output;
        }
        case Threshold: {
            cv::Mat output;
            cv::threshold(toGray(input), output,
                          m_parameters.value(QStringLiteral("threshold")).toDouble(),
                          255, cv::THRESH_BINARY);
            return output;
        }
        case FaceDetect: {
            const QString cascadePath = m_parameters.value(QStringLiteral("cascadePath")).toString();
            if (cascadePath.isEmpty()) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("未找到 Haar 人脸分类器，请配置 cascadePath");
                }
                return cv::Mat();
            }

            cv::CascadeClassifier classifier;
            if (!classifier.load(cascadePath.toStdString())) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("无法加载人脸分类器：%1").arg(cascadePath);
                }
                return cv::Mat();
            }

            cv::Mat output = input.clone();
            std::vector<cv::Rect> faces;
            classifier.detectMultiScale(
                toGray(input), faces,
                m_parameters.value(QStringLiteral("scaleFactor")).toDouble(),
                m_parameters.value(QStringLiteral("minNeighbors")).toInt());
            for (const cv::Rect &face : faces) {
                cv::rectangle(output, face, cv::Scalar(0, 220, 0), 2);
            }
            return output;
        }
        }
    } catch (const cv::Exception &exception) {
        if (errorMessage) {
            *errorMessage = QString::fromUtf8(exception.what());
        }
    }
    return cv::Mat();
}
