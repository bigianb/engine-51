#pragma once

#include <string>
#include <vector>

#include "../VectorMath.h"

class Renderer;

namespace ui
{
    class User;
    class Manager;

    class Window
    {
    public:
        enum flags
        {
            WF_VISIBLE = 0x00000001, // Is visible
            WF_STATIC = 0x00000002,  // Is static, should not respond to input
            WF_BORDER = 0x00000004,  // Has Border
            WF_TAB = 0x00000008,     // Is a page of a tabbed dialog

            WF_NO_ACTIVATE = 0x00000010, // Do not activate first control of dialog
            WF_TITLE = 0x00010000,       // Has a title.
            WF_DLG_CENTER = 0x00000020,  // Center Dialog when it is opened

            WF_DISABLED = 0x00000100,  // Is disabled
            WF_SELECTED = 0x00000200,  // Is selected
            WF_HIGHLIGHT = 0x00000400, // Is highlight

            WF_INPUTMODAL = 0x00001000,  // Is input modal, input stops here
            WF_RENDERMODAL = 0x00002000, // Is render modal, rendering stops here

            WF_BUTTON_LEFT = 0x00004000,  // Button needs to be left just.
            WF_BUTTON_RIGHT = 0x00008000, // Button needs to be right just.

            WF_USE_ABSOLUTE = 0x00010000, // Use absolute co-ordinates

            WF_SCALE_XPOS = 0x00020000,  // Scale dialog object X position
            WF_SCALE_XSIZE = 0x00040000, // Scale dialog object X size
            WF_SCALE_YPOS = 0x00080000,  // Scale dialog object Y position
            WF_SCALE_YSIZE = 0x00100000, // Scale dialog object Y size
        };

        Window()
            : id(-1)
        {
        }
        virtual ~Window();

        virtual void destroy();

        virtual bool create(User*          user,
                            Manager*       manager,
                            const IntRect& position,
                            Window*        parent,
                            int            flags);

        void addChild(Window* w) { children.push_back(w); }
        void removeChild(ui::Window* child);

        virtual void render(Renderer& renderer, int x, int y);

        Window* findChildById(int id) const;

        virtual void onUpdate(float deltaTime);

        int  getControlId() const { return id; }
        void setControlId(int id) { this->id = id; }

        bool visible() const { return (flags & WF_VISIBLE) == WF_VISIBLE; }
        bool disabled() const { return (flags & WF_DISABLED) == WF_DISABLED; }
        bool isStatic() const { return (flags & WF_STATIC) == WF_STATIC; }
        bool highlighted() const { return (flags & WF_HIGHLIGHT) == WF_HIGHLIGHT; }
        bool selected() const { return (flags & WF_SELECTED) == WF_SELECTED; }
        bool isRenderModel() const { return (flags & WF_RENDERMODAL) == WF_RENDERMODAL; }

        const IntRect& getPositon() const { return position; }

        Manager* getUIManger() const { return manager; }

        int  getFlags() const { return flags; }
        void setFlag(int flag) { flags |= flag; }
        void clearFlag(int flag) { flags &= ~flag; }

        void setCreatePosition(IntRect p) { createPosition = p; }

        std::vector<Window*> children;

    protected:
        IntRect position;

        std::wstring  label;
        unsigned int labelFlags;
        Colour       labelColor;
    private:
        Manager* manager;
        User*    user;

        Window* parent;

        int flags;

        int     id;
        IntRect createPosition;


        //int                 font;
    };
};
