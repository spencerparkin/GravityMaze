#pragma once

#include <android/asset_manager.h>
#include <GLES3/gl3.h>

class ShaderProgram
{
public:
    ShaderProgram();
    virtual ~ShaderProgram();

    bool Load(const char* fragShaderFile, const char* vertShaderfile, AAssetManager* assetManager);
    void Clear();
    void Bind();

private:
    GLuint program;

    // TODO: Cache attribute and uniform locations here.
};