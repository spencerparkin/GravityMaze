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

/*virtual*/ void MazeBall::Render(DrawHelper& drawHelper, double transitionAlpha) const
{
    Transform renderTransform;
    this->CalcRenderTransform(renderTransform, transitionAlpha);

    Vector2D renderPosition = renderTransform.TransformPoint(this->position);

    drawHelper.DrawCircle(renderPosition, this->radius, this->color);
}