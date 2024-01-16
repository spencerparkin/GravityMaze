#include "DrawHelper.h"
#include "Color.h"

using namespace PlanarPhysics;

//----------------------------- DrawHelper -----------------------------

DrawHelper::DrawHelper()
{
    this->newFrame = nullptr;
    pthread_mutex_init(&this->frameArrayMutex, nullptr);
}

/*virtual*/ DrawHelper::~DrawHelper()
{
    pthread_mutex_destroy(&this->frameArrayMutex);
}

bool DrawHelper::Setup(AAssetManager* assetManager)
{
    if(!this->lineShader.Load("lineFragmentShader.txt", "lineVertexShader.txt", assetManager))
        return false;

    return true;
}

bool DrawHelper::Shutdown()
{
    for(Frame* frame : this->frameArray)
        delete frame;

    this->frameArray.clear();
    this->lineShader.Clear();

    return true;
}

void DrawHelper::BeginRender(PlanarPhysics::Engine* engine, double aspectRatio)
{
    if(this->newFrame)
        return;

    this->newFrame = new Frame();

    const BoundingBox& worldBox = engine->GetWorldBox();
    BoundingBox viewBox(worldBox);
    double marginSize = 50.0;
    viewBox.min.x -= marginSize;
    viewBox.max.x += marginSize;
    viewBox.min.y -= marginSize;
    viewBox.max.y += marginSize;
    viewBox.MatchAspectRatio(aspectRatio, BoundingBox::MatchMethod::EXPAND);
    newFrame->SetOrthographicProjection(viewBox.min.x, viewBox.max.x, viewBox.min.y, viewBox.max.y, -1.0, 1.0);

    newFrame->lineVertexBuffer.clear();
}

void DrawHelper::EndRender()
{
    if(!this->newFrame)
        return;

    pthread_mutex_lock(&this->frameArrayMutex);
    this->frameArray.push_back(newFrame);
    pthread_mutex_unlock(&this->frameArrayMutex);

    this->newFrame = nullptr;
}

void DrawHelper::DrawLine(const PlanarPhysics::Vector2D& pointA, const PlanarPhysics::Vector2D& pointB, const Color& color)
{
    if(!this->newFrame)
        return;

    this->newFrame->lineVertexBuffer.push_back(Vertex{(GLfloat)pointA.x, (GLfloat)pointA.y, (GLfloat)color.r, (GLfloat)color.g, (GLfloat)color.b});
    this->newFrame->lineVertexBuffer.push_back(Vertex{(GLfloat)pointB.x, (GLfloat)pointB.y, (GLfloat)color.r, (GLfloat)color.g, (GLfloat)color.b});
}

void DrawHelper::DrawCircle(const PlanarPhysics::Vector2D& center, double radius, const Color& color, int numSegments /*= 32*/)
{
    if(!this->newFrame)
        return;

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

void DrawHelper::Render()
{
    if(this->frameArray.size() == 0)
        return;

    pthread_mutex_lock(&this->frameArrayMutex);

    // Get rid of dropped frames.
    while(this->frameArray.size() > 1)
    {
        std::list<Frame*>::iterator iter = this->frameArray.begin();
        Frame* frame = *iter;
        delete frame;
        this->frameArray.erase(iter);
    }

    // Grab the latest/newest frame.
    Frame* renderFrame = this->frameArray.back();

    pthread_mutex_unlock(&this->frameArrayMutex);

    if(renderFrame->lineVertexBuffer.size() > 0)
    {
        // TODO: Can we get a speed-up here by using a dynamic VBO?
        //       See: https://stackoverflow.com/questions/36000356/why-use-vertex-buffer-objects-for-dynamic-objects

        this->lineShader.Bind();

        GLint location = this->lineShader.GetUniformLocation("projectionMatrix");
        glUniformMatrix4fv(location, 1, false, renderFrame->projectionMatrix);

        uint8_t* vertexBuf = (uint8_t*)renderFrame->lineVertexBuffer.data();

        location = this->lineShader.GetAttributeLocation("localPosition");
        glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), vertexBuf);
        glEnableVertexAttribArray(location);

        location = this->lineShader.GetAttributeLocation("vertexColor");
        glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), vertexBuf + sizeof(GLfloat[2]));
        glEnableVertexAttribArray(location);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDrawArrays(GL_LINES, 0, renderFrame->lineVertexBuffer.size());
    }
}

//----------------------------- DrawHelper::Frame -----------------------------

DrawHelper::Frame::Frame()
{
    ::memset(this->projectionMatrix, 0, sizeof(this->projectionMatrix));
}

/*virtual*/ DrawHelper::Frame::~Frame()
{
}

void DrawHelper::Frame::SetOrthographicProjection(float left, float right, float bottom, float top, float near, float far)
{
    this->projectionMatrix[0] = 2.0f / (right - left);
    this->projectionMatrix[1] = 0.0f;
    this->projectionMatrix[2] = 0.0f;
    this->projectionMatrix[3] = 0.0f;

    this->projectionMatrix[4] = 0.0f;
    this->projectionMatrix[5] = 2.0f / (top - bottom);
    this->projectionMatrix[6] = 0.0f;
    this->projectionMatrix[7] = 0.0f;

    this->projectionMatrix[8] = 0.0f;
    this->projectionMatrix[9] = 0.0f;
    this->projectionMatrix[10] = 2.0 / (far - near);
    this->projectionMatrix[11] = 0.0f;

    this->projectionMatrix[12] = -(right + left) / (right - left);
    this->projectionMatrix[13] = -(top + bottom) / (top - bottom);
    this->projectionMatrix[14] = -(far + near) / (far - near);
    this->projectionMatrix[15] = 1.0f;
}