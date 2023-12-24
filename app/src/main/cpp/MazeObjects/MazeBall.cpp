#include "MazeBall.h"
#include "../DrawHelper.h"

using namespace PlanarPhysics;

MazeBall::MazeBall()
{
}

/*virtual*/ MazeBall::~MazeBall()
{
}

/*static*/ MazeBall* MazeBall::Create()
{
    return new MazeBall();
}

/*virtual*/ PlanarObject* MazeBall::CreateNew() const
{
    return new MazeBall();
}

/*virtual*/ void MazeBall::Render(DrawHelper& drawHelper) const
{
    drawHelper.DrawCircle(this->position, this->radius, this->color);
}