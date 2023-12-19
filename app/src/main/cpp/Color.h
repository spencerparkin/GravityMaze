#pragma once

class Color
{
public:
    Color();
    Color(const Color& color);
    Color(double r, double g, double b);
    virtual ~Color();

    void operator=(const Color& color);
    void Mix(const Color& colorA, const Color& colorB);
    void Clamp();

    double r, g, b;
};