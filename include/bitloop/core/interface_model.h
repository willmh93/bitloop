#pragma once
#include "var_buffer.h"

BL_BEGIN_NS


// base methods for project/scenes
class InterfaceModel
{
public:

    virtual void init() {}
    virtual void destroy() {}
    virtual void sidebar() {}
    virtual void overlay() {}

    virtual bool isDoubleBuffered() const = 0;

    virtual bool sidebarVisible() const { return true; }
};

// alias for direct interface model (no buffering)
class DirectInterfaceModel :
    public InterfaceModel
{
public:
    bool isDoubleBuffered() const override final
    {
        return false;
    }
};

// double-buffered interface model
template<typename BaseType>
class DoubleBufferedInterfaceModel :
    public InterfaceModel,
    public DoubleBufferedAccessor<BaseType>
{
public:
    DoubleBufferedInterfaceModel(const BaseType* t) : DoubleBufferedAccessor<BaseType>(t)
    {}

    bool isDoubleBuffered() const override final
    {
        return true;
    }
};

BL_END_NS;
