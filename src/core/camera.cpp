#include <cmath>
#include "camera.h"
#include "project.h"
#include "platform.h"

//void Camera::setTransformFilters(bool _transform_coordinates, bool _scale_line_txt, bool _scale_sizes, bool _rotate_text)
//{
//    transform_coordinates = _transform_coordinates;
//    scale_lines_text = _scale_line_txt;
//    scale_sizes = _scale_sizes;
//    rotate_text = _rotate_text;
//    viewport->setLineWidth(viewport->line_width);
//}
//
//void Camera::setTransformFilters(bool all)
//{
//    transform_coordinates = all;
//    scale_lines_text = all;
//    scale_sizes = all;
//    rotate_text = all;
//    viewport->setLineWidth(viewport->line_width);
//}

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
    scale_sizes = false;// true; // Make sure zoom doesn't affect circle/shape sizes
    rotate_text = false;
    viewport->setLineWidth(viewport->line_width);
}

void Camera::worldHudTransform()
{
    transform_coordinates = true;
    scale_lines_text = false;
    scale_sizes = false; //scale_sizes = true; // leave unchanged
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

DVec2 Camera::originPixelOffset()
{
    double viewport_cx = (viewport->width / 2.0);
    double viewport_cy = (viewport->height / 2.0);

    return Vec2(
        viewport_cx + viewport->width * (focal_anchor_x - 0.5),
        viewport_cy + viewport->height * (focal_anchor_y - 0.5)
    );
}

DVec2 Camera::getViewportFocusedWorldSize()
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
    double left, double top, 
    double right, double bottom, 
    bool stretch)
{
    double world_w = right - left;
    double world_h = bottom - top;
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
        double aspect_rect = world_w / world_h;

        if (aspect_rect > aspect_view)
        {
            // Shrink Height, gap top and bottom
            zoom_x = zoom_y = (viewport_w / world_w);

            assert(zoom_x > 0.0);
            assert(zoom_y > 0.0);

            DVec2 _originWorldOffset = originWorldOffset();

            double world_ox = 0;
            double world_oy = ((viewport_h - (world_h * zoom_y)) / 2.0) / zoom_y;

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

            double world_ox = ((viewport_w - (world_w * zoom_x)) / 2.0) / zoom_x;
            double world_oy = 0;

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
    saveCameraTransform();
    worldTransform();

    pan_down_touch_x = _x;
    pan_down_touch_y = _y;
    pan_down_touch_dist = touch_dist;
    pan_down_touch_angle = touch_angle;

    if (direct_cam_panning)
    {
        pan_beg_cam_x = pan_x;
        pan_beg_cam_y = pan_y;
    }
    else
    {
        //DVec2 world_mouse = toWorld(x, y);
        pan_beg_cam_x = cam_x;
        pan_beg_cam_y = cam_y;
        pan_beg_cam_zoom_x = zoom_x;
        pan_beg_cam_zoom_y = zoom_y;
        pan_beg_cam_angle = cam_rotation;
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
        if (direct_cam_panning)
        {
            int dx = _x - pan_down_touch_x;
            int dy = _y - pan_down_touch_y;
            
            pan_x = pan_beg_cam_x + (double)(dx/*/ zoom_x*/) /* * pan_mult*/;
            pan_y = pan_beg_cam_y + (double)(dy/*/ zoom_y*/) /* * pan_mult*/;
        }
        else
        {
            //DVec2 world_mouse = toWorld(_x, _y);
            //double dx = world_mouse.x - pan_down_touch_x;
            //double dy = world_mouse.y - pan_down_touch_y;
            int dx = _x - pan_down_touch_x;
            int dy = _y - pan_down_touch_y;


            DVec2 world_offset = stageToWorldOffset(dx, dy);
            //qDebug() << "(dx,dy) = (" << dx << ", " << dy << ")";

            cam_x = pan_beg_cam_x - world_offset.x /* * pan_mult */;
            cam_y = pan_beg_cam_y - world_offset.y /* * pan_mult */;

            if (pan_down_touch_dist > 0.0)
            {
                double delta_zoom = touch_dist / pan_down_touch_dist;
                zoom_x = pan_beg_cam_zoom_x * delta_zoom;
                zoom_y = pan_beg_cam_zoom_y * delta_zoom;
            }
            
            if (fingers.size() >= 2)
            {
                double delta_rotation = Math::closestAngleDifference(pan_down_touch_angle, touch_angle);
                cam_rotation = pan_beg_cam_angle + delta_rotation;
                DebugPrint("Setting rotation: %.3f", (double)cam_rotation);

                // todo: In order to lock camera during pan, you may need to manually
                //       send an event mimicking a mouse move in order to trigger pollEvents

                //ProjectWorker::instance()->queueEvent()
            }
        }
    }

    restoreCameraTransform();
    updateCameraMatrix();
}

void Camera::panZoomProcess()
{
    if (zoom_x < ref_zoom_x * min_zoom) setZoomX(ref_zoom_x * min_zoom);
    if (zoom_x > ref_zoom_x * max_zoom) setZoomX(ref_zoom_x * max_zoom);
    if (zoom_y < ref_zoom_y * min_zoom) setZoomX(ref_zoom_y * min_zoom);
    if (zoom_y > ref_zoom_y * max_zoom) setZoomY(ref_zoom_y * max_zoom);
    
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
            case SDL_FINGERDOWN:
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
            case SDL_FINGERUP:
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
                std::erase_if(fingers, [&](const FingerInfo& f) 
                {
                    return f.fingerId == e.fingerID();
                });

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
            case SDL_FINGERMOTION:
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
            case SDL_MOUSEBUTTONDOWN:
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
            case SDL_MOUSEBUTTONUP:
            {
                fingers.clear();
                if (e.button() == SDL_BUTTON_MIDDLE ||
                    (single_touch_pan && e.button() == SDL_BUTTON_LEFT))
                {
                    panEnd();
                }
            }
            break;
            case SDL_MOUSEMOTION:
            {
                if (fingers.size() > 0)
                {
                    FingerInfo& info = fingers[0];
                    info.x = e.x();
                    info.y = e.y();
                }

                DebugPrint("handleWorldNavigation");
                panZoomProcess();
                //if (panning)
                //    panDrag((int)e.x(), (int)e.y(), 0.0, 0.0);
            }
            break;
            case SDL_MOUSEWHEEL:
            {
                zoom_x += (e.wheelY() / 10.0) * zoom_x;
                zoom_y += (e.wheelY() / 10.0) * zoom_y;
                panZoomProcess();
                updateCameraMatrix();
            }
            break;
        }
    }
}
