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

bool ShaderProgram::Load(const char* fragShaderFile, const char* vertShaderFile, AAssetManager* assetManager)
{
    bool success = false;

    this->Clear();

    Shader fragShader(GL_FRAGMENT_SHADER);
    Shader vertShader(GL_VERTEX_SHADER);

    do
    {
        if(!fragShader.Load(fragShaderFile, assetManager))
            break;

        if(!vertShader.Load(vertShaderFile, assetManager))
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

        aout << "Dump of attributes..." << std::endl;
        GLint numAttribs = 0;
        glGetProgramiv(this->program, GL_ACTIVE_ATTRIBUTES, &numAttribs);
        for(int i = 0; i < numAttribs; i++)
        {
            GLchar attribNameBuf[128];
            GLsizei attribNameBufLen = 0;
            GLenum attribType;
            GLint attribSize = 0;
            glGetActiveAttrib(this->program, i, sizeof(attribNameBuf), &attribNameBufLen, &attribSize, &attribType, attribNameBuf);
            aout << "Attribute " << i << " is \"" << attribNameBuf << " of type " << GLint(attribType) << " and size " << attribSize << "." << std::endl;
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

GLint ShaderProgram::GetAttributeLocation(const std::string& attribName)
{
    std::unordered_map<std::string, GLint>::iterator iter = this->attributeMap.find(attribName);
    if(iter == this->attributeMap.end())
    {
        GLint location = glGetAttribLocation(this->program, attribName.c_str());
        this->attributeMap.insert(std::pair<std::string, GLint>(attribName, location));
        return location;
    }

    return iter->second;
}

GLint ShaderProgram::GetUniformLocation(const std::string& uniformName)
{
    std::unordered_map<std::string, GLint>::iterator iter = this->uniformMap.find(uniformName);
    if(iter == this->uniformMap.end())
    {
        GLint location = glGetUniformLocation(this->program, uniformName.c_str());
        this->uniformMap.insert(std::pair<std::string, GLint>(uniformName, location));
        return location;
    }

    return iter->second;
}