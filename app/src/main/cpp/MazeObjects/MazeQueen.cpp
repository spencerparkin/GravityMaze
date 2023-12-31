#include "MazeQueen.h"
#include "MazeBlock.h"
#include "../DrawHelper.h"

using namespace PlanarPhysics;

MazeQueen::MazeQueen()
{
    this->addedBlocks = false;
    this->alive = true;
    this->numConcentricCircles = 32;
    this->colorRampOffset = 0;

    for(int i = 0; i < this->numConcentricCircles; i++)
    {
        this->colorRamp.push_back(Color(
                double(i) / double(this->numConcentricCircles),
                1.0 - double(i) / double(this->numConcentricCircles),
                double(i) / double(this->numConcentricCircles)
                ));
    }
}

/*virtual*/ MazeQueen::~MazeQueen()
{
}

/*static*/ MazeQueen* MazeQueen::Create()
{
    return new MazeQueen();
}

/*virtual*/ PlanarObject* MazeQueen::CreateNew() const
{
    return new MazeQueen();
}

/*virtual*/ void MazeQueen::Render(DrawHelper& drawHelper, double transitionAlpha) const
{
    Transform renderTransform;
    this->CalcRenderTransform(renderTransform, transitionAlpha);

    Vector2D renderPosition = renderTransform.TransformPoint(this->position);

    for(int i = 0; i < this->numConcentricCircles; i++)
    {
        double scale = double(i) / double(this->numConcentricCircles - 1);
        const Color& renderColor = this->colorRamp[(i + this->colorRampOffset) % this->colorRamp.size()];
        drawHelper.DrawCircle(renderPosition, this->radius * scale, renderColor);
    }

    this->colorRampOffset = (this->colorRampOffset + 1) % this->colorRamp.size();
}

/*virtual*/ PlanarPhysics::Vector2D MazeQueen::GetPosition() const
{
    return this->position;
}

/*virtual*/ void MazeQueen::CollisionOccurredWith(PlanarPhysics::PlanarObject* planarObject, PlanarPhysics::Engine* engine)
{
    auto goodMazeBlock = dynamic_cast<GoodMazeBlock*>(planarObject);
    if(goodMazeBlock)
    {
        if(goodMazeBlock->IsTouched())
            goodMazeBlock->SetTouched(false);
    }

    auto evilMazeBlock = dynamic_cast<EvilMazeBlock*>(planarObject);
    if(evilMazeBlock)
    {
        if(!this->addedBlocks)
        {
            this->addedBlocks = true;

            // TODO: Add more good blocks to the maze?
        }
    }
}