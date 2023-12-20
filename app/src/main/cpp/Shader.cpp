#include "Shader.h"
#include "AndroidOut.h"

Shader::Shader(GLenum type)
{
    this->shader = 0;
    this->type = type;
}

/*virtual*/ Shader::~Shader()
{
    if(this->shader)
        glDeleteShader(this->shader);
}

bool Shader::Load(const char* shaderFile, AAssetManager* assetManager)
{
    bool succeeded = false;

    AAsset* shaderAsset = nullptr;
    this->shader = 0;

    do
    {
        shaderAsset = AAssetManager_open(assetManager, shaderFile, AASSET_MODE_STREAMING);
        if(!shaderAsset)
        {
            aout << "Failed to load: " << shaderFile << std::endl;
            break;
        }

        const GLchar* shaderBuf = (const GLchar*)AAsset_getBuffer(shaderAsset);
        GLint shaderBufSize = AAsset_getLength(shaderAsset);

        this->shader = glCreateShader(this->type);
        if(!this->shader)
            break;

        glShaderSource(this->shader, 1, &shaderBuf, &shaderBufSize);
        glCompileShader(this->shader);

        GLint shaderCompiled = 0;
        glGetShaderiv(this->shader, GL_COMPILE_STATUS, &shaderCompiled);

        if(!shaderCompiled)
        {
            GLint infoLength = 0;
            glGetShaderiv(this->shader, GL_INFO_LOG_LENGTH, &infoLength);
            if(infoLength > 0)
            {
                auto* infoBuf = new GLchar[infoLength];
                glGetShaderInfoLog(this->shader, infoLength, nullptr, infoBuf);
                aout << "Failed to compile shader: " << infoBuf << std::endl;
                delete[] infoBuf;
            }

            break;
        }

        succeeded = true;
    }
    while(false);

    if(shaderAsset)
        AAsset_close(shaderAsset);

    return succeeded;
}