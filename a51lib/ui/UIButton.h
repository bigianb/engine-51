#pragma once

#include "UIControl.h"
#include "UIManager.h"

namespace ui
{
    class Button : public Control
    {
    public:
        static Window* factory(User* user, Manager* manager, const IntRect& position, Window* parent, int flags)
        {
            Button* control = new Button();
            control->create(user, manager, position, parent, flags);
            return control;
        }

        Button() {}
        ~Button();

        bool create(User*          user,
                    Manager*       manager,
                    const IntRect& position,
                    Window*        parent,
                    int            flags) override;

        void render(Renderer& renderer, int ox = 0, int oy = 0) override;

        void onUpdate(float deltaTime) override;

        // global button color functions
        static void setTextColorNormal(Colour color) { m_TextColorNormal = color; }
        static void setTextColorHightlight(Colour color) { m_TextColorHighlight = color; }
        static void setTextColorDisabled(Colour color) { m_TextColorDisabled = color; }
        static void setTextColorShadow(Colour color) { m_TextColorShadow = color; }

    protected:
        bool  m_useSmallText;
        bool  m_useNativeColor;
        bool  m_bPulseOn;
        bool  m_bPulseUp;
        float m_PulseRate;
        float m_PulseValue;

        int m_iElement;
        int m_BitmapID;
        int m_Data;

        // set once for all buttons
        static Colour m_TextColorNormal;
        static Colour m_TextColorHighlight;
        static Colour m_TextColorDisabled;
        static Colour m_TextColorShadow;
    };
};
