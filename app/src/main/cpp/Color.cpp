#include "Color.h"

Color::Color()
{
    this->r = 1.0;
    this->g = 1.0;
    this->b = 1.0;
}

Color::Color(const Color& color)
{
    this->r = color.r;
    this->g = color.g;
    this->b = color.b;
}

Color::Color(GLfloat r, GLfloat g, GLfloat b)
{
    this->r = r;
    this->g = g;
    this->b = b;
}

/*virtual*/ Color::~Color()
{
}

void Color::operator=(const Color& color)
{
    this->r = color.r;
    this->g = color.g;
    this->b = color.b;
}

void Color::Mix(const Color& colorA, const Color& colorB)
{
    this->r = colorA.r + colorB.r;
    this->g = colorA.g + colorB.g;
    this->b = colorA.b + colorB.b;

    this->Clamp();
}

void Color::Clamp()
{
    if(this->r < 0.0f)
        this->r = 0.0f;
    else if(this->r > 1.0f)
        this->r = 1.0f;

    if(this->g < 0.0f)
        this->g = 0.0f;
    else if(this->g > 1.0f)
            this->g = 1.0f;

    if(this->b < 0.0f)
        this->b = 0.0f;
    else if(this->b > 1.0f)
            this->b = 1.0f;
}