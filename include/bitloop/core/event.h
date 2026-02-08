#pragma once
#include <SDL3/SDL.h>
#include <string>

#define BL_BEGIN_NS namespace bl {
#define BL_END_NS   }

BL_BEGIN_NS

class ProjectBase;
class SceneBase;
class CameraInfo;
class Viewport;

class PointerEvent;

class Event
{
    friend class ProjectBase;
    friend class CameraInfo;

    friend class PointerEvent;
    friend class KeyEvent;

protected:
    Viewport* _focused_ctx = nullptr;
    Viewport* _hovered_ctx = nullptr;
    Viewport* _owner_ctx = nullptr;

    SDL_Event& sdl_event;

    void setFocusedViewport(Viewport* ctx) { _focused_ctx = ctx; }
    void setHoveredViewport(Viewport* ctx) { _hovered_ctx = ctx; }
    void setOwnerViewport(Viewport* ctx)   { _owner_ctx = ctx; }

public:

    // Pointer = mouse/touch
    [[nodiscard]] bool isPointerEvent() const
    {
        switch (sdl_event.type)
        {
        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_MOTION:

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_WHEEL:
            return true;
        }
        return false;
    }

    [[nodiscard]] bool isFingerEvent() const
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


    Event(SDL_Event& e) : sdl_event(e) {}
    Event(const Event& e) = default;

    [[nodiscard]] auto type() const             { return sdl_event.type; }
    [[nodiscard]] Viewport* ctx_focused() const { return _focused_ctx; }
    [[nodiscard]] Viewport* ctx_hovered() const { return _hovered_ctx; }
    [[nodiscard]] Viewport* ctx_owner()   const { return _owner_ctx; }
    [[nodiscard]] SDL_Event* sdl()        const { return &sdl_event; }
    [[nodiscard]] std::string toString();

    bool ownedBy(const SceneBase* scene);
};

class PointerEvent : public Event
{
public:
    PointerEvent(const Event& e) : Event(e) {}

    // Mouse
    [[nodiscard]] Uint8 button() { return sdl_event.button.button; }
    [[nodiscard]] double wheelY() { return (double)sdl_event.wheel.y; }

    // Touch
    [[nodiscard]] constexpr SDL_FingerID fingerID() { return sdl_event.tfinger.fingerID; }
    [[nodiscard]] double x();
    [[nodiscard]] double y();

    [[nodiscard]] bool hoveredOver(const SceneBase* scene);
};

typedef SDL_Keycode KeyCode;
typedef SDL_Keymod KeyMod;
typedef SDL_Scancode ScanCode;

class KeyEvent : public Event
{
public:
    KeyEvent(const Event& e) : Event(e.sdl_event) {}

    [[nodiscard]] KeyCode keyCode()     { return static_cast<KeyCode>(sdl_event.key.key); }
    [[nodiscard]] ScanCode scanCode()   { return static_cast<ScanCode>(sdl_event.key.scancode); }
    [[nodiscard]] SDL_Keymod keyMod()   { return static_cast<KeyMod>(sdl_event.key.mod); }

    [[nodiscard]] const char* keyName() { return SDL_GetKeyName(keyCode()); }
};

BL_END_NS
