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

/*virtual*/ void MazeWall::Render(DrawHelper& drawHelper) const
{
    drawHelper.DrawLine(this->lineSeg.vertexA, this->lineSeg.vertexB, this->color);
}