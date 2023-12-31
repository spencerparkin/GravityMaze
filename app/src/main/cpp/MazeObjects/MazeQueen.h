#pragma once

#include "PlanarObjects/Ball.h"
#include "../MazeObject.h"

class MazeQueen : public PlanarPhysics::Ball, public MazeObject
{
public:
    MazeQueen();
    virtual ~MazeQueen();

    static MazeQueen* Create();

    virtual PlanarObject* CreateNew() const override;
    virtual void Render(DrawHelper& drawHelper, double transitionAlpha) const override;
    virtual PlanarPhysics::Vector2D GetPosition() const override;
    virtual void CollisionOccurredWith(PlanarPhysics::PlanarObject* planarObject, PlanarPhysics::Engine* engine) override;

    int numConcentricCircles;
    mutable int colorRampOffset;
    std::vector<Color> colorRamp;
    bool alive;
    bool addedBlocks;
};