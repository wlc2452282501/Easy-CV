#ifndef VISIONNODE_H
#define VISIONNODE_H

#include <QVariantMap>

#include <opencv2/core.hpp>

/// One OpenCV operation with serializable parameters and generated code text.
class VisionNode
{
public:
    enum Type {
        Grayscale,
        GaussianBlur,
        Canny,
        Threshold,
        FaceDetect
    };

    explicit VisionNode(Type type = Grayscale);

    Type type() const;
    QString name() const;
    void setParameter(const QString &key, const QVariant &value);
    QVariant parameter(const QString &key) const;
    QString codeSnippet() const;
    /// Executes the node and returns an empty image when errorMessage is set.
    cv::Mat process(const cv::Mat &input, QString *errorMessage) const;

private:
    Type m_type;
    QVariantMap m_parameters;
};

#endif // VISIONNODE_H
