#include "UIFont.h"
#include "../DataReader.h"
#include "../system/Renderer.h"

#include <iostream>
#include <algorithm>

const int BUTTON_SPRITE_WIDTH = 18;

enum class ButtonCode
{
    CROSS,
    SQUARE,
    TRIANGLE,
    CIRCLE,
    DPAD_DOWN,
    DPAD_LEFT,
    DPAD_UP,
    DPAD_RIGHT,
    DPAD_UPDOWN,
    DPAD_LEFTRIGHT,
    STICK_RIGHT,
    STICK_LEFT,
    L1,
    L2,
    R1,
    R2,
    START,
    KILL_ICON,
    TEAM_KILL_ICON,
    DEATH_ICON,
    FLAG_ICON,
    VOTE_ICON,
    NEW_CREDIT_PAGE,
    CREDIT_TITLE_LINE,
    CREDIT_END,
    NUM_BUTTON_TEXTURES,
    UNDEFINED
};

struct ButtonCodeMapping
{
    const wchar_t* codeString;
    ButtonCode     code;
};

static const ButtonCodeMapping buttonCodeTable[] = {
    {L"x", ButtonCode::CROSS},
    {L"q", ButtonCode::SQUARE},
    {L"a", ButtonCode::TRIANGLE},
    {L"o", ButtonCode::CIRCLE},
    {L"d", ButtonCode::DPAD_DOWN},
    {L"l", ButtonCode::DPAD_LEFT},
    {L"u", ButtonCode::DPAD_UP},
    {L"r", ButtonCode::DPAD_RIGHT},
    {L"R", ButtonCode::STICK_RIGHT},
    {L"L", ButtonCode::STICK_LEFT},
    {L"1", ButtonCode::L1},
    {L"2", ButtonCode::L2},
    {L"3", ButtonCode::R1},
    {L"4", ButtonCode::R2},
    {L"S", ButtonCode::START},
    {L"SQUARE", ButtonCode::SQUARE},
    {L"CROSS", ButtonCode::CROSS},
    {L"TRIANGLE", ButtonCode::TRIANGLE},
    {L"CIRCLE", ButtonCode::CIRCLE},
    {L"DOWN", ButtonCode::DPAD_DOWN},
    {L"LEFT", ButtonCode::DPAD_LEFT},
    {L"UP", ButtonCode::DPAD_UP},
    {L"RIGHT", ButtonCode::DPAD_RIGHT},
    {L"UPDOWN", ButtonCode::DPAD_UPDOWN},
    {L"LEFTRIGHT", ButtonCode::DPAD_LEFTRIGHT},
    {L"L1", ButtonCode::L1},
    {L"L2", ButtonCode::L2},
    {L"L3", ButtonCode::STICK_LEFT},
    {L"R1", ButtonCode::R1},
    {L"R2", ButtonCode::R2},
    {L"R3", ButtonCode::STICK_RIGHT},
    {L"PAUSE", ButtonCode::START},
    {L"GRENADE", ButtonCode::SQUARE},
    {L"RELOAD", ButtonCode::CROSS},
    {L"USE", ButtonCode::CROSS},
    {L"PREVWEAPON", ButtonCode::TRIANGLE},
    {L"NEXTWEAPON", ButtonCode::CIRCLE},
    {L"LEANLEFT", ButtonCode::DPAD_LEFT},
    {L"LEANRIGHT", ButtonCode::DPAD_RIGHT},
    {L"LEAN", ButtonCode::DPAD_LEFTRIGHT},
    {L"MUTATE", ButtonCode::DPAD_UP},
    {L"TRANSFORM", ButtonCode::DPAD_UP},
    {L"JUMP", ButtonCode::L1},
    {L"CROUCH", ButtonCode::L2},
    {L"FLASHLIGHT", ButtonCode::STICK_LEFT},
    {L"FIRE", ButtonCode::R1},
    {L"SECONDARY", ButtonCode::R2},
    {L"MELEE", ButtonCode::STICK_RIGHT},
    {L"PARASITE", ButtonCode::R1},
    {L"CONTAGION", ButtonCode::R2},
    {L"KILLS", ButtonCode::KILL_ICON},
    {L"DEATHS", ButtonCode::DEATH_ICON},
    {L"TKS", ButtonCode::TEAM_KILL_ICON},
    {L"FLAGS", ButtonCode::FLAG_ICON},
    {L"VOTES", ButtonCode::VOTE_ICON},
    {
        L"NEWPAGE",
        ButtonCode::NEW_CREDIT_PAGE,
    },
    {
        L"TITLE",
        ButtonCode::CREDIT_TITLE_LINE,
    },
    {
        L"CREDITEND",
        ButtonCode::CREDIT_END,
    }};

ButtonCode lookUpButtonCode(const wchar_t* pString, int iStart)
{
    std::wstring codeString;

    wchar_t c = pString[iStart];
    while (c && (c != 0xBB)) {
        // add this character to the string
        codeString += pString[iStart];
        iStart++;
        c = pString[iStart];
    }

    if (codeString.empty()) {
        return ButtonCode::UNDEFINED;
    }

    const int numMappings = sizeof(buttonCodeTable) / sizeof(buttonCodeTable[0]);

    // look for the string in the code table
    for (int i = 0; i < numMappings; i++) {
        if (codeString == buttonCodeTable[i].codeString) {
            return (buttonCodeTable[i].code);
        }
    }

    return ButtonCode::UNDEFINED;
}

bool ui::Font::readFile(uint8_t* fileData, int len)
{
    DataReader reader(fileData, len);
    int        cmapSize = reader.readUInt16();
    int        numGlyphs = reader.readUInt16();
    lineHeight = reader.readUInt16();

    for (int i = 0; i < cmapSize; ++i) {
        Charmap cm;
        cm.character = reader.readUInt16();
        cm.glyph = reader.readUInt16();
        charmaps.push_back(cm);
    }

    for (int i = 0; i < numGlyphs; ++i) {
        Glyph g;
        g.x = reader.readUInt16();
        g.y = reader.readUInt16();
        g.w = reader.readUInt16();
        glyphs.push_back(g);
    }
    return true;
}

int ui::Font::textWidth(const wchar_t* text, int count) const
{
    int bestWidth = 0;
    int width = 0;

    if (text) {
        // Loop until end of string or end of count.
        while (*text && (count > 0)) {
            int c = *text++;

            // Check for embedded color code.
            if ((c & 0xFF00) == 0xFF00) {
                // Skip 2nd character in embedded color code.
                text++;
                count--;
            } else
                // Check for newline.
                if (c == '\n') {
                    bestWidth = std::max(bestWidth, width - 1);
                    width = 0;
                } else if (c == 0xAB) {
                    // If this is a ButtonIcon then add Sprite width
                    ButtonCode buttonCode = lookUpButtonCode(text, 0);

                    if (buttonCode == ButtonCode::CREDIT_TITLE_LINE || buttonCode == ButtonCode::NEW_CREDIT_PAGE || buttonCode == ButtonCode::CREDIT_END) {
                        buttonCode = ButtonCode::UNDEFINED;
                    }

                    if (buttonCode != ButtonCode::UNDEFINED) {
                        width += BUTTON_SPRITE_WIDTH;
                    }

                    // Loop past control code.
                    while (c != 0xBB) {
                        c = *text++;
                    }
                } else {
                    width += glyphs[lookUpCharacter(c)].w + 1;
                }

            count--;
        }

        bestWidth = std::max(bestWidth, width - 1);
    }

    // Return best width.
    return (bestWidth);
}

int ui::Font::textHeight(const wchar_t* text, int count) const
{
    return 1;
}

int ui::Font::lookUpCharacter(int c) const
{
    const int UNDEFINED_CHARACTER = 0x7F;

    if (c < 256) {
        if (!((c < 0x10) || (charmaps[c].character != 0))) {
            if (charmaps[UNDEFINED_CHARACTER].character != 0) {
                c = UNDEFINED_CHARACTER;
            } else {
                c = 'x';
            }
        }
        return charmaps[c].glyph;
    } else {
        int  imax = charmaps.size();
        int  imin = 256;
        bool found = false;

        //Binary search. TODO: Can probably use a built-in
        while (imax >= imin) {
            int i = (imin + imax) / 2;

            if (imax == imin + 1) {
                if (charmaps[i].character == c) {
                    found = true;
                } else {
                    break;
                }
            }

            if (charmaps[i].character == c) {
                found = true;
            } else if (charmaps[i].character > c) {
                imax = i - 1;
            } else {
                imin = i + 1;
            }

            if (found) {
                return (charmaps[i].glyph);
            }
        }

        if (charmaps[UNDEFINED_CHARACTER].character != 0) {
            return (charmaps[UNDEFINED_CHARACTER].glyph);
        } else {
            return (charmaps['x'].glyph);
        }
    }
}

void ui::Font::renderText(Renderer& renderer, const IntRect& pos, int flags, Colour textColor, const wchar_t* text, bool ignoreEmbeddedColor, bool useGradient, float flareAmount) const
{
    Colour colour1 = textColor;
    Colour colour2 = textColor;
    if (useGradient) {
        if ((textColor.r + textColor.g + textColor.b) / 3.0f > 59.0f) {
            colour2.r = 255;
            colour2.g = 255;
            colour2.b = 255;
            colour2.a = textColor.a;
        } else {
            colour1.r = 0;
            colour1.g = 0;
            colour1.b = 0;
            colour1.a = textColor.a;
        }
    }

    // Do the flare thing
    if (flareAmount > 0.0f) {
        int brightnessDelta = (int)(flareAmount * 75);

        colour1.r = (colour1.r + brightnessDelta) > 255 ? 255 : colour1.r + brightnessDelta;
        colour1.g = (colour1.g + brightnessDelta) > 255 ? 255 : colour1.g + brightnessDelta;
        colour1.b = (colour1.b + brightnessDelta) > 255 ? 255 : colour1.b + brightnessDelta;

        colour2.r = (colour2.r + brightnessDelta) > 255 ? 255 : colour2.r + brightnessDelta;
        colour2.g = (colour2.g + brightnessDelta) > 255 ? 255 : colour2.g + brightnessDelta;
        colour2.b = (colour2.b + brightnessDelta) > 255 ? 255 : colour2.b + brightnessDelta;
    }

    float ScaleX = 1;
    float ScaleY = 1;

    if (flags & is_help_text) {
        //renderHelpText( renderer, pos, flags, textColor, text, ignoreEmbeddedColor, useGradient, flareAmount );
        return;
    }

    // Prepare to draw characters.
    int drawFlags = DRAW_USE_ALPHA | DRAW_TEXTURED | DRAW_2D | DRAW_NO_ZBUFFER | DRAW_NO_ZWRITE | DRAW_XBOX_WRITE_A | DRAW_UV_CLAMP | DRAW_CULL_NONE;
    if (flags & blend_additive) {
        drawFlags |= DRAW_BLEND_ADD;
    }
    renderer.drawBegin(Renderer::Primitive::DRAW_TRIANGLES, drawFlags);
    renderer.setTexture(bitmap);

    int height = textHeight(text);

    int tx = pos.left;
    int ty = pos.top;
    // Position start vertically.
    if (flags & v_center) {
        ty += (pos.getHeight() - height + 4) / 2;
    } else if (flags & v_bottom) {
        ty += (pos.getHeight() - height);
    }

    bool wasEllipsisClipped = false;
    if (flags & clip_ellipsis) {
        /*
        const wchar_t* newString = clipEllipsis( pString, Rect );
        if( newString != text )
        {
            text = newString;
            wasEllipsisClipped = true;
        }
            */
    }

    int iStart = 0;
    int iEnd = 0;
    while (text[iStart]) {
        if (text[iStart] == '\n') {
            iEnd = iStart;
        } else {
            iEnd = iStart + 1;
            while (text[iEnd] && (text[iEnd] != '\n')) {
                iEnd++;
            }
        }

        const int width = textWidth(&text[iStart], iEnd - iStart);
        if (flags & h_center) {
            tx = pos.left + (pos.getWidth() - width) / 2;
        } else if (flags & h_right) {
            tx = pos.left + (pos.getWidth() - width);
        } else {
            tx = pos.left;
        }

        // Check for justification when clipping.
        if ((width > pos.getWidth()) || wasEllipsisClipped) {
            if (flags & clip_l_justify) {
                tx = pos.left;
            } else if (flags & clip_r_justify) {
                tx = pos.right - width;
            }
        }

        const int  MaxButtons = 10;
        int        numButtons = 0;
        ButtonCode buttonCodes[MaxButtons];
        float      buttonX[MaxButtons];
        float      buttonY[MaxButtons];

        for (; iStart < iEnd; iStart++) {
            wchar_t c = text[iStart];

            if (c == 0x00AB) {
                // command code
                iStart++;

                if (text[iStart] == 0) {
                    continue;
                }

                ButtonCode buttonCode = lookUpButtonCode(text, iStart);

                if (buttonCode == ButtonCode::CREDIT_TITLE_LINE || buttonCode == ButtonCode::NEW_CREDIT_PAGE || buttonCode == ButtonCode::CREDIT_END) {
                    buttonCode = ButtonCode::UNDEFINED;
                }

                while (c && (c != 0x00BB)) {
                    c = text[++iStart];
                }

                // If we found a button code then render it.
                if (buttonCode != ButtonCode::UNDEFINED) {
                    while (c && (c != 0x00BB)) {
                        c = text[++iStart];
                    }

                    if (numButtons >= MaxButtons) {
                        continue;
                    }

                    buttonCodes[numButtons] = buttonCode;
                    buttonX[numButtons] = (float)(tx);
                    buttonY[numButtons] = (float)(ty);

                    tx += BUTTON_SPRITE_WIDTH;

                    numButtons++;
                }
                continue;
            }

            // Look for an embedded color code.
            else if (c && ((c & 0xFF00) == 0xFF00)) {
                if (ignoreEmbeddedColor) {
                    iStart++;
                } else {
                    colour1.r = (c & 0x00FF);
                    iStart++;
                    c = text[iStart];
                    colour1.g = (c & 0xFF00) >> 8;
                    colour1.b = (c & 0x00FF);
                }
                continue;
            }

            if (c == 0) {
                break;
            }

            //
            // We have a normal character if we've made it this far
            //
            int          ci = lookUpCharacter(c);
            const Glyph& g = glyphs[ci];

            float u0 = (g.x + 0.5f) / bitmap->width;
            float u1 = (g.x + g.w + 0.5f) / bitmap->width;
            float v0 = (g.y + 0.5f) / bitmap->height;
            float v1 = (g.y + lineHeight + 0.5f) / bitmap->height;

            renderer.drawColour(colour2);
            renderer.drawUV(u0, v0);
            renderer.drawVertex(tx, ty, 0.0f);
            renderer.drawUV(u1, v0);
            renderer.drawVertex(tx + g.w, ty, 0.0f);
            renderer.drawColour(colour1);
            renderer.drawUV(u0, v1);
            renderer.drawVertex(tx, ty + lineHeight, 0.0f);

            renderer.drawVertex(tx, ty + lineHeight, 0.0f);
            renderer.drawUV(u1, v1);
            renderer.drawVertex(tx + g.w, ty + lineHeight, 0.0f);
            renderer.drawColour(colour2);
            renderer.drawUV(u1, v0);
            renderer.drawVertex(tx + g.w, ty, 0.0f);

            tx += g.w + 1;
        }

        if (text[iStart] == '\n') {
            ty += lineHeight;
            iStart++;
        }

        renderer.drawEnd();

        if (numButtons > 0 && useGradient) {
            // now draw the buttons
            renderer.drawBegin(Renderer::Primitive::DRAW_SPRITES, DRAW_TEXTURED | DRAW_2D | DRAW_USE_ALPHA | DRAW_NO_ZBUFFER);

            for (int i = 0; i < numButtons; i++) {
                /* TODO
                Bitmap* button = manager->getButtonTexture(buttonCodes[i]);
                renderer.setTexture(button);

                renderer.drawSprite(Vector3(buttonX[i] + 1, buttonY[i] + 1, 0), Vector2(BUTTON_SPRITE_WIDTH, BUTTON_SPRITE_WIDTH), Colour(0, 0, 0, 255));
                renderer.drawSprite(Vector3(buttonX[i], buttonY[i], 0), Vector2(BUTTON_SPRITE_WIDTH, BUTTON_SPRITE_WIDTH), Colour(255, 255, 255));
                */
            }
            renderer.drawEnd();
        }
    }
}
