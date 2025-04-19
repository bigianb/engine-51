#include "UIWindow.h"
#include "UIFont.h"

ui::Window::~Window()
{
    destroy();
}

void ui::Window::destroy()
{
    // child destructor will remove it from the child list.
    while(children.size() > 0){
        delete children[0];
    }
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
    labelFlags = Font::h_center | Font::v_center;
    labelColor = COLOR_WHITE;

    if (parent) {
        parent->addChild(this);
    }
    return true;
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

void ui::Window::localToScreen(int& x, int& y) const
{
    x += position.left;
    y += position.top;
    if (parent) {
        parent->localToScreen(x, y);
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

void ui::Window::onUpdate(float deltaTime)
{
    for (Window* child : children) {
        child->onUpdate(deltaTime);
    }
}

void ui::Window::onNotify(ui::Window* sender, int command, void* data)
{
    if (parent){
        parent->onNotify(sender, command, data);
    }
}

void ui::Window::onLBDown(Window* win)
{
    if (parent){
        parent->onPadSelect(win);
    }
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
    flags |= WF_HIGHLIGHT;
}

void ui::Window::onCursorExit()
{
    flags &= ~WF_HIGHLIGHT;
}

void ui::Window::onKeyDown(int key)
{
    if (parent){
        parent->onKeyDown(key);
    }
}

void ui::Window::onKeyUp(int key)
{
    if (parent){
        parent->onKeyUp(key);
    }
}

void ui::Window::onPadNavigate(ui::Window::NavigateDir code, int presses, int repeats, bool wrapX, bool wrapY)
{
    if (parent) {
        parent->onPadNavigate(code, presses, repeats, wrapX, wrapY);
    }
}

void ui::Window::onPadSelect(Window* win)
{
    if (parent) {
        parent->onPadSelect(win);
    }
}

void ui::Window::onPadBack()
{
    if (parent) {
        parent->onPadBack();
    }
}

void ui::Window::onPadDelete()
{
    if (parent) {
        parent->onPadDelete();
    }
}

void ui::Window::onPadHelp()
{
    if (parent) {
        parent->onPadHelp();
    }
}

void ui::Window::onPadActivate()
{
    if (parent) {
        parent->onPadActivate();
    }
}

void ui::Window::onPadShoulder(int direction)
{
    if (parent) {
        parent->onPadShoulder(direction);
    }
}

void ui::Window::onPadShoulder2(int direction)
{
    if (parent) {
        parent->onPadShoulder2(direction);
    }
}
