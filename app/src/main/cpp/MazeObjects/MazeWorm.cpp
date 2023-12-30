#include "MazeWorm.h"
#include "MazeBlock.h"
#include "Math/Utilities/Random.h"
#include "../DrawHelper.h"

using namespace PlanarPhysics;

MazeWorm::MazeWorm()
{
    this->color = Color(1.0, 0.0, 1.0);
    this->maxPositionFifoSize = 32;
}

/*virtual*/ MazeWorm::~MazeWorm()
{
}

/*static*/ MazeWorm* MazeWorm::Create()
{
    return new MazeWorm();
}

/*virtual*/ PlanarObject* MazeWorm::CreateNew() const
{
    return new MazeWorm();
}

/*virtual*/ void MazeWorm::AdvanceBegin()
{
    PlanarObject::AdvanceBegin();

    double speed = this->velocity.Magnitude();
    if(speed < 200.0)
    {
        speed = Random::Number(200.0, 250.0);
        this->velocity = this->velocity.Normalized() * speed;
    }

    this->positionFifo.push_back(this->position);
    while(this->positionFifo.size() > this->maxPositionFifoSize)
        this->positionFifo.erase(this->positionFifo.begin());
}

/*virtual*/ void MazeWorm::Render(DrawHelper& drawHelper, double transitionAlpha) const
{
    Transform renderTransform;
    this->CalcRenderTransform(renderTransform, transitionAlpha);

    Vector2D renderPosition = renderTransform.TransformPoint(this->position);

    drawHelper.DrawCircle(renderPosition, this->radius, this->color);

    for (int i = 0; i < (signed)this->positionFifo.size(); i++)
    {
        const Vector2D& bodyPosition = this->positionFifo[i];
        double scale = double(i) / double(this->positionFifo.size());
        Vector2D renderBodyPosition = renderTransform.TransformPoint(bodyPosition);
        drawHelper.DrawCircle(renderBodyPosition, this->radius * scale, this->color);
    }
}

/*virtual*/ void MazeWorm::CollisionOccurredWith(PlanarObject* planarObject, Engine* engine)
{
    auto mazeBlock = dynamic_cast<GoodMazeBlock*>(planarObject);
    if(mazeBlock)
        mazeBlock->SetTouched(false);
}

/*virtual*/ PlanarPhysics::Vector2D MazeWorm::GetPosition() const
{
    return this->position;
}