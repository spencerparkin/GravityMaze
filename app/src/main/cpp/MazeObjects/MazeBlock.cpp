#include "MazeBlock.h"
#include "MazeBall.h"
#include "../DrawHelper.h"
#include "Math/Utilities/LineSegment.h"
#include "../Progress.h"

using namespace PlanarPhysics;

//------------------------ MazeBlock ------------------------

MazeBlock::MazeBlock()
{
}

/*virtual*/ MazeBlock::~MazeBlock()
{
}

/*virtual*/ void MazeBlock::Render(DrawHelper& drawHelper, double transitionAlpha) const
{
    Transform renderTransform;
    this->CalcRenderTransform(renderTransform, transitionAlpha);

    const ConvexPolygon& polygon = this->GetWorldPolygon();
    for(int i = 0; i < polygon.GetVertexCount(); i++)
    {
        int j = (i + 1) % polygon.GetVertexCount();

        const Vector2D& vertexA = polygon.GetVertexArray()[i];
        const Vector2D& vertexB = polygon.GetVertexArray()[j];

        LineSegment lineSeg(vertexA, vertexB);
        LineSegment renderSeg = renderTransform.TransformLineSegment(lineSeg);

        drawHelper.DrawLine(renderSeg.vertexA, renderSeg.vertexB, this->color);
    }
}

/*virtual*/ PlanarPhysics::Vector2D MazeBlock::GetPosition() const
{
    return this->position;
}

//------------------------ MazeBlock::GoodMazeBlock ------------------------

GoodMazeBlock::GoodMazeBlock()
{
    this->color = Color(1.0, 0.0, 0.0);
    this->touched = false;
}

/*virtual*/ GoodMazeBlock::~GoodMazeBlock()
{
}

/*static*/ GoodMazeBlock* GoodMazeBlock::Create()
{
    return new GoodMazeBlock();
}

/*virtual*/ PlanarObject* GoodMazeBlock::CreateNew() const
{
    return new GoodMazeBlock();
}

void GoodMazeBlock::SetTouched(bool touched)
{
    this->touched = touched;
    this->color = touched ? Color(0.0, 1.0, 0.0) : Color(1.0, 0.0, 0.0);
}

bool GoodMazeBlock::IsTouched() const
{
    return this->touched;
}

//------------------------ MazeBlock::EvilMazeBlock ------------------------

EvilMazeBlock::EvilMazeBlock()
{
    this->color = Color(0.0, 1.0, 1.0);
}

/*virtual*/ EvilMazeBlock::~EvilMazeBlock()
{
}

/*static*/ EvilMazeBlock* EvilMazeBlock::Create()
{
    return new EvilMazeBlock();
}

/*virtual*/ PlanarObject* EvilMazeBlock::CreateNew() const
{
    return new EvilMazeBlock();
}

/*virtual*/ void EvilMazeBlock::CollisionOccurredWith(PlanarPhysics::PlanarObject* planarObject, PlanarPhysics::Engine* engine)
{
    auto mazeBall = dynamic_cast<MazeBall*>(planarObject);
    if(mazeBall)
    {
        const std::vector<PlanarObject *> &planarObjectArray = engine->GetPlanarObjectArray();
        for (PlanarObject *planarObject: planarObjectArray)
        {
            auto goodMazeBlock = dynamic_cast<GoodMazeBlock *>(planarObject);
            if (goodMazeBlock)
                goodMazeBlock->SetTouched(false);
        }
    }
}