#include "MazeBall.h"
#include "../DrawHelper.h"

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

/*virtual*/ void MazeBall::Render(DrawHelper& drawHelper)
{
    drawHelper.DrawCircle(this->position, this->radius, this->color);
}