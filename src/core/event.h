#pragma once
#include <SDL3/SDL.h>
#include <string>

class ProjectBase;
class Camera;
class Viewport;

class PointerEvent;

class Event
{
    friend class ProjectBase;
    friend class Camera;

    friend class PointerEvent;
    friend class KeyEvent;

    Viewport* _focused_ctx = nullptr;
    Viewport* _hovered_ctx = nullptr;
    Viewport* _owner_ctx = nullptr;

protected:
    SDL_Event& sdl_event;

    void setFocusedViewport(Viewport* ctx) { _focused_ctx = ctx; }
    void setHoveredViewport(Viewport* ctx) { _hovered_ctx = ctx; }
    void setOwnerViewport(Viewport* ctx)   { _owner_ctx = ctx; }

    [[nodiscard]] bool isPointerEvent()
    {
        switch (sdl_event.type)
        {
        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_FINGER_MOTION:
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_WHEEL:
            return true;
        }
        return false;
    }

    [[nodiscard]] bool isFingerEvent()
    {
        switch (sdl_event.type)
        {
        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_MOTION:
            return true;
        }
        return false;
    }

public:
    Event(SDL_Event& e) : sdl_event(e) {}

    [[nodiscard]] auto type() const       { return sdl_event.type; }
    [[nodiscard]] Viewport* ctx_focused() { return _focused_ctx; }
    [[nodiscard]] Viewport* ctx_hovered() { return _hovered_ctx; }
    [[nodiscard]] Viewport* ctx_owner()   { return _owner_ctx; }
    [[nodiscard]] SDL_Event* sdl()        { return &sdl_event; }
    [[nodiscard]] std::string toString();
};

class PointerEvent : public Event
{
public:
    PointerEvent(Event& e) : Event(e.sdl_event) {}

    // Mouse
    [[nodiscard]] Uint8 button() { return sdl_event.button.button; }
    [[nodiscard]] double wheelY() { return (double)sdl_event.wheel.y; }

    // Touch
    [[nodiscard]] constexpr SDL_FingerID fingerID() { return sdl_event.tfinger.fingerId; }
    [[nodiscard]] double x();
    [[nodiscard]] double y();
};

typedef SDL_KeyCode KeyCode;
typedef SDL_Scancode ScanCode;

class KeyEvent : public Event
{
public:
    KeyEvent(Event& e) : Event(e.sdl_event) {}

    [[nodiscard]] KeyCode keyCode()     { return static_cast<KeyCode>(sdl_event.key.keysym.sym); }
    [[nodiscard]] ScanCode scanCode()   { return static_cast<ScanCode>(sdl_event.key.keysym.scancode); }
    [[nodiscard]] const char* keyName() { return SDL_GetKeyName(sdl_event.key.keysym.sym); }
};

