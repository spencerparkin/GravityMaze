#include "DrawHelper.h"
#include "Color.h"

using namespace PlanarPhysics;

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
    this->lineShader.Clear();

    return true;
}

void DrawHelper::BeginRender(PlanarPhysics::Engine* engine)
{
    // The world-box aspect ratio should already match that of the screen.
    const BoundingBox& worldBox = engine->GetWorldBox();

    float worldWidth = worldBox.Width();
    float worldHeight = worldBox.Height();

    this->projectionMatrix[0] = 2.0f / worldWidth;
    this->projectionMatrix[1] = 0.0f;
    this->projectionMatrix[2] = 0.0f;
    this->projectionMatrix[3] = 0.0f;

    this->projectionMatrix[4] = 0.0f;
    this->projectionMatrix[5] = 2.0f / worldHeight;
    this->projectionMatrix[6] = 0.0f;
    this->projectionMatrix[7] = 0.0f;

    this->projectionMatrix[8] = 0.0f;
    this->projectionMatrix[9] = 0.0f;
    this->projectionMatrix[10] = 1.0f;
    this->projectionMatrix[11] = 0.0f;

    this->projectionMatrix[12] = -1.0f;
    this->projectionMatrix[13] = -1.0f;
    this->projectionMatrix[14] = 0.0f;
    this->projectionMatrix[15] = 1.0f;

    this->lineVertexBuffer.clear();
}

void DrawHelper::EndRender()
{
    if(this->lineVertexBuffer.size() > 0)
    {
        this->lineShader.Bind();

        GLint location = this->lineShader.GetUniformLocation("projectionMatrix");
        glUniformMatrix4fv(location, 1, false, this->projectionMatrix);

        uint8_t* vertexBuf = (uint8_t*)this->lineVertexBuffer.data();

        location = this->lineShader.GetAttributeLocation("localPosition");
        glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), vertexBuf);
        glEnableVertexAttribArray(location);

        location = this->lineShader.GetAttributeLocation("vertexColor");
        glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), vertexBuf + sizeof(Vector2D));
        glEnableVertexAttribArray(location);

        glDrawArrays(GL_LINES, 0, this->lineVertexBuffer.size());
    }
}

void DrawHelper::DrawLine(const PlanarPhysics::Vector2D& pointA, const PlanarPhysics::Vector2D& pointB, const Color& color)
{
    this->lineVertexBuffer.push_back(Vertex{pointA, color});
    this->lineVertexBuffer.push_back(Vertex{pointB, color});
}

void DrawHelper::DrawCircle(const PlanarPhysics::Vector2D& center, double radius, const Color& color, int numSegments /*= 32*/)
{
    std::vector<Vector2D> circlePointArray;

    Vector2D xAxis(1.0, 0.0);
    Vector2D yAxis(0.0, 1.0);

    for(int i = 0; i < numSegments; i++)
    {
        double angle = 2.0 * PLNR_PHY_PI * double(i) / double(numSegments);
        Vector2D circlePoint = center + radius * (xAxis * ::cos(angle) + yAxis * ::sin(angle));
        circlePointArray.push_back(circlePoint);
    }

    for(int i = 0; i < (signed)circlePointArray.size(); i++)
    {
        int j = (i + 1) % circlePointArray.size();
        this->DrawLine(circlePointArray[i], circlePointArray[j], color);
    }
}