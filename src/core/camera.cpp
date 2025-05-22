#include <cmath>
#include "camera.h"
#include "project.h"
#include "platform.h"

void Camera::setTransformFilters(bool _transform_coordinates, bool _scale_line_txt, bool _scale_sizes, bool _rotate_text)
{
    transform_coordinates = _transform_coordinates;
    scale_lines_text = _scale_line_txt;
    scale_sizes = _scale_sizes;
    rotate_text = _rotate_text;
    viewport->setLineWidth(viewport->line_width);
}

void Camera::setTransformFilters(bool all)
{
    transform_coordinates = all;
    scale_lines_text = all;
    scale_sizes = all;
    rotate_text = all;
    viewport->setLineWidth(viewport->line_width);
}

void Camera::worldTransform()
{
    transform_coordinates = true;
    scale_lines_text = true;
    scale_sizes = true;
    rotate_text = true;
    viewport->setLineWidth(viewport->line_width);
}

void Camera::stageTransform()
{
    transform_coordinates = false;
    scale_lines_text = false;
    scale_sizes = true; // Make sure zoom doesn't affect circle/shape sizes
    rotate_text = false;
    viewport->setLineWidth(viewport->line_width);
}

void Camera::labelTransform()
{
    transform_coordinates = true;
    scale_lines_text = false;
    //scale_sizes = true; // leave unchanged
    rotate_text = false;
    viewport->setLineWidth(viewport->line_width);
}

void Camera::saveCameraTransform()
{
    saved_transform_coordinates = transform_coordinates;
    saved_scale_lines_text = scale_lines_text;
    saved_scale_sizes = scale_sizes;
    saved_rotate_text = rotate_text;
}

void Camera::restoreCameraTransform()
{
    transform_coordinates = saved_transform_coordinates;
    scale_lines_text = saved_scale_lines_text;
    scale_sizes = saved_scale_sizes;
    rotate_text = saved_rotate_text;
    viewport->setLineWidth(viewport->line_width);
}


void Camera::setOriginViewportAnchor(double ax, double ay)
{
    focal_anchor_x = ax;
    focal_anchor_y = ay;
}

void Camera::setOriginViewportAnchor(Anchor anchor)
{
    switch (anchor)
    {
    case Anchor::TOP_LEFT:
        focal_anchor_x = 0;
        focal_anchor_y = 0;
        break;
    case Anchor::CENTER:
        focal_anchor_x = 0.5;
        focal_anchor_y = 0.5;
        break;
    }
}

DVec2 Camera::originPixelOffset()
{
    double viewport_cx = (viewport->width / 2.0);
    double viewport_cy = (viewport->height / 2.0);

    return Vec2(
        viewport_cx + viewport->width * (focal_anchor_x - 0.5),
        viewport_cy + viewport->height * (focal_anchor_y - 0.5)
    );
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

    //enabled = true;
    rotation = 0;
    zoom_x = port_w / world_w;
    zoom_y = port_h / world_h;
    x = (port_w / 2) / zoom_x;
    y = (port_h / 2) / zoom_y;
}

void Camera::focusWorldRect(
    double left, double top, 
    double right, double bottom, 
    bool stretch)
{
    double world_w = right - left;
    double world_h = bottom - top;
    rotation = 0;
    pan_x = 0;
    pan_y = 0;

    if (stretch)
    {
        targ_zoom_x = zoom_x = (viewport_w / world_w);
        targ_zoom_y = zoom_y = (viewport_h / world_h);

        DVec2 _originWorldOffset = originWorldOffset();
        x = left + _originWorldOffset.x;
        y = top + _originWorldOffset.y;
    }
    else
    {
        double aspect_view = viewport_w / viewport_h;
        double aspect_rect = world_w / world_h;

        DVec2 stage_rect_tl;

        if (aspect_rect > aspect_view)
        {
            // Shrink Height, gap top and bottom
            zoom_x = zoom_y = (viewport_w / world_w);
            targ_zoom_x = zoom_x;
            targ_zoom_y = zoom_y;

            assert(zoom_x > 0.0);
            assert(zoom_y > 0.0);

            DVec2 _originWorldOffset = originWorldOffset();

            double world_ox = 0;
            double world_oy = ((viewport_h - (world_h * zoom_y)) / 2.0) / zoom_y;

            x = _originWorldOffset.x + left - world_ox;
            y = _originWorldOffset.y + top - world_oy;
        }
        else
        {
            // Shrink Width, gap left and right
            zoom_x = zoom_y = (viewport_h / world_h);
            targ_zoom_x = zoom_x;
            targ_zoom_y = zoom_y;

            assert(zoom_x > 0.0);
            assert(zoom_y > 0.0);

            DVec2 _originWorldOffset = originWorldOffset();

            double world_ox = ((viewport_w - (world_w * zoom_x)) / 2.0) / zoom_x;
            double world_oy = 0;

            x = _originWorldOffset.x + left - world_ox;
            y = _originWorldOffset.y + top - world_oy;
        }
    }

    reference_zoom_x = zoom_x;
    reference_zoom_y = zoom_y;
}

void Camera::originToCenterViewport()
{
    x = 0;
    y = 0;
}


void Camera::setRelativeZoomRange(double min, double max)
{
    min_zoom = min;
    max_zoom = max;
}

void Camera::panBegin(int _x, int _y, double touch_dist, double touch_angle)
{
    saveCameraTransform();
    worldTransform();

    pan_down_touch_x = _x;
    pan_down_touch_y = _y;
    pan_down_touch_dist = touch_dist;
    pan_down_touch_angle = touch_angle;

    if (use_panning_offset)
    {
        pan_beg_cam_x = targ_pan_x;
        pan_beg_cam_y = targ_pan_y;
    }
    else
    {
        //DVec2 world_mouse = toWorld(x, y);
        pan_beg_cam_x = x;
        pan_beg_cam_y = y;
        pan_beg_cam_zoom_x = zoom_x;
        pan_beg_cam_zoom_y = zoom_y;
        pan_beg_cam_angle = rotation;
    }
    panning = true;

    restoreCameraTransform();
}

void Camera::panDrag(int _x, int _y, double touch_dist, double touch_angle)
{
    saveCameraTransform();
    worldTransform();

    if (panning)
    {
        if (use_panning_offset)
        {
            int dx = _x - pan_down_touch_x;
            int dy = _y - pan_down_touch_y;
            targ_pan_x = pan_beg_cam_x + (double)(dx / zoom_x) * pan_mult;
            targ_pan_y = pan_beg_cam_y + (double)(dy / zoom_y) * pan_mult;
        }
        else
        {
            DVec2 world_mouse = toWorld(_x, _y);
            //double dx = world_mouse.x - pan_down_touch_x;
            //double dy = world_mouse.y - pan_down_touch_y;
            int dx = _x - pan_down_touch_x;
            int dy = _y - pan_down_touch_y;

            double delta_rotation = Math::closestAngleDifference(pan_down_touch_angle, touch_angle);

            DVec2 world_offset = toWorldOffset(dx, dy);
            //qDebug() << "(dx,dy) = (" << dx << ", " << dy << ")";

            x = pan_beg_cam_x - world_offset.x * pan_mult;
            y = pan_beg_cam_y - world_offset.y * pan_mult;

            if (pan_down_touch_dist > 0.0)
            {
                double delta_zoom = touch_dist / pan_down_touch_dist;
                zoom_x = pan_beg_cam_zoom_x * delta_zoom;
                zoom_y = pan_beg_cam_zoom_y * delta_zoom;
            }

            rotation = pan_beg_cam_angle + delta_rotation;
        }
    }

    restoreCameraTransform();
}

void Camera::panZoomProcess()
{
    double ease = 1.0;

    /*if (targ_zoom_x < reference_zoom_x*min_zoom) targ_zoom_x = reference_zoom_x*min_zoom;
    if (targ_zoom_y < reference_zoom_y*min_zoom) targ_zoom_y = reference_zoom_y*min_zoom;
    if (zoom_x < reference_zoom_x*min_zoom) zoom_x = reference_zoom_x*min_zoom;
    if (zoom_y < reference_zoom_y*min_zoom) zoom_y = reference_zoom_y*min_zoom;

    if (targ_zoom_x > reference_zoom_x * max_zoom) targ_zoom_x = reference_zoom_x * max_zoom;
    if (targ_zoom_y > reference_zoom_y * max_zoom) targ_zoom_y = reference_zoom_y * max_zoom;
    if (zoom_x > reference_zoom_x * max_zoom) zoom_x = reference_zoom_x * max_zoom;
    if (zoom_y > reference_zoom_y * max_zoom) zoom_y = reference_zoom_y * max_zoom;
    */
    pan_x += (targ_pan_x - pan_x) * ease;
    pan_y += (targ_pan_y - pan_y) * ease;
    ///zoom_x += (targ_zoom_x - zoom_x) * ease;
    ///zoom_y += (targ_zoom_y - zoom_y) * ease;
}


void Camera::panEnd(int _x, int _y)
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
            case SDL_FINGERDOWN:
            {
                if (pressed_fingers.size() >= 2)
                {
                    // Ignore 3 or more fingers
                    return;
                }

                // Add pressed finger
                FingerInfo info;
                info.fingerId = e.fingerID();
                info.x = e.x();
                info.y = e.y();
                pressed_fingers.push_back(info);

                if (pressed_fingers.size() == 1)
                {
                    if (single_touch_pan)
                        panBegin((int)e.x(), (int)e.y(), 0.0, 0.0);
                }
                else if (pressed_fingers.size() == 2)
                {
                    if (panning)
                    {
                        // Was already panning with a single finger. Restart with 2 fingers
                        panEnd((int)pressed_fingers.front().x, (int)pressed_fingers.front().y);
                    }

                    double avg_x = (pressed_fingers[0].x + pressed_fingers[1].x) / 2.0;
                    double avg_y = (pressed_fingers[0].y + pressed_fingers[1].y) / 2.0;
                    panBegin((int)avg_x, (int)avg_y, touchDist(), touchAngle());
                }
            }
            break;
            case SDL_FINGERUP:
            {
                if (pressed_fingers.size() > 2)
                {
                    // Ignore 3 or more fingers
                    return;
                }

                double avg_x = 0.0;
                double avg_y = 0.0;
                if (pressed_fingers.size() == 2)
                {
                    avg_x = (pressed_fingers[0].x + pressed_fingers[1].x) / 2.0;
                    avg_y = (pressed_fingers[0].y + pressed_fingers[1].y) / 2.0;
                }

                // Erase lifted finger
                for (size_t i = 0; i < pressed_fingers.size(); i++)
                {
                    FingerInfo& info = pressed_fingers[i];
                    if (info.fingerId == e.fingerID())
                    {
                        pressed_fingers.erase(pressed_fingers.begin() + i);
                        break;
                    }
                }

                if (pressed_fingers.size() == 0)
                {
                    // End single-finger pan (previously had 1 remaining pressed finger)
                    if (single_touch_pan)
                        panEnd((int)e.x(), (int)e.y());
                }
                else if (pressed_fingers.size() == 1)
                {
                    // End 2-finger pan, switch to single-finger pan
                    panEnd((int)avg_x, (int)avg_y);

                    // Switch to whichever finger is still pressed (might not be finger index 0)
                    if (single_touch_pan)
                        panBegin((int)pressed_fingers.front().x, (int)pressed_fingers.front().y, 0.0, 0.0);
                }
            }
            break;
            case SDL_FINGERMOTION:
            {
                if (pressed_fingers.size() > 2)
                {
                    // Ignore 3 or more fingers
                    return;
                }

                // Update finger info
                for (size_t i = 0; i < pressed_fingers.size(); i++)
                {
                    FingerInfo& info = pressed_fingers[i];
                    if (info.fingerId == e.fingerID())
                    {
                        info.x = e.x();
                        info.y = e.y();
                        break;
                    }
                }

                if (pressed_fingers.size() == 1)
                {
                    if (single_touch_pan)
                        panDrag((int)e.x(), (int)e.y(), 0.0, 0.0);
                }
                else if (pressed_fingers.size() == 2)
                {
                    double avg_x = (pressed_fingers[0].x + pressed_fingers[1].x) / 2.0;
                    double avg_y = (pressed_fingers[0].y + pressed_fingers[1].y) / 2.0;
                    panDrag((int)avg_x, (int)avg_y, touchDist(), touchAngle());
                }
            }
            break;
        }
    }
    else
    {
        switch (e.type())
        {
            case SDL_MOUSEBUTTONDOWN:
            {
                if (e.button() == SDL_BUTTON_MIDDLE ||
                    (single_touch_pan && e.button() == SDL_BUTTON_LEFT))
                {
                    panBegin((int)e.x(), (int)e.y(), 0.0, 0.0);
                }
            }
            break;
            case SDL_MOUSEBUTTONUP:
            {
                if (e.button() == SDL_BUTTON_MIDDLE ||
                    (single_touch_pan && e.button() == SDL_BUTTON_LEFT))
                {
                    panEnd((int)e.x(), (int)e.y());
                }
            }
            break;
            case SDL_MOUSEMOTION:
            {
                if (panning)
                    panDrag((int)e.x(), (int)e.y(), 0.0, 0.0);
            }
            break;
            case SDL_MOUSEWHEEL:
            {
                zoom_x += (e.wheelY() / 10.0) * zoom_x;
                zoom_y += (e.wheelY() / 10.0) * zoom_y;
            }
            break;
        }
    }
}
