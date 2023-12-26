#include "MazeWall.h"
#include "../DrawHelper.h"

using namespace PlanarPhysics;

MazeWall::MazeWall()
{
}

/*virtual*/ MazeWall::~MazeWall()
{
}

/*static*/ MazeWall* MazeWall::Create()
{
    return new MazeWall();
}

/*virtual*/ PlanarObject* MazeWall::CreateNew() const
{
    return new MazeWall();
}

/*virtual*/ void MazeWall::Render(DrawHelper& drawHelper, double transitionAlpha) const
{
    Transform renderTransform;
    this->CalcRenderTransform(renderTransform, transitionAlpha);

    LineSegment renderSegment = renderTransform.TransformLineSegment(this->lineSeg);

    drawHelper.DrawLine(renderSegment.vertexA, renderSegment.vertexB, this->color);
}