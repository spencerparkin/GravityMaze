#pragma once

#include "PlanarObjects/RigidBody.h"
#include "../MazeObject.h"

class MazeBlock : public PlanarPhysics::RigidBody, public MazeObject
{
public:
    MazeBlock();
    virtual ~MazeBlock();

    static MazeBlock* Create();

    virtual void Render(DrawHelper& drawHelper) const override;
};