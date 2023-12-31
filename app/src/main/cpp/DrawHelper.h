#pragma once

#include "Engine.h"
#include "Math/GeometricAlgebra/Vector2D.h"
#include "ShaderProgram.h"
#include <android/asset_manager.h>

class Color;

class DrawHelper
{
public:
    DrawHelper();
    virtual ~DrawHelper();

    bool Setup(AAssetManager* assetManager);
    bool Shutdown();

    void BeginRender(PlanarPhysics::Engine* engine, double aspectRatio);
    void EndRender();

    void DrawLine(const PlanarPhysics::Vector2D& pointA, const PlanarPhysics::Vector2D& pointB, const Color& color);
    void DrawCircle(const PlanarPhysics::Vector2D& center, double radius, const Color& color, int numSegments = 32);

private:
    void SetOrthographicProjection(float left, float right, float bottom, float top, float near, float far);

    struct Vertex
    {
        GLfloat x, y;
        GLfloat r, g, b;
    };

    ShaderProgram lineShader;
    std::vector<Vertex> lineVertexBuffer;
    float projectionMatrix[16];
};
