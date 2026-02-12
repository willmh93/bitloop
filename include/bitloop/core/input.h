#pragma once
#include <SDL3/SDL_mouse.h>

BL_BEGIN_NS

class Viewport;
class ProjectBase;
class CameraNavigator;

enum struct MouseButton
{
    LEFT = SDL_BUTTON_LEFT,
    WHEEL = SDL_BUTTON_LEFT,
    RIGHT = SDL_BUTTON_RIGHT,
    X1 = SDL_BUTTON_X1,
    X2 = SDL_BUTTON_X2,

    _COUNT = X2
};

struct MouseButtonState
{
    bool is_down = false;
    bool clicked = false;

    void clearStaleButtonStates()
    {
        clicked = false;
    }
};

class MouseInfo
{
    friend class ProjectBase;

    MouseButtonState button_states[3];
    MouseButtonState& buttonState(MouseButton btn_type)
    {
        return button_states[(int)btn_type - 1];
    }

    void clearStaleButtonStates()
    {
        for (int i = 0; i < (int)MouseButton::_COUNT; i++)
            button_states[i].clearStaleButtonStates();
    }

public:

    Viewport* viewport = nullptr;

    f64  client_x = 0;
    f64  client_y = 0;
    f64  stage_x  = 0;
    f64  stage_y  = 0;
    f128 world_x  = 0;
    f128 world_y  = 0;
    int  scroll_delta = 0;

    const MouseButtonState& buttonState(MouseButton btn_type) const
    {
        return button_states[(int)btn_type-1];
    }

    bool buttonClicked(MouseButton btn_type) const
    {
        return buttonState(btn_type).clicked;
    }

    bool buttonDown(MouseButton btn_type) const
    {
        return buttonState(btn_type).is_down;
    }
};

class FingerInfo
{
    friend class CameraNavigator;
    friend class ProjectBase;

    Viewport* ctx_owner = nullptr;
    i64 fingerId;
    f64 x, y;
};

BL_END_NS
