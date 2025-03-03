#include "UIWindow.h"

ui::Window::~Window()
{
    destroy();
}

void ui::Window::destroy()
{
    for (Window* child : children) {
        delete child;
    }
    children.clear();
    if (parent) {
        parent->removeChild(this);
        parent = nullptr;
    }
}

void ui::Window::removeChild(ui::Window* child)
{
    for (auto it = children.begin(); it != children.end(); ++it) {
        if (*it == child) {
            children.erase(it);
            return;
        }
    }
}

bool ui::Window::create(ui::User*      user,
                        ui::Manager*   manager,
                        const IntRect& position,
                        ui::Window*    parent,
                        int            flags)
{
    this->user = user;
    this->manager = manager;
    this->parent = parent;
    createPosition = position;
    this->position = position;
    this->flags = flags;

    if (parent) {
        parent->addChild(this);
    }
    return true;
}

void ui::Window::onUpdate(float deltaTime)
{
    for (Window* child : children) {
        child->onUpdate(deltaTime);
    }
}

ui::Window* ui::Window::findChildById(int id) const
{
    for (Window* child : children) {
        if (child->getControlId() == id) {
            return child;
        }
    }
    return nullptr;
}

void ui::Window::render(Renderer& renderer, int x, int y)
{
    if (flags & WF_VISIBLE) {
        for (Window* child : children) {
            child->render(renderer, x, y);
        }
    }
}

void ui::Window::screenToLocal(int& x, int& y) const
{
    x -= position.left;
    y -= position.top;
    if (parent) {
        parent->screenToLocal(x, y);
    }
}

bool ui::Window::isChildOf(Window* parentIn) const
{
    if (parent) {
        if (parent == parentIn) {
            return true;
        } else {
            return parent->isChildOf(parentIn);
        }
    }

    return false;
}

ui::Window* ui::Window::getWindowAtXY(int x, int y)
{
    Window* pFound = nullptr;

    // Don't process for STATIC, DISABLED or INVISIBLE windows
    if (!(flags & WF_STATIC) && !(flags & WF_DISABLED) && (flags & WF_VISIBLE)) {
        // Check if the coordinates hit our rectangle
        if (((x >= 0) && (x < position.getWidth())) &&
            ((y >= 0) && (y < position.getHeight()))) {
            // Loop through all children testing
            for (int i = 0; (i < children.size()) && !pFound; i++) {
                IntRect r = children[i]->getPosition();
                pFound = children[i]->getWindowAtXY(x - r.left, y - r.top);
            }

            // If no child found then return this window
            if (pFound == nullptr) {
                pFound = this;
            }
        }
    }

    return pFound;
}

void ui::Window::onNotify(ui::Window* sender, int command, void* data)
{
}

void ui::Window::onLBDown()
{
}

void ui::Window::onLBUp()
{
}

void ui::Window::onMBDown()
{
}

void ui::Window::onMBUp()
{
}

void ui::Window::onRBDown()
{
}

void ui::Window::onRBUp()
{
}

void ui::Window::onCursorMove(int x, int y)
{
}

void ui::Window::onCursorEnter()
{
}

void ui::Window::onCursorExit()
{
}

void ui::Window::onKeyDown(int key)
{
}

void ui::Window::onKeyUp(int key)
{
}

void ui::Window::onPadNavigate(ui::Window::NavigateDir code, int presses, int repeats, bool wrapX, bool wrapY)
{
}

void ui::Window::onPadSelect()
{
    if (parent) {
        parent->onPadSelect();
    }
}

void ui::Window::onPadBack()
{
}

void ui::Window::onPadDelete()
{
}

void ui::Window::onPadHelp()
{
}

void ui::Window::onPadActivate()
{
}

void ui::Window::onPadShoulder(int direction)
{
}

void ui::Window::onPadShoulder2(int direction)
{
}
