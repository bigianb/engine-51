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
    if (parent){
        parent->removeChild(this);
        parent = nullptr;
    }
}

void ui::Window::removeChild(ui::Window* child)
{
    for (auto it = children.begin() ; it != children.end(); ++it){
        if (*it == child){
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
        if (child->getControlId() == id){
            return child;
        }
    }
    return nullptr;
}

void ui::Window::render(Renderer& renderer, int x, int y)
{
    if (flags & WF_VISIBLE){
        for (Window* child : children) {
            child->render(renderer, x, y);
        }
    }
}
