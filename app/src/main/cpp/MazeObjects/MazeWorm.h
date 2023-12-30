#pragma once

#include "PlanarObjects/Ball.h"
#include "../MazeObject.h"

class MazeWorm : public PlanarPhysics::Ball, public MazeObject
{
public:
    MazeWorm();
    virtual ~MazeWorm();

    static MazeWorm* Create();

    virtual PlanarObject* CreateNew() const override;
    virtual void AdvanceBegin() override;
    virtual void Render(DrawHelper& drawHelper, double transitionAlpha) const override;
    virtual PlanarPhysics::Vector2D GetPosition() const override;
    virtual void CollisionOccurredWith(PlanarPhysics::PlanarObject* planarObject, PlanarPhysics::Engine* engine) override;

private:

    std::vector<PlanarPhysics::Vector2D> positionFifo;
    int maxPositionFifoSize;
};