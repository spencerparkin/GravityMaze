#pragma once

#include "Color.h"

class DrawHelper;

class MazeObject
{
public:
    MazeObject();
    virtual ~MazeObject();

    virtual void Render(DrawHelper& drawHelper);

    Color color;
};