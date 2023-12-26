#include "MazeObject.h"

using namespace PlanarPhysics;

MazeObject::MazeObject()
{
}

/*virtual*/ MazeObject::~MazeObject()
{
}

/*virtual*/ void MazeObject::Render(DrawHelper& drawHelper, double transitionAlpha) const
{
}

void MazeObject::CalcRenderTransform(PlanarPhysics::Transform& renderTransform, double transitionAlpha) const
{
    Transform identityTransform;
    renderTransform.Interpolate(this->transitionTransform, identityTransform, transitionAlpha);
}