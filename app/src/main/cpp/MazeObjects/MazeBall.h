#pragma once

#include "PlanarObjects/Ball.h"
#include "../MazeObject.h"

class MazeBall : public PlanarPhysics::Ball, public MazeObject
{
public:
    MazeBall();
    virtual ~MazeBall();

    static MazeBall* Create();

    virtual void Render(DrawHelper& drawHelper) override;
};