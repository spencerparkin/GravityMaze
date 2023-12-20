#pragma once

#include <android/asset_manager.h>
#include <GLES3/gl3.h>
#include <unordered_map>

class ShaderProgram
{
public:
    ShaderProgram();
    virtual ~ShaderProgram();

    bool Load(const char* fragShaderFile, const char* vertShaderFile, AAssetManager* assetManager);
    void Clear();
    void Bind();

    GLint GetAttributeLocation(const std::string& attribName);
    GLint GetUniformLocation(const std::string& uniformName);

private:
    GLuint program;

    std::unordered_map<std::string, GLint> attributeMap;
    std::unordered_map<std::string, GLint> uniformMap;
};