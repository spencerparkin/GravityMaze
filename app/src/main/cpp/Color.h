#pragma once

#include <GLES3/gl3.h>

class Color
{
public:
    Color();
    Color(const Color& color);
    Color(GLfloat r, GLfloat g, GLfloat b);
    virtual ~Color();

    void operator=(const Color& color);
    void Mix(const Color& colorA, const Color& colorB);
    void Clamp();

    GLfloat r, g, b;
};