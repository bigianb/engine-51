#pragma once

/* Generic engine. Implement system specific ones. */
class Engine
{
public:
    virtual ~Engine() {}
    virtual void init() = 0;
};
