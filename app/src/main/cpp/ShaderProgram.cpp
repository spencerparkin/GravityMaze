#include "AndroidOut.h"
#include "ShaderProgram.h"
#include "Shader.h"

ShaderProgram::ShaderProgram()
{
    this->program = 0;
}

/*virtual*/ ShaderProgram::~ShaderProgram()
{
    this->Clear();
}

bool ShaderProgram::Load(const char* fragShaderFile, const char* vertShaderfile, AAssetManager* assetManager)
{
    bool success = false;

    this->Clear();

    Shader fragShader(GL_FRAGMENT_SHADER);
    Shader vertShader(GL_VERTEX_SHADER);

    do
    {
        if(!fragShader.Load(fragShaderFile, assetManager))
            break;

        if(!vertShader.Load(vertShaderfile, assetManager))
            break;

        this->program = glCreateProgram();
        if(!this->program)
            break;

        glAttachShader(this->program, vertShader.shader);
        glAttachShader(this->program, fragShader.shader);

        glLinkProgram(this->program);

        GLint linkStatus = GL_FALSE;
        glGetProgramiv(this->program, GL_LINK_STATUS, &linkStatus);
        if(linkStatus != GL_TRUE)
        {
            GLint logLength = 0;
            glGetProgramiv(this->program, GL_INFO_LOG_LENGTH, &logLength);
            if(logLength > 0)
            {
                GLchar* logBuf = new GLchar[logLength];
                glGetProgramInfoLog(this->program, logLength, nullptr, logBuf);
                aout << "Failed to link shader program: " << logBuf << std::endl;
                delete[] logBuf;
            }

            break;
        }

        success = true;
    }
    while(false);

    if(!success)
        this->Clear();

    return success;
}

void ShaderProgram::Clear()
{
    if(this->program)
    {
        glDeleteProgram(this->program);
        this->program = 0;
    }
}

void ShaderProgram::Bind()
{
    glUseProgram(this->program);
}