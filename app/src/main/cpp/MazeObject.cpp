#include "MazeObject.h"

using namespace PlanarPhysics;

MazeObject::MazeObject()
{
    this->sourceTransform.Identity();
    this->targetTransform.Identity();
}

/*virtual*/ MazeObject::~MazeObject()
{
}

/*virtual*/ void MazeObject::Render(DrawHelper& drawHelper, double transitionAlpha) const
{
}

void MazeObject::CalcRenderTransform(PlanarPhysics::Transform& renderTransform, double transitionAlpha) const
{
    renderTransform.Interpolate(this->sourceTransform, this->targetTransform, transitionAlpha);
}