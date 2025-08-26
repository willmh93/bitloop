#include <cmath>
#include <core/camera.h>
#include <core/project.h>
#include <core/platform.h>

BL_BEGIN_NS

//void Camera::setTransformFilters(bool _transform_coordinates, bool _scale_line_txt, bool _scale_sizes, bool _rotate_text)
//{
//    transform_coordinates = _transform_coordinates;
//    scale_lines = _scale_line_txt;
//    scale_sizes = _scale_sizes;
//    rotate_text = _rotate_text;
//    viewport->setLineWidth(viewport->line_width);
//}
//
//void Camera::setTransformFilters(bool all)
//{
//    transform_coordinates = all;
//    scale_lines = all;
//    scale_sizes = all;
//    rotate_text = all;
//    viewport->setLineWidth(viewport->line_width);
//}

void Camera::worldTransform()
{
    transform_coordinates = true;
    scale_lines = true;
    scale_sizes = true;
    scale_text  = true;
    rotate_text = true;
    viewport->setLineWidth(viewport->line_width);
}

void Camera::stageTransform()
{
    transform_coordinates = false;
    scale_lines = false;
    scale_sizes = false;// true; // Make sure zoom doesn't affect circle/shape sizes
    scale_text  = false;
    rotate_text = false;
    viewport->setLineWidth(viewport->line_width);
}

void Camera::worldHudTransform()
{
    transform_coordinates = true;
    scale_lines = false;
    scale_sizes = false; //scale_sizes = true; // leave unchanged
    scale_text  = false;
    rotate_text = false;
    viewport->setLineWidth(viewport->line_width);
}

void Camera::saveCameraTransform()
{
    saved_transform_coordinates = transform_coordinates;
    saved_scale_lines = scale_lines;
    saved_scale_sizes = scale_sizes;
    saved_scale_text  = scale_text;
    saved_rotate_text = rotate_text;
}

void Camera::restoreCameraTransform()
{
    transform_coordinates = saved_transform_coordinates;
    scale_lines = saved_scale_lines;
    scale_sizes = saved_scale_sizes;
    scale_text  = saved_scale_text;
    rotate_text = saved_rotate_text;
    viewport->setLineWidth(viewport->line_width);
}

void Camera::setOriginViewportAnchor(double ax, double ay)
{
    focal_anchor_x = ax;
    focal_anchor_y = ay;
    updateCameraMatrix();
}

void Camera::setOriginViewportAnchor(Anchor anchor)
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

DVec2 Camera::originPixelOffset() const
{
    double viewport_cx = (viewport->w / 2.0);
    double viewport_cy = (viewport->h / 2.0);

    return Vec2(
        viewport_cx + viewport->w * (focal_anchor_x - 0.5),
        viewport_cy + viewport->h * (focal_anchor_y - 0.5)
    );
}

DVec2 Camera::getViewportFocusedWorldSize() const
{
    // You want to know how big the viewport is (in world size) at the reference zoom
    DVec2 ctx_size = viewport->viewportRect().size();
    DVec2 focused_size = ctx_size / getReferenceZoom();
    return focused_size;
}

// Scale world to fit viewport rect
void Camera::cameraToViewport(
    double left,
    double top,
    double right,
    double bottom)
{
    double world_w = (right - left);
    double world_h = (bottom - top);

    double port_w = viewport_w;
    double port_h = viewport_h;

    cam_rotation = 0;
    zoom_x = port_w / world_w;
    zoom_y = port_h / world_h;
    cam_x = (port_w / 2) / zoom_x;
    cam_y = (port_h / 2) / zoom_y;
}

void Camera::focusWorldRect(
    flt128 left, flt128 top,
    flt128 right, flt128 bottom,
    bool stretch)
{
    flt128 world_w = right - left;
    flt128 world_h = bottom - top;
    cam_rotation = 0;
    pan_x = 0;
    pan_y = 0;

    if (stretch)
    {
        zoom_x = (viewport_w / world_w);
        zoom_y = (viewport_h / world_h);

        DVec2 _originWorldOffset = originWorldOffset();
        cam_x = left + _originWorldOffset.x;
        cam_y = top + _originWorldOffset.y;
    }
    else
    {
        double aspect_view = viewport_w / viewport_h;
        flt128 aspect_rect = world_w / world_h;

        if (aspect_rect > aspect_view)
        {
            // Shrink Height, gap top and bottom
            zoom_x = zoom_y = (viewport_w / world_w);

            assert(zoom_x > 0.0);
            assert(zoom_y > 0.0);

            DVec2 _originWorldOffset = originWorldOffset();

            flt128 world_ox = 0;
            flt128 world_oy = ((viewport_h - (world_h * zoom_y)) / 2.0) / zoom_y;

            cam_x = _originWorldOffset.x + left - world_ox;
            cam_y = _originWorldOffset.y + top - world_oy;
        }
        else
        {
            // Shrink Width, gap left and right
            zoom_x = zoom_y = (viewport_h / world_h);

            assert(zoom_x > 0.0);
            assert(zoom_y > 0.0);

            DVec2 _originWorldOffset = originWorldOffset();

            flt128 world_ox = ((viewport_w - (world_w * zoom_x)) / 2.0) / zoom_x;
            flt128 world_oy = 0;

            cam_x = _originWorldOffset.x + left - world_ox;
            cam_y = _originWorldOffset.y + top - world_oy;
        }
    }

    ref_zoom_x = zoom_x;
    ref_zoom_y = zoom_y;
    updateCameraMatrix();
}

void Camera::originToCenterViewport()
{
    setPos(0, 0);
}


void Camera::restrictRelativeZoomRange(double min, double max)
{
    min_zoom = min;
    max_zoom = max;
    panZoomProcess();
}

void Camera::panBegin(int _x, int _y, double touch_dist, double touch_angle)
{
    //BL::print("[DOWN] Cam Pos: (%.3f, %.3f) Zoom: (%.3f, %.3f) Angle: %.3f",
    //    (double)cam_x, (double)cam_y, (double)zoom_x, (double)zoom_y, cam_rotation);

    saveCameraTransform();
    worldTransform();

    pan_down_touch_x = _x;
    pan_down_touch_y = _y;
    pan_down_touch_dist = touch_dist;
    pan_down_touch_angle = touch_angle;

    if (direct_cam_panning)
    {
        //DVec2 world_mouse = toWorld(x, y);
        pan_beg_cam_x = this->x();
        pan_beg_cam_y = this->y();
        pan_beg_cam_zoom_x = this->zoomX();
        pan_beg_cam_zoom_y = this->zoomY();
        pan_beg_cam_angle = this->rotation();
    }
    else
    {
        pan_beg_cam_x = pan_x;
        pan_beg_cam_y = pan_y;
    }
    panning = true;

    restoreCameraTransform();
}

void Camera::panDrag(int _x, int _y, double touch_dist, double touch_angle)
{
    //BL::print("[DRAG] Cam Pos: (%.3f, %.3f) Zoom: (%.3f, %.3f) Angle: %.3f",
    //    (double)x, (double)y, (double)zoom_x, (double)zoom_y, cam_rotation);

    saveCameraTransform();
    worldTransform();

    if (panning)
    {
        if (direct_cam_panning)
        {
            //DVec2 world_mouse = toWorld(_x, _y);
            //double dx = world_mouse.x - pan_down_touch_x;
            //double dy = world_mouse.y - pan_down_touch_y;
            int dx = _x - pan_down_touch_x;
            int dy = _y - pan_down_touch_y;


            DVec2 world_offset = stageToWorldOffset(dx, dy);
            //BL::print("(dx,dy) = (%.3f, %.3f)", world_offset.x, world_offset.y);
            //BL::print() << "(dx,dy) = (" << BL::dp(3) << world_offset.x << ", " << world_offset.)y

            setPos(
                pan_beg_cam_x - world_offset.x /* * pan_mult */,
                pan_beg_cam_y - world_offset.y /* * pan_mult */
            );

            if (pan_down_touch_dist > 0.0)
            {
                double delta_zoom = touch_dist / pan_down_touch_dist;
                setZoom(
                    pan_beg_cam_zoom_x * delta_zoom,
                    pan_beg_cam_zoom_y * delta_zoom
                );
            }

            if (fingers.size() >= 2)
            {
                double delta_rotation = Math::closestAngleDifference(pan_down_touch_angle, touch_angle);
                setRotation(pan_beg_cam_angle + delta_rotation);
                //BL::print() << "Setting rotation: " << BL::dp(3) << (double)cam_rotation;

                // todo: In order to lock camera during pan, you may need to manually
                //       send an event mimicking a mouse move in order to trigger pollEvents

                //ProjectWorker::instance()->queueEvent()
            }
        }
        else
        {
            int dx = _x - pan_down_touch_x;
            int dy = _y - pan_down_touch_y;

            setPan(
                (double)(pan_beg_cam_x + (double)(dx/*/ zoom_x*/) /* * pan_mult*/),
                (double)(pan_beg_cam_y + (double)(dy/*/ zoom_y*/) /* * pan_mult*/)
            );
        }
    }

    restoreCameraTransform();
}

void Camera::panZoomProcess()
{
    if (zoomX() < ref_zoom_x * min_zoom) setZoomX(ref_zoom_x * min_zoom);
    if (zoomX() > ref_zoom_x * max_zoom) setZoomX(ref_zoom_x * max_zoom);
    if (zoomY() < ref_zoom_y * min_zoom) setZoomX(ref_zoom_y * min_zoom);
    if (zoomY() > ref_zoom_y * max_zoom) setZoomY(ref_zoom_y * max_zoom);

    if (fingers.size() == 1)
    {
        panDrag((int)fingers[0].x, (int)fingers[0].y, 0.0, 0.0);
    }
    else if (fingers.size() == 2)
    {
        double avg_x = (fingers[0].x + fingers[1].x) / 2.0;
        double avg_y = (fingers[0].y + fingers[1].y) / 2.0;
        panDrag((int)avg_x, (int)avg_y, touchDist(), touchAngle());
    }
}

void Camera::panEnd()
{
    ///panDrag(_x, _y, 0, 0);
    panning = false;
}

void Camera::handleWorldNavigation(Event event, bool single_touch_pan)
{
    if (!event.isPointerEvent())
        return;

    PointerEvent e(event);

    if (Platform()->is_mobile())
    {
        // Support both single-finger pan & 2 finger transform
        switch (e.type())
        {
        case SDL_EVENT_FINGER_DOWN:
        {
            if (fingers.size() >= 2)
            {
                // Ignore 3 or more fingers
                return;
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
        }
        break;
        case SDL_EVENT_FINGER_UP:
        {
            if (fingers.size() > 2)
            {
                // Ignore 3 or more fingers
                return;
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
        }
        break;
        case SDL_EVENT_FINGER_MOTION:
        {
            if (fingers.size() > 2)
            {
                // Ignore 3 or more fingers
                return;
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

            panZoomProcess();

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

            panZoomProcess();
            //if (panning)
            //    panDrag((int)e.x(), (int)e.y(), 0.0, 0.0);
        }
        break;
        case SDL_EVENT_MOUSE_WHEEL:
        {
            setZoomX(zoomX() + (e.wheelY() / 10.0) * zoomX());
            setZoomY(zoomY() + (e.wheelY() / 10.0) * zoomY());
            panZoomProcess();
        }
        break;
        }
    }
}

void CameraViewController::populateUI(DRect cam_area)
{
    int decimals = 1 + Math::countWholeDigits(zoom*5);
    char format[16];
    snprintf(format, sizeof(format), "%%.%df", decimals);

    static double init_cam_x = x;
    static double init_cam_y = y;
    ImGui::RevertableDragDouble("X", &x, &init_cam_x, 1 / avg_real_zoom, cam_area.x1, cam_area.x2, format);
    ImGui::RevertableDragDouble("Y", &y, &init_cam_y, 1 / avg_real_zoom, cam_area.y1, cam_area.y2, format);

    static double init_degrees = angle_degrees;
    if (ImGui::RevertableSliderDouble("Rotation", &angle_degrees, &init_degrees, 0.0, 360.0, "%.0f\xC2\xB0"))
    {
        angle = angle_degrees * Math::PI / 180.0;
    }

    // 1e16 = double limit before preicions loss
    static double zoom_speed = zoom / 100.0;
    static double init_cam_zoom = 1.0;
    if (ImGui::RevertableDragDouble("Zoom", &zoom, &init_cam_zoom, zoom_speed, 0.1, 1e16, "%.2f"))
        zoom_speed = zoom / 100.0;

    static DVec2 init_cam_zoom_xy = zoom_xy;
    ImGui::RevertableSliderDouble2("Zoom X/Y", zoom_xy.asArray(), init_cam_zoom_xy.asArray(), 0.1, 10.0, "%.2fx");
}

BL_END_NS
