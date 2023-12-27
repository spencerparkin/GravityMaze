#pragma once

#include "PlanarObjects/Ball.h"
#include "../MazeObject.h"

class MazeBall : public PlanarPhysics::Ball, public MazeObject
{
public:
    MazeBall();
    virtual ~MazeBall();

    static MazeBall* Create();

    virtual PlanarObject* CreateNew() const override;
    virtual void Render(DrawHelper& drawHelper, double transitionAlpha) const override;
    virtual PlanarPhysics::Vector2D GetPosition() const override;
};