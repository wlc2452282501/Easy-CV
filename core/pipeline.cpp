#include "pipeline.h"

void Pipeline::addNode(const VisionNode &node)
{
    m_nodes.append(node);
}

void Pipeline::removeNode(int index)
{
    if (index >= 0 && index < m_nodes.size()) {
        m_nodes.removeAt(index);
    }
}

void Pipeline::clear()
{
    m_nodes.clear();
}

int Pipeline::size() const
{
    return m_nodes.size();
}

const VisionNode &Pipeline::nodeAt(int index) const
{
    return m_nodes.at(index);
}

QVector<Pipeline::NodeResult> Pipeline::execute(const cv::Mat &input) const
{
    QVector<NodeResult> results;
    cv::Mat current = input;
    for (const VisionNode &node : m_nodes) {
        NodeResult result;
        result.code = node.codeSnippet();
        result.image = node.process(current, &result.error);
        results.append(result);
        if (!result.error.isEmpty() || result.image.empty()) {
            break;
        }
        current = result.image;
    }
    return results;
}
