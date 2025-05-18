#include "nano_bitmap.h"
#include "nano_canvas.h"

DQuad CanvasImage::getWorldQuad(Camera* camera)
{
    if (coordinate_type == CoordinateType::WORLD)
        // WORLD Input, use unchanged
        return getQuad();
    else
        // STAGE Input, convert to WORLD
        return camera->toWorldQuad(getQuad());
}
