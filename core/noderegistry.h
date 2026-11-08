#ifndef NODEREGISTRY_H
#define NODEREGISTRY_H

#include <QVector>

#include "visionnode.h"

/// Designer-facing metadata for one registered processing node.
struct NodeDescriptor
{
    VisionNode::Type type;
    QString id;
    QString displayName;
};

/// Central extension point for adding new OpenCV node types.
class NodeRegistry
{
public:
    static QVector<NodeDescriptor> descriptors();
    static VisionNode create(VisionNode::Type type);
};

#endif // NODEREGISTRY_H
