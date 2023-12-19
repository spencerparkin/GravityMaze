#pragma once

#include "Math/GeometricAlgebra/Vector2D.h"

class Color;

class DrawHelper
{
public:
    DrawHelper();
    virtual ~DrawHelper();

    void DrawLine(const PlanarPhysics::Vector2D& pointA, const PlanarPhysics::Vector2D& pointB, const Color& color);
    void DrawCircle(const PlanarPhysics::Vector2D& center, double radius, const Color& color);
};
