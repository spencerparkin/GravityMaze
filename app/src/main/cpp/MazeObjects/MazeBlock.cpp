#include "MazeBlock.h"
#include "../DrawHelper.h"

using namespace PlanarPhysics;

MazeBlock::MazeBlock()
{
}

/*virtual*/ MazeBlock::~MazeBlock()
{
}

/*static*/ MazeBlock* MazeBlock::Create()
{
    return new MazeBlock();
}

/*virtual*/ void MazeBlock::Render(DrawHelper& drawHelper) const
{
    const ConvexPolygon& polygon = this->GetWorldPolygon();
    for(int i = 0; i < polygon.GetVertexCount(); i++)
    {
        int j = (i + 1) % polygon.GetVertexCount();

        const Vector2D& vertexA = polygon.GetVertexArray()[i];
        const Vector2D& vertexB = polygon.GetVertexArray()[j];

        drawHelper.DrawLine(vertexA, vertexB, this->color);
    }
}