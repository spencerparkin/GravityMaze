#pragma once

#include <android/asset_manager.h>
#include <GLES3/gl3.h>

class Shader
{
    friend class ShaderProgram;

public:
    Shader(GLenum type);
    virtual ~Shader();

    bool Load(const char* shaderFile, AAssetManager* assetManager);

private:
    GLuint shader;
    GLenum type;
};