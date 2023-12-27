#pragma once

#include "PlanarObjects/Wall.h"
#include "../MazeObject.h"

class MazeWall : public PlanarPhysics::Wall, public MazeObject
{
public:
    MazeWall();
    virtual ~MazeWall();

    static MazeWall* Create();

    virtual PlanarObject* CreateNew() const override;
    virtual void Render(DrawHelper& drawHelper, double transitionAlpha) const override;
    virtual PlanarPhysics::Vector2D GetPosition() const override;
};