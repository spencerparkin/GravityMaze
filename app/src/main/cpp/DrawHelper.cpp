#include "DrawHelper.h"
#include "Color.h"

DrawHelper::DrawHelper()
{
}

/*virtual*/ DrawHelper::~DrawHelper()
{
}

void DrawHelper::BeginRender(PlanarPhysics::Engine* engine)
{
    // TODO: Make sure our shader is bound.  Make sure its uniforms are up-to-date.  Do other stuff.
}

void DrawHelper::EndRender()
{
}

void DrawHelper::DrawLine(const PlanarPhysics::Vector2D& pointA, const PlanarPhysics::Vector2D& pointB, const Color& color)
{
}

void DrawHelper::DrawCircle(const PlanarPhysics::Vector2D& center, double radius, const Color& color)
{
}