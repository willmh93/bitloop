#include <cmath>
#include "camera.h"
#include "project.h"

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

Vec2 Camera::originPixelOffset()
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

        Vec2 _originWorldOffset = originWorldOffset();
        x = left + _originWorldOffset.x;
        y = top + _originWorldOffset.y;
    }
    else
    {
        double aspect_view = viewport_w / viewport_h;
        double aspect_rect = world_w / world_h;

        Vec2 stage_rect_tl;

        if (aspect_rect > aspect_view)
        {
            // Shrink Height, gap top and bottom
            zoom_x = zoom_y = (viewport_w / world_w);
            targ_zoom_x = zoom_x;
            targ_zoom_y = zoom_y;

            Vec2 _originWorldOffset = originWorldOffset();

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

            Vec2 _originWorldOffset = originWorldOffset();

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

void Camera::panBegin(int _x, int _y)
{
    pan_down_mx = _x;
    pan_down_my = _y;

    if (use_panning_offset)
    {
        pan_beg_x = targ_pan_x;
        pan_beg_y = targ_pan_y;
    }
    else
    {
        //Vec2 world_mouse = toWorld(x, y);
        pan_beg_x = x;
        pan_beg_y = y;
    }
    panning = true;
}

void Camera::panDrag(int _x, int _y)
{
    if (panning)
    {
        if (use_panning_offset)
        {
            int dx = _x - pan_down_mx;
            int dy = _y - pan_down_my;
            targ_pan_x = pan_beg_x + (double)(dx / zoom_x) * pan_mult;
            targ_pan_y = pan_beg_y + (double)(dy / zoom_y) * pan_mult;
        }
        else
        {
            Vec2 world_mouse = toWorld(_x, _y);
            //double dx = world_mouse.x - pan_down_mx;
            //double dy = world_mouse.y - pan_down_my;
            int dx = _x - pan_down_mx;
            int dy = _y - pan_down_my;
            Vec2 world_offset = toWorldOffset(dx, dy);
            //qDebug() << "(dx,dy) = (" << dx << ", " << dy << ")";

            x = pan_beg_x - world_offset.x * pan_mult;
            y = pan_beg_y - world_offset.y * pan_mult;
        }
    }
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
    zoom_x += (targ_zoom_x - zoom_x) * ease;
    zoom_y += (targ_zoom_y - zoom_y) * ease;
}

void Camera::panEnd(int _x, int _y)
{
    panDrag(_x, _y);
    panning = false;
}
