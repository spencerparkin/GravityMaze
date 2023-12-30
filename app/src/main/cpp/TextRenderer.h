#pragma once

#include "Math/Utilities/Transform.h"
#include "Color.h"
#include <string>
#include <vector>
#include <map>

class DrawHelper;

class TextRenderer
{
public:
    TextRenderer();
    virtual ~TextRenderer();

    void RenderText(const std::string& text, const PlanarPhysics::Transform& textToWorld, const Color& color, DrawHelper& drawHelper) const;

private:
    void MakeGlyphs();

    typedef std::vector<PlanarPhysics::Vector2D> Stroke;
    typedef std::vector<Stroke> Glyph;
    typedef std::map<unsigned char, Glyph> GlyphMap;

    GlyphMap glyphMap;
};