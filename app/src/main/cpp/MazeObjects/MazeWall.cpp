#include "MazeWall.h"
#include "../DrawHelper.h"

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

/*virtual*/ void MazeWall::Render(DrawHelper& drawHelper) const
{
    drawHelper.DrawLine(this->lineSeg.vertexA, this->lineSeg.vertexB, this->color);
}