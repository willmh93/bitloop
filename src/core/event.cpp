#include "event.h"
#include "platform.h"
#include "project.h"

std::string Event::toString()
{
    std::string focused = _focused_ctx ? std::to_string(_focused_ctx->viewportIndex()) : "_";
    std::string hovered = _hovered_ctx ? std::to_string(_hovered_ctx->viewportIndex()) : "_";
    std::string type;

    char attribs[256];
    
    switch (sdl_event.type) 
    {
    case SDL_KEYDOWN:           type = "SDL_KEYDOWN";          goto key_attribs;
    case SDL_KEYUP:             type = "SDL_KEYUP";            goto key_attribs;
        key_attribs:
        sprintf(attribs, "{%s}", SDL_GetKeyName(sdl_event.key.keysym.sym));

        break;

    case SDL_MOUSEMOTION:       type = "SDL_MOUSEMOTION";     
        sprintf(attribs, "{%d, %d}",
            sdl_event.motion.x,
            sdl_event.motion.y); 

        break;

    case SDL_MOUSEBUTTONDOWN:   type = "SDL_MOUSEBUTTONDOWN";  goto mouse_attribs;
    case SDL_MOUSEBUTTONUP:     type = "SDL_MOUSEBUTTONUP";    goto mouse_attribs;
        mouse_attribs:
        sprintf(attribs, "{%d, %d}", 
            sdl_event.button.x, 
            sdl_event.button.y); 

        break;

    case SDL_FINGERDOWN:        type = "SDL_FINGERDOWN";    goto finger_attribs;
    case SDL_FINGERUP:          type = "SDL_FINGERUP";      goto finger_attribs;
    case SDL_FINGERMOTION:      type = "SDL_FINGERMOTION";
        finger_attribs:
        sprintf(attribs, "{F_%lld, %d, %d}", 
            sdl_event.tfinger.fingerId, 
            (int)(sdl_event.tfinger.x * (float)Platform()->fbo_width()),
            (int)(sdl_event.tfinger.y * (float)Platform()->fbo_height()));

        break;

    case SDL_MULTIGESTURE:      type = "SDL_MULTIGESTURE";     break;

    case SDL_MOUSEWHEEL:        type = "SDL_MOUSEWHEEL";       break;
    case SDL_TEXTINPUT:         type = "SDL_TEXTINPUT";        break;

    default:                    type = "UNKNOWN_EVENT";        break;
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
    switch (sdl_event.type)
    {
    case SDL_FINGERDOWN:
    case SDL_FINGERUP:
    case SDL_FINGERMOTION:
        return (double)(sdl_event.tfinger.x * (float)Platform()->fbo_width());

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        return (double)(sdl_event.button.x);

    case SDL_MOUSEMOTION:
        return (double)(sdl_event.motion.x);
    }
    return 0;
}

double PointerEvent::y()
{
    switch (sdl_event.type)
    {
    case SDL_FINGERDOWN:
    case SDL_FINGERUP:
    case SDL_FINGERMOTION:
        return (double)(sdl_event.tfinger.y * (float)Platform()->fbo_height());

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        return (double)(sdl_event.button.y);

    case SDL_MOUSEMOTION:
        return (double)(sdl_event.motion.y);
    }
    return 0;
}
