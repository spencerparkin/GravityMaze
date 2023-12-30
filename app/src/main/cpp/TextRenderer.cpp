#include "TextRenderer.h"
#include "DrawHelper.h"
#include <ctype.h>

using namespace PlanarPhysics;

//--------------------------------- TextRenderer ---------------------------------

TextRenderer::TextRenderer()
{
    this->MakeGlyphs();
}

/*virtual*/ TextRenderer::~TextRenderer()
{
}

void TextRenderer::MakeGlyphs()
{
    this->glyphMap.insert(std::pair<unsigned char, Glyph>('A', {
        {Vector2D(0.0, 0.0), Vector2D(0.0, 1.0), Vector2D(1.0, 1.0), Vector2D(1.0, 0.0)},
        {Vector2D(0.0, 0.5), Vector2D(1.0, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('B', {
        {Vector2D(0.0, 0.0), Vector2D(0.0, 1.0), Vector2D(1.0, 1.0), Vector2D(1.0, 0.0), Vector2D(0.0, 0.0)},
        {Vector2D(0.0, 0.5), Vector2D(1.0, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('C', {
        {Vector2D(1.0, 0.0), Vector2D(0.0, 0.0), Vector2D(0.0, 1.0), Vector2D(1.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('D', {
        {Vector2D(0.0, 0.0), Vector2D(0.0, 1.0), Vector2D(1.0, 0.5), Vector2D(0.0, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('E', {
        {Vector2D(1.0, 0.0), Vector2D(0.0, 0.0), Vector2D(0.0, 1.0), Vector2D(1.0, 1.0)},
        {Vector2D(0.0, 0.5), Vector2D(1.0, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('F', {
        {Vector2D(0.0, 0.0), Vector2D(0.0, 1.0), Vector2D(1.0, 1.0)},
        {Vector2D(0.0, 0.5), Vector2D(1.0, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('G', {
        {Vector2D(1.0, 1.0), Vector2D(0.0, 1.0), Vector2D(0.0, 0.0), Vector2D(1.0, 0.0), Vector2D(1.0, 0.5), Vector2D(0.5, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('H', {
        {Vector2D(0.0, 0.0), Vector2D(0.0, 1.0)},
        {Vector2D(1.0, 0.0), Vector2D(1.0, 1.0)},
        {Vector2D(0.0, 0.5), Vector2D(1.0, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('I', {
        {Vector2D(0.0, 0.0), Vector2D(1.0, 0.0)},
        {Vector2D(0.0, 1.0), Vector2D(1.0, 1.0)},
        {Vector2D(0.5, 0.0), Vector2D(0.5, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('J', {
        {Vector2D(0.0, 0.5), Vector2D(0.0, 0.0), Vector2D(0.5, 0.0), Vector2D(0.5, 1.0)},
        {Vector2D(0.0, 1.0), Vector2D(1.0, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('K', {
        {Vector2D(0.0, 0.0), Vector2D(0.0, 1.0)},
        {Vector2D(1.0, 0.0), Vector2D(0.0, 0.5), Vector2D(1.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('L', {
        {Vector2D(1.0, 0.0), Vector2D(0.0, 0.0), Vector2D(0.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('M', {
        {Vector2D(0.0, 0.0), Vector2D(0.0, 1.0), Vector2D(0.5, 0.5), Vector2D(1.0, 1.0), Vector2D(1.0, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('N', {
        {Vector2D(0.0, 0.0), Vector2D(0.0, 1.0), Vector2D(1.0, 0.0), Vector2D(1.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('O', {
        {Vector2D(0.0, 0.0), Vector2D(1.0, 0.0), Vector2D(1.0, 1.0), Vector2D(0.0, 1.0), Vector2D(0.0, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('P', {
        {Vector2D(0.0, 0.0), Vector2D(0.0, 1.0), Vector2D(1.0, 1.0), Vector2D(1.0, 0.5), Vector2D(0.0, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('Q', {
        {Vector2D(0.0, 0.0), Vector2D(0.0, 1.0), Vector2D(1.0, 1.0), Vector2D(1.0, 0.5), Vector2D(0.5, 0.0)},
        {Vector2D(0.5, 0.5), Vector2D(1.0, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('R', {
        {Vector2D(0.0, 0.0), Vector2D(0.0, 1.0), Vector2D(1.0, 1.0), Vector2D(1.0, 0.5), Vector2D(0.0, 0.5), Vector2D(1.0, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('S', {
        {Vector2D(0.0, 0.0), Vector2D(1.0, 0.0), Vector2D(1.0, 0.5), Vector2D(0.0, 0.5), Vector2D(0.0, 1.0), Vector2D(1.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('T', {
        {Vector2D(0.0, 1.0), Vector2D(1.0, 1.0)},
        {Vector2D(0.5, 1.0), Vector2D(0.5, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('U', {
        {Vector2D(0.0, 1.0), Vector2D(0.0, 0.0), Vector2D(1.0, 0.0), Vector2D(1.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('V', {
        {Vector2D(0.0, 1.0), Vector2D(0.5, 0.0), Vector2D(1.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('W', {
        {Vector2D(0.0, 1.0), Vector2D(0.0, 0.0), Vector2D(0.5, 0.5), Vector2D(1.0, 0.0), Vector2D(1.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('X', {
        {Vector2D(0.0, 0.0), Vector2D(1.0, 1.0)},
        {Vector2D(0.0, 1.0), Vector2D(1.0, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('Y', {
        {Vector2D(0.0, 1.0), Vector2D(0.5, 0.5), Vector2D(1.0, 1.0)},
        {Vector2D(0.5, 0.5), Vector2D(0.5, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('Z', {
        {Vector2D(0.0, 1.0), Vector2D(1.0, 1.0), Vector2D(0.0, 0.0), Vector2D(1.0, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('0', {
        {Vector2D(0.0, 0.0), Vector2D(1.0, 0.0), Vector2D(1.0, 1.0), Vector2D(0.0, 1.0), Vector2D(0.0, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('1', {
        {Vector2D(0.0, 0.0), Vector2D(1.0, 0.0)},
        {Vector2D(0.5, 0.0), Vector2D(0.5, 1.0), Vector2D(0.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('2', {
        {Vector2D(1.0, 0.0), Vector2D(0.0, 0.0), Vector2D(0.0, 0.5), Vector2D(1.0, 0.5), Vector2D(1.0, 1.0), Vector2D(0.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('3', {
        {Vector2D(0.0, 0.0), Vector2D(1.0, 0.0), Vector2D(1.0, 1.0), Vector2D(0.0, 1.0)},
        {Vector2D(0.0, 0.5), Vector2D(1.0, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('4', {
        {Vector2D(1.0, 0.0), Vector2D(1.0, 1.0)},
        {Vector2D(1.0, 0.5), Vector2D(0.0, 0.5), Vector2D(0.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('5', {
        {Vector2D(0.0, 0.0), Vector2D(1.0, 0.0), Vector2D(1.0, 0.5), Vector2D(0.0, 0.5), Vector2D(0.0, 1.0), Vector2D(1.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('6', {
        {Vector2D(1.0, 1.0), Vector2D(0.0, 1.0), Vector2D(0.0, 0.0), Vector2D(1.0, 0.0), Vector2D(1.0, 0.5), Vector2D(0.0, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('7', {
        {Vector2D(1.0, 0.0), Vector2D(1.0, 1.0), Vector2D(0.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('8', {
        {Vector2D(0.0, 0.0), Vector2D(0.0, 1.0), Vector2D(1.0, 1.0), Vector2D(1.0, 0.0), Vector2D(0.0, 0.0)},
        {Vector2D(0.0, 0.5), Vector2D(1.0, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('9', {
        {Vector2D(1.0, 0.0), Vector2D(1.0, 1.0), Vector2D(0.0, 1.0), Vector2D(0.0, 0.5), Vector2D(1.0, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('.', {
        {Vector2D(0.25, 0.0), Vector2D(0.75, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('!', {
        {Vector2D(0.25, 0.0), Vector2D(0.75, 0.0)},
        {Vector2D(0.5, 0.25), Vector2D(0.5, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('?', {
        {Vector2D(0.25, 0.0), Vector2D(0.75, 0.0)},
        {Vector2D(0.5, 0.25), Vector2D(0.5, 0.5), Vector2D(1.0, 1.0), Vector2D(0.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>(',', {
        {Vector2D(0.75, 0.25), Vector2D(0.25, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>(';', {
        {Vector2D(0.75, 0.25), Vector2D(0.25, 0.0)},
        {Vector2D(0.25, 0.5), Vector2D(0.75, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('+', {
        {Vector2D(0.0, 0.5), Vector2D(1.0, 0.5)},
        {Vector2D(0.5, 1.0), Vector2D(0.5, 0.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('-', {
        {Vector2D(0.0, 0.5), Vector2D(1.0, 0.5)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('*', {
        {Vector2D(0.25, 0.25), Vector2D(0.75, 0.75)},
        {Vector2D(0.25, 0.75), Vector2D(0.75, 0.25)},
        {Vector2D(0.25, 0.5), Vector2D(0.75, 0.5)},
        {Vector2D(0.5, 0.25), Vector2D(0.5, 0.75)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('/', {
        {Vector2D(0.0, 0.0), Vector2D(1.0, 1.0)}
    }));

    this->glyphMap.insert(std::pair<unsigned char, Glyph>('=', {
        {Vector2D(0.0, 0.75), Vector2D(1.0, 0.75)},
        {Vector2D(0.0, 0.25), Vector2D(1.0, 0.25)}
    }));

    Transform toCenter;
    toCenter.Identity();
    toCenter.translation = Vector2D(-0.5, -0.5);

    Transform fromCenter;
    fromCenter.Identity();
    fromCenter.translation = Vector2D(0.5, 0.5);

    Transform scale;
    scale.Identity();
    scale.scale = 0.9;

    Transform inset = toCenter * scale * fromCenter;

    for(auto& pair : this->glyphMap)
    {
        Glyph& glyph = pair.second;
        for(Stroke& stroke : glyph)
            for (Vector2D& point: stroke)
                point = inset.TransformPoint(point);
    }
}

void TextRenderer::RenderText(const std::string& text, const PlanarPhysics::Transform& textToWorld, const Color& color, DrawHelper& drawHelper) const
{
    Transform charToText;

    for(int i = 0; text.c_str()[i] != '\0'; i++)
    {
        unsigned char ch = text.c_str()[i];
        if(ch == ' ')
            continue;

        ch = ::toupper(ch);

        charToText.Identity();
        charToText.translation = Vector2D(double(i), 0.0);

        Transform charToWorld = charToText * textToWorld;

        auto iter = this->glyphMap.find(ch);
        if(iter == this->glyphMap.end())
            iter = this->glyphMap.find('?');

        const Glyph& glyph = iter->second;
        for(const Stroke& stroke : glyph)
        {
            for(int j = 0; j < (signed)stroke.size() - 1; j++)
            {
                const Vector2D& pointA = stroke[j];
                const Vector2D& pointB = stroke[j + 1];

                Vector2D worldPointA = charToWorld.TransformPoint(pointA);
                Vector2D worldPointB = charToWorld.TransformPoint(pointB);

                drawHelper.DrawLine(worldPointA, worldPointB, color);
            }
        }
    }
}