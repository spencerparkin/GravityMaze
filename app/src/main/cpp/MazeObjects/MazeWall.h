#pragma once

#include "PlanarObjects/Wall.h"
#include "../MazeObject.h"

class MazeWall : public PlanarPhysics::Wall, public MazeObject
{
public:
    MazeWall();
    virtual ~MazeWall();

    static MazeWall* Create();

    virtual void Render(DrawHelper& drawHelper) override;
};