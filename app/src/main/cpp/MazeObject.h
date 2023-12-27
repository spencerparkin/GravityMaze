#pragma once

#include "Color.h"
#include "Math/Utilities/Transform.h"
#include "Math/GeometricAlgebra/Vector2D.h"

class DrawHelper;

class MazeObject
{
public:
    MazeObject();
    virtual ~MazeObject();

    virtual void Render(DrawHelper& drawHelper, double transitionAlpha) const;
    virtual PlanarPhysics::Vector2D GetPosition() const;

    void CalcRenderTransform(PlanarPhysics::Transform& renderTransform, double transitionAlpha) const;

    Color color;
    PlanarPhysics::Transform sourceTransform;
    PlanarPhysics::Transform targetTransform;
};