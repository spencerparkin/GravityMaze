#pragma once

#include "Engine.h"
#include "Math/GeometricAlgebra/Vector2D.h"
#include "ShaderProgram.h"
#include <android/asset_manager.h>
#include <pthread.h>

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

    void Render();

private:

    struct Vertex
    {
        GLfloat x, y;
        GLfloat r, g, b;
    };

    ShaderProgram lineShader;

    // Everything needed to draw a single frame should be contained within this structure.
    class Frame
    {
    public:
        Frame();
        virtual ~Frame();

        void SetOrthographicProjection(float left, float right, float bottom, float top, float near, float far);

        std::vector<Vertex> lineVertexBuffer;
        float projectionMatrix[16];
    };

    std::list<Frame*> frameArray;
    Frame* newFrame;
    pthread_mutex_t frameArrayMutex;
};
