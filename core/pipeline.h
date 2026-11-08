#ifndef PIPELINE_H
#define PIPELINE_H

#include <QVector>

#include "visionnode.h"

/// Ordered node executor used by both previews and full pipeline runs.
class Pipeline
{
public:
    struct NodeResult {
        cv::Mat image;
        QString code;
        QString error;
    };

    void addNode(const VisionNode &node);
    void removeNode(int index);
    void clear();
    int size() const;
    const VisionNode &nodeAt(int index) const;
    /// Runs nodes in insertion order and stops at the first failed node.
    QVector<NodeResult> execute(const cv::Mat &input) const;

private:
    QVector<VisionNode> m_nodes;
};

#endif // PIPELINE_H
