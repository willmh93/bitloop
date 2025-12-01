#include <bitloop/core/event.h>
#include <bitloop/platform/platform.h>
#include <bitloop/core/viewport.h>

BL_BEGIN_NS

std::string Event::toString()
{
    std::string focused = _focused_ctx ? std::to_string(_focused_ctx->viewportIndex()) : "_";
    std::string hovered = _hovered_ctx ? std::to_string(_hovered_ctx->viewportIndex()) : "_";
    std::string type;

    char attribs[256];
    
    switch (sdl_event.type) 
    {
    case SDL_EVENT_KEY_DOWN:           type = "SDL_KEYDOWN";          goto key_attribs;
    case SDL_EVENT_KEY_UP:             type = "SDL_KEYUP";            goto key_attribs;
        key_attribs:
        sprintf(attribs, "{%s}", SDL_GetKeyName(sdl_event.key.key));

        break;

    case SDL_EVENT_MOUSE_MOTION:       type = "SDL_MOUSEMOTION";
        sprintf(attribs, "{%.1f, %.1f}",
            sdl_event.motion.x,
            sdl_event.motion.y); 

        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:  type = "SDL_MOUSEBUTTONDOWN";  goto mouse_attribs;
    case SDL_EVENT_MOUSE_BUTTON_UP:    type = "SDL_MOUSEBUTTONUP";    goto mouse_attribs;
        mouse_attribs:
        sprintf(attribs, "{%.1f, %.1f}", 
            sdl_event.button.x, 
            sdl_event.button.y); 

        break;

    case SDL_EVENT_FINGER_DOWN:        type = "SDL_FINGERDOWN";    goto finger_attribs;
    case SDL_EVENT_FINGER_UP:          type = "SDL_FINGERUP";      goto finger_attribs;
    case SDL_EVENT_FINGER_MOTION:      type = "SDL_FINGERMOTION";
        finger_attribs:
        sprintf(attribs, "{F_%llu, %d, %d}",
            (unsigned long long) sdl_event.tfinger.fingerID,
            (int)(sdl_event.tfinger.x * (float)platform()->fbo_width()),
            (int)(sdl_event.tfinger.y * (float)platform()->fbo_height()));

        break;

    case SDL_EVENT_MOUSE_WHEEL:        type = "SDL_MOUSEWHEEL";       break;
    case SDL_EVENT_TEXT_INPUT:         type = "SDL_TEXTINPUT";        break;

    default:                           type = "UNKNOWN_EVENT";        break;
    }

    std::string info;
    info += "[focused: " + focused + "] ";
    info += "[hovered: " + hovered + "] ";
    info += "[" + type + "] ";
    info += attribs;

    return info;
}

double PointerEvent::x()
{
    double x0 = ctx_owner()->left();

    switch (sdl_event.type)
    {
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_MOTION:
        return (double)(sdl_event.tfinger.x * (float)platform()->fbo_width()) - x0;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return (double)(sdl_event.button.x) - x0;

    case SDL_EVENT_MOUSE_MOTION:
        return (double)(sdl_event.motion.x) - x0;
    }
    return 0;
}

double PointerEvent::y()
{
    double y0 = ctx_owner()->top();

    switch (sdl_event.type)
    {
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_MOTION:
        return (double)(sdl_event.tfinger.y * (float)platform()->fbo_height()) - y0;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return (double)(sdl_event.button.y) - y0;

    case SDL_EVENT_MOUSE_MOTION:
        return (double)(sdl_event.motion.y) - y0;
    }
    return 0;
}

BL_END_NS
