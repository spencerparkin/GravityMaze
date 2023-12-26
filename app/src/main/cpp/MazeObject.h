#pragma once

#include "Color.h"
#include "Math/Utilities/Transform.h"

class DrawHelper;

class MazeObject
{
public:
    MazeObject();
    virtual ~MazeObject();

    virtual void Render(DrawHelper& drawHelper, double transitionAlpha) const;

    void CalcRenderTransform(PlanarPhysics::Transform& renderTransform, double transitionAlpha) const;

    Color color;
    PlanarPhysics::Transform transitionTransform;
};