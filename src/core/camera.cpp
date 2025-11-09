#include <cmath>
#include <bitloop/core/camera.h>
#include <bitloop/core/scene.h>
#include <bitloop/core/viewport.h>
#include <bitloop/imguix/imgui_custom.h>
#include <bitloop/platform/platform.h>

BL_BEGIN_NS

DVec2 CameraInfo::originWorldOffset() const
{
    return originPixelOffset() / (f64)(zoom_128 * surface->initialSizeScale());
}

bool CameraInfo::operator ==(const CameraInfo& rhs) const
{
    if (pos_128 != rhs.pos_128) return false;
    if (stretch_64 != rhs.stretch_64) return false;
    if (zoom_128 != rhs.zoom_128) return false;
    if (rotation_64 != rhs.rotation_64) return false;
    if (cam_pan != rhs.cam_pan) return false;
    if (ref_zoom != rhs.ref_zoom) return false;
    if (viewport_anchor != rhs.viewport_anchor) return false;
    if (init_pos != rhs.init_pos) return false;
    if (init_zoom != rhs.init_zoom) return false;
    if (init_stretch != rhs.init_stretch) return false;
    if (init_rotation != rhs.init_rotation) return false;
    return true;
}

CameraInfo& CameraInfo::operator =(const CameraInfo& rhs)
{
    if (*this != rhs)
    {
        pos_128 = rhs.pos_128;
        zoom_128 = rhs.zoom_128;
        stretch_64 = rhs.stretch_64;
        rotation_64 = rhs.rotation_64;
        cam_pan = rhs.cam_pan;
        ref_zoom = rhs.ref_zoom;
        viewport_anchor = rhs.viewport_anchor;
        init_pos = rhs.init_pos;
        init_zoom = rhs.init_zoom;
        init_stretch = rhs.init_stretch;
        init_rotation = rhs.init_rotation;

        posDirty();
        zoomDirty();
    }
    return *this;
}

const WorldStageTransform& CameraInfo::getTransform() const
{
    // todo: Make thread safe? If called from multiple threads, is it safe?
    assert(surface != nullptr);

    if (surface->resized())
        is_dirty = true; // Force update if surface size changes

    if (!is_dirty) return *t;
    if (!t) t = new WorldStageTransform();

    DDVec2 origin_offset = originPixelOffset();
    DDVec2 adjusted_zoom = zoom_xy * surface->initialSizeScale();

    t->m128 = glm::ddmat3(1.0);
    t->m128 *= glm_ddtranslate(origin_offset.x + pan_x, origin_offset.y + pan_y);
    t->m128 *= glm_ddrotate(rotation_64);
    t->m128 *= glm_ddtranslate(-x_128 * adjusted_zoom.x, -y_128 * adjusted_zoom.y);
    t->m128 *= glm_ddscale(adjusted_zoom.x, adjusted_zoom.y);

    t->updateCache();
    is_dirty = false;

    return *t;
}

void CameraInfo::setOriginViewportAnchor(double ax, double ay)
{
    viewport_anchor = { ax, ay };
    dirty();
}

DVec2 bl::CameraInfo::viewportStageSize() const
{
    return surface->size();
}

void CameraInfo::setOriginViewportAnchor(Anchor anchor)
{
    switch (anchor)
    {
        case Anchor::TOP_LEFT:     setOriginViewportAnchor(0.0, 0.0);  break;
        case Anchor::TOP:          setOriginViewportAnchor(0.5, 0.0);  break;
        case Anchor::TOP_RIGHT:    setOriginViewportAnchor(1.0, 0.0);  break;
        case Anchor::LEFT:         setOriginViewportAnchor(0.0, 0.5);  break;
        case Anchor::CENTER:       setOriginViewportAnchor(0.5, 0.5);  break;
        case Anchor::RIGHT:        setOriginViewportAnchor(1.0, 0.5);  break;
        case Anchor::BOTTOM_LEFT:  setOriginViewportAnchor(0.0, 1.0);  break;
        case Anchor::BOTTOM:       setOriginViewportAnchor(0.5, 1.0);  break;
        case Anchor::BOTTOM_RIGHT: setOriginViewportAnchor(1.0, 1.0);  break;
    }
}

DVec2 CameraInfo::originPixelOffset() const
{
    double viewport_cx = (surface->width() / 2.0);
    double viewport_cy = (surface->height() / 2.0);

    return Vec2(
        viewport_cx + surface->width() * (viewport_anchor.x - 0.5),
        viewport_cy + surface->height() * (viewport_anchor.y - 0.5)
    );
}

// Scale world to fit viewport rect
void CameraInfo::cameraToViewport(
    double left,
    double top,
    double right,
    double bottom)
{
    double world_w = (right - left);
    double world_h = (bottom - top);

    rotation_64 = 0;
    zoom_x = surface->width() / world_w;
    zoom_y = surface->height() / world_h;
    x_128  = (surface->width() / 2) / zoom_x;
    y_128  = (surface->height() / 2) / zoom_y;
}

void CameraInfo::focusWorldRect(
    f128 left, f128 top,
    f128 right, f128 bottom,
    bool stretch)
{
    f128 world_w = right - left;
    f128 world_h = bottom - top;
    rotation_64 = 0;
    pan_x = 0;
    pan_y = 0;

    if (stretch)
    {
        f128 zx = surface->width() / world_w;
        f128 zy = surface->height() / world_h;
        zoom_128 = DDVec2{ zx, zy }.mag();
        sx_64 = (double)(zx / zoom_128);
        sy_64 = (double)(zy / zoom_128);

        DVec2 _originWorldOffset = originWorldOffset();
        x_128 = left + _originWorldOffset.x;
        y_128 = top  + _originWorldOffset.y;
    }
    else
    {
        f128 aspect_view = f128(surface->width() / surface->height());
        f128 aspect_rect = world_w / world_h;

        // Not re-stretching, but preserve existing stretch aspect ratio?
        stretch_64 = { 1, 1 };

        if (aspect_rect > aspect_view)
        {
            // Shrink Height, gap top and bottom
            zoom_128 = (surface->width() / world_w);

            assert(zoom_128 > 0.0);

            DVec2 _originWorldOffset = originWorldOffset();

            f128 world_ox{ 0 };
            f128 world_oy = ((surface->height() - (world_h * zoom_128)) / 2) / zoom_128;

            x_128 = _originWorldOffset.x + left - world_ox;
            y_128 = _originWorldOffset.y + top - world_oy;
        }
        else
        {
            // Shrink Width, gap left and right
            zoom_128 = (surface->height() / world_h);

            assert(zoom_128 > 0.0);

            DVec2 _originWorldOffset = originWorldOffset();

            f128 world_ox = ((surface->width() - (world_w * zoom_128)) / 2) / zoom_128;
            f128 world_oy{ 0 };

            x_128 = _originWorldOffset.x + left - world_ox;
            y_128 = _originWorldOffset.y + top - world_oy;
        }
    }

    ref_zoom = zoom_128;

    posDirty();
    zoomDirty();
}

void CameraInfo::originToCenterViewport()
{
    setPos(0, 0);
}

void CameraInfo::populateUI(DRect restrict_world_rect)
{
    bool changed = false;

    float required_space = 0.0f;
    ImGui::IncreaseRequiredSpaceForLabel(required_space, "Zoom X/Y", scale_size(20.0f));

    int decimals = getPositionDecimals();
    char format[16];
    snprintf(format, sizeof(format), "%%.%df", decimals);


    ImGui::SetNextItemWidthForSpace(required_space);
    if (ImGui::RevertableDragFloat128("X", &x_128, &init_pos.x, 1.0 / zoom_128, restrict_world_rect.x1, restrict_world_rect.x2, format))
        changed = true;
    ImGui::SetNextItemWidthForSpace(required_space);
    if (ImGui::RevertableDragFloat128("Y", &y_128, &init_pos.y, 1.0 / zoom_128, restrict_world_rect.y1, restrict_world_rect.y2, format))
        changed = true;

    ImGui::SetNextItemWidthForSpace(required_space);
    if (ImGui::RevertableSliderAngle("Rotation", &rotation_64, &init_rotation))
        changed = true;

    f128 relative_zoom = relativeZoom<f128>();
    f128 zoom_speed = relative_zoom / 100.0;
    ImGui::SetNextItemWidthForSpace(required_space);
    if (ImGui::RevertableDragFloat128("Zoom", &relative_zoom, &init_zoom, zoom_speed, 0.1, 1e32, "%.5f"))
    {
        zoom_speed = relative_zoom / 100.0;
        zoom_128 = relative_zoom * getReferenceZoom<f128>();
        changed = true;
    }

    ImGui::SetNextItemWidthForSpace(required_space);
    if (ImGui::RevertableSliderDouble2("Zoom X/Y", stretch_64.data(), init_stretch.data(), 0.1, 10.0, "%.2fx"))
        changed = true;

    if (changed)
    {
        zoomDirty();
        posDirty();
    }

    return;
}

void CameraNavigator::restrictRelativeZoomRange(double min, double max)
{
    min_zoom = min;
    max_zoom = max;
    panZoomProcess();
}

void CameraNavigator::panBegin(int _x, int _y, double touch_dist, double touch_angle)
{
    //blPrint("[DOWN] Cam Pos: (%.3f, %.3f) Zoom: (%.3f, %.3f) Angle: %.3f",
    //    (double)x_128, (double)y_128, (double)zoom_x, (double)zoom_y, cam_rotation);

    pan_down_touch_x = _x;
    pan_down_touch_y = _y;
    pan_down_touch_dist = touch_dist;
    pan_down_touch_angle = touch_angle;

    if (direct_cam_panning)
    {
        //DVec2 world_mouse = toWorld(x, y);
        pan_beg_cam_x = camera->x<f128>();
        pan_beg_cam_y = camera->y<f128>();
        pan_beg_cam_zoom = camera->zoom<f128>();
        ///pan_beg_cam_zoom_x = zoom_x;
        ///pan_beg_cam_zoom_y = zoom_y;
        pan_beg_cam_angle = camera->rotation();

        //last_pan_snapped_cam_grid_pos = DDVec2(x_128 * zoom_x, y_128 * zoom_y).snapped(f128(cam_pos_stage_snap_size));
    }
    else
    {
        pan_beg_cam_x = camera->panX();
        pan_beg_cam_y = camera->panY();
    }
    panning = true;
}

bool CameraNavigator::panDrag(int _x, int _y, double touch_dist, double touch_angle)
{
    //blPrint("[DRAG] Cam Pos: (%.3f, %.3f) Zoom: (%.3f, %.3f) Angle: %.3f",
    //    (double)x, (double)y, (double)zoom_x, (double)zoom_y, cam_rotation);

    bool changed = false;

    if (panning)
    {
        if (direct_cam_panning)
        {
            //DVec2 world_mouse = toWorld(_x, _y);
            //double dx = world_mouse.x - pan_down_touch_x;
            //double dy = world_mouse.y - pan_down_touch_y;
            int dx = _x - pan_down_touch_x;
            int dy = _y - pan_down_touch_y;


            DDVec2 world_offset = camera->getTransform().toWorldOffset<f128>(dx, dy);
            //blPrint("(dx,dy) = (%.3f, %.3f)", world_offset.x, world_offset.y);
            //blPrint() << "(dx,dy) = (" << bl::dp(3) << world_offset.x << ", " << world_offset.)y

            f128 new_cam_world_x = pan_beg_cam_x - world_offset.x;
            f128 new_cam_world_y = pan_beg_cam_y - world_offset.y;

            changed |= camera->setPos(
                new_cam_world_x,
                new_cam_world_y
            );

            /*DDVec2 potential_snapped_cam_grid_pos = DDVec2(new_cam_world_x * zoom_x, new_cam_world_y * zoom_y).snapped(cam_pos_stage_snap_size);

            if (potential_snapped_cam_grid_pos != last_pan_snapped_cam_grid_pos)
            {
                changed |= setPos(
                    potential_snapped_cam_grid_pos.x / zoom_x,
                    potential_snapped_cam_grid_pos.y / zoom_y
                );
            }
            
            if (changed)
            {
                last_pan_snapped_cam_grid_pos = potential_snapped_cam_grid_pos;
            }*/

            if (pan_down_touch_dist > 0.0)
            {
                double delta_zoom = touch_dist / pan_down_touch_dist;
                changed |= camera->setZoom(pan_beg_cam_zoom * delta_zoom);
            }

            if (fingers.size() >= 2)
            {
                double delta_rotation = Math::closestAngleDifference(pan_down_touch_angle, touch_angle);
                changed |= camera->setRotation(pan_beg_cam_angle + delta_rotation);
                //blPrint() << "Setting rotation: " << bl::dp(3) << (double)cam_rotation;

                // todo: In order to lock camera during pan, you may need to manually
                //       send an event mimicking a mouse move in order to trigger pollEvents

                //project_worker()->queueEvent()
            }
        }
        else
        {
            int dx = _x - pan_down_touch_x;
            int dy = _y - pan_down_touch_y;

            changed |= camera->setPan(
                (double)(pan_beg_cam_x + (dx/*/ zoom_x*/) /* * pan_mult*/),
                (double)(pan_beg_cam_y + (dy/*/ zoom_y*/) /* * pan_mult*/)
            );
        }
    }

    return changed;
}

bool CameraNavigator::panZoomProcess()
{
    bool changed = false;
    if (camera->zoom<f128>() < camera->getReferenceZoom<f128>() * min_zoom)
        changed |= camera->setZoom(camera->getReferenceZoom<f128>() * min_zoom);
    if (camera->zoom<f128>() > camera->getReferenceZoom<f128>() * max_zoom)
        changed |= camera->setZoom(camera->getReferenceZoom<f128>() * max_zoom);

    if (fingers.size() == 1)
    {
        changed |= panDrag((int)fingers[0].x, (int)fingers[0].y, 0.0, 0.0);
    }
    else if (fingers.size() == 2)
    {
        double avg_x = (fingers[0].x + fingers[1].x) / 2.0;
        double avg_y = (fingers[0].y + fingers[1].y) / 2.0;
        changed |= panDrag((int)avg_x, (int)avg_y, touchDist(), touchAngle());
    }

    return changed;
}

void CameraNavigator::panEnd()
{
    ///panDrag(_x, _y, 0, 0);
    panning = false;
}

bool CameraNavigator::handleWorldNavigation(Event event, bool single_touch_pan, bool zoom_anchor_mouse)
{
    Viewport* viewport = event.ctx_focused();
    if (!viewport) return false;

    if (!viewport->mountedScene()->ownsEvent(event) || !event.isPointerEvent())
        return false;

    PointerEvent e(event);

    if (platform()->is_mobile())
    {
        // Support both single-finger pan & 2 finger transform
        switch (e.type())
        {
        case SDL_EVENT_FINGER_DOWN:
        {
            if (fingers.size() >= 2)
            {
                // Ignore 3 or more fingers
                return false;
            }

            // Add pressed finger
            FingerInfo info;
            info.fingerId = e.fingerID();
            info.x = e.x();
            info.y = e.y();
            fingers.push_back(info);

            if (fingers.size() == 1)
            {
                if (single_touch_pan)
                    panBegin((int)e.x(), (int)e.y(), 0.0, 0.0);
            }
            else if (fingers.size() == 2)
            {
                if (panning)
                {
                    // Was already panning with a single finger. Restart with 2 fingers
                    panEnd();
                }

                double avg_x = (fingers[0].x + fingers[1].x) / 2.0;
                double avg_y = (fingers[0].y + fingers[1].y) / 2.0;
                panBegin((int)avg_x, (int)avg_y, touchDist(), touchAngle());
            }

            return true;
        }
        break;
        case SDL_EVENT_FINGER_UP:
        {
            if (fingers.size() > 2)
            {
                // Ignore 3 or more fingers
                return false;
            }

            double avg_x = 0.0;
            double avg_y = 0.0;
            if (fingers.size() == 2)
            {
                avg_x = (fingers[0].x + fingers[1].x) / 2.0;
                avg_y = (fingers[0].y + fingers[1].y) / 2.0;
            }

            // Erase lifted finger
            std::erase_if(fingers, [&](const FingerInfo& f) { return f.fingerId == e.fingerID(); });

            if (fingers.size() == 0)
            {
                // End single-finger pan (previously had 1 remaining pressed finger)
                if (single_touch_pan)
                    panEnd();
            }
            else if (fingers.size() == 1)
            {
                // End 2-finger pan, switch to single-finger pan
                panEnd();

                // Switch to whichever finger is still pressed (might not be finger index 0)
                if (single_touch_pan)
                    panBegin((int)fingers.front().x, (int)fingers.front().y, 0.0, 0.0);
            }

            return false;
        }
        break;
        case SDL_EVENT_FINGER_MOTION:
        {
            if (fingers.size() > 2)
            {
                // Ignore 3 or more fingers
                return false;
            }

            // Update finger info
            for (size_t i = 0; i < fingers.size(); i++)
            {
                FingerInfo& info = fingers[i];
                if (info.fingerId == e.fingerID())
                {
                    info.x = e.x();
                    info.y = e.y();
                    break;
                }
            }

            return panZoomProcess();

            ///if (fingers.size() == 1)
            ///{
            ///    panDrag((int)e.x(), (int)e.y(), 0.0, 0.0);
            ///}
            ///else if (fingers.size() == 2)
            ///{
            ///    double avg_x = (fingers[0].x + fingers[1].x) / 2.0;
            ///    double avg_y = (fingers[0].y + fingers[1].y) / 2.0;
            ///    panDrag((int)avg_x, (int)avg_y, touchDist(), touchAngle());
            ///}
        }
        break;
        }
    }
    else
    {
        switch (e.type())
        {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            if (fingers.size() == 0)
            {
                FingerInfo info;
                info.fingerId = 0;
                info.x = e.x();
                info.y = e.y();
                fingers.push_back(info);
            }

            if (!panning &&
                e.button() == SDL_BUTTON_MIDDLE ||
                (single_touch_pan && e.button() == SDL_BUTTON_LEFT))
            {
                panBegin((int)e.x(), (int)e.y(), 0.0, 0.0);
            }
        }
        break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            fingers.clear();
            if (e.button() == SDL_BUTTON_MIDDLE ||
                (single_touch_pan && e.button() == SDL_BUTTON_LEFT))
            {
                panEnd();
            }
        }
        break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            if (fingers.size() > 0)
            {
                FingerInfo& info = fingers[0];
                info.x = e.x();
                info.y = e.y();
            }

            return panZoomProcess();
            //if (panning)
            //    panDrag((int)e.x(), (int)e.y(), 0.0, 0.0);
        }
        break;
        case SDL_EVENT_MOUSE_WHEEL:
        {
            UNUSED(zoom_anchor_mouse);
            //if (zoom_anchor_mouse)
            //{
            //    // Zoom towards mouse position
            //    DVec2 before_zoom = toWorld(e.x(), e.y());
            //    setZoomX(zoomX() + (e.wheelY() / 10.0) * zoomX());
            //    setZoomY(zoomY() + (e.wheelY() / 10.0) * zoomY());
            //    DVec2 after_zoom = toWorld(e.x(), e.y());
            //    setPos(x() + (before_zoom.x - after_zoom.x), y() + (before_zoom.y - after_zoom.y));
            //    return panZoomProcess();
            //}
            //else
            {
                camera->setZoom(camera->zoom() + (e.wheelY()/10.0) * camera->zoom());
                panZoomProcess();
                return true;
            }
        }
        break;
        }
    }
    return false;
}

void CameraNavigator::debugPrint(Viewport* ctx) const
{
    double pinch_zoom_factor = touchDist() / panDownTouchDist();

    //setFont(getDefaultFont());
    ctx->print() << "cam.position          = (" << camera->x() << ",  " << camera->y() << ")\n";
    ctx->print() << "cam.pan               = (" << camera->panX() << ",  " << camera->panY() << ")\n";
    ctx->print() << "cam.rotation          = " << camera->rotation() << "\n";
    ctx->print() << "cam.zoom              = (" << camera->zoomX() << ", " << camera->zoomY() << ")\n\n";
    ctx->print() << "--------------------------------------------------------------\n";
    ctx->print() << "pan_down_touch_x      = " << panDownTouchX() << "\n";
    ctx->print() << "pan_down_touch_y      = " << panDownTouchY() << "\n";
    ctx->print() << "pan_down_touch_dist   = " << panDownTouchDist() << "\n";
    ctx->print() << "pan_down_touch_angle  = " << panDownTouchAngle() << "\n";
    ctx->print() << "--------------------------------------------------------------\n";
    ctx->print() << "pan_beg_cam_x         = " << (double)panBegCamX() << "\n";
    ctx->print() << "pan_beg_cam_y         = " << (double)panBegCamY() << "\n";
    ctx->print() << "pan_beg_cam_zoom_x    = " << (double)panBegCamZoom() << "\n";
    ctx->print() << "pan_beg_cam_angle     = " << panBegCamAngle() << "\n";
    ctx->print() << "--------------------------------------------------------------\n";
    ctx->print() << "cam.touchAngle()      = " << touchAngle() << "\n";
    ctx->print() << "cam.touchDist()       = " << touchDist() << "\n";
    ctx->print() << "--------------------------------------------------------------\n";
    ctx->print() << "pinch_zoom_factor     = " << pinch_zoom_factor << "x\n";
    ctx->print() << "--------------------------------------------------------------\n";

    int i = 0;
    auto fingers = pressedFingers();
    for (auto finger : fingers)
    {
        ctx->print() << "Finger " << i << ": [id: " << finger.fingerId << "] - (" << finger.x << ", " << finger.y << ")\n";
        i++;
    }
}




BL_END_NS
