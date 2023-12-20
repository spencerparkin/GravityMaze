#include "DrawHelper.h"
#include "Color.h"

DrawHelper::DrawHelper()
{
}

/*virtual*/ DrawHelper::~DrawHelper()
{
}

bool DrawHelper::Setup(AAssetManager* assetManager)
{
    if(!this->lineShader.Load("lineFragmentShader.txt", "lineVertexShader.txt", assetManager))
        return false;

    return true;
}

bool DrawHelper::Shutdown()
{
    return true;
}

void DrawHelper::BeginRender(PlanarPhysics::Engine* engine)
{
    // TODO: Make sure our shader is bound.  Make sure its uniforms are up-to-date.  Do other stuff.
}

void DrawHelper::EndRender()
{
    // TODO: This is where we make a single draw-call for all line to be drawn.
}

void DrawHelper::DrawLine(const PlanarPhysics::Vector2D& pointA, const PlanarPhysics::Vector2D& pointB, const Color& color)
{
    // Maybe just add to a vertex buffer here.
}

void DrawHelper::DrawCircle(const PlanarPhysics::Vector2D& center, double radius, const Color& color)
{
    // Add more to our vertex buffer here.
}