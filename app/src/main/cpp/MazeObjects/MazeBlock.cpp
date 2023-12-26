#include "MazeBlock.h"
#include "../DrawHelper.h"
#include "Math/Utilities/LineSegment.h"

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

/*virtual*/ PlanarObject* MazeBlock::CreateNew() const
{
    return new MazeBlock();
}

/*virtual*/ void MazeBlock::Render(DrawHelper& drawHelper, double transitionAlpha) const
{
    Transform renderTransform;
    this->CalcRenderTransform(renderTransform, transitionAlpha);

    const ConvexPolygon& polygon = this->GetWorldPolygon();
    for(int i = 0; i < polygon.GetVertexCount(); i++)
    {
        int j = (i + 1) % polygon.GetVertexCount();

        const Vector2D& vertexA = polygon.GetVertexArray()[i];
        const Vector2D& vertexB = polygon.GetVertexArray()[j];

        LineSegment lineSeg(vertexA, vertexB);
        LineSegment renderSeg = renderTransform.TransformLineSegment(lineSeg);

        drawHelper.DrawLine(renderSeg.vertexA, renderSeg.vertexB, this->color);
    }
}