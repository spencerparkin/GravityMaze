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

Color::Color(double r, double g, double b)
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
    if(this->r < 0.0)
        this->r = 0.0;
    else if(this->r > 1.0)
        this->r = 1.0;

    if(this->g < 0.0)
        this->g = 0.0;
    else if(this->g > 1.0)
        this->g = 1.0;

    if(this->b < 0.0)
        this->b = 0.0;
    else if(this->b > 1.0)
        this->b = 1.0;
}