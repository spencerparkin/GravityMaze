#pragma once

#include "PlanarObjects/RigidBody.h"
#include "Engine.h"
#include "../MazeObject.h"

class MazeBlock : public PlanarPhysics::RigidBody, public MazeObject
{
public:
    MazeBlock();
    virtual ~MazeBlock();

    virtual void Render(DrawHelper& drawHelper, double transitionAlpha) const override;
    virtual PlanarPhysics::Vector2D GetPosition() const override;
};

class GoodMazeBlock : public MazeBlock
{
public:
    GoodMazeBlock();
    virtual ~GoodMazeBlock();

    static GoodMazeBlock* Create();

    virtual PlanarObject* CreateNew() const override;

    void SetTouched(bool touched);
    bool IsTouched() const;

private:
    bool touched;
};

class EvilMazeBlock : public MazeBlock
{
public:
    EvilMazeBlock();
    virtual ~EvilMazeBlock();

    static EvilMazeBlock* Create();

    virtual PlanarObject* CreateNew() const override;
    virtual void CollisionOccurredWith(PlanarPhysics::PlanarObject* planarObject, PlanarPhysics::Engine* engine) override;
};