#include <bitloop/core/viewport.h>
#include <bitloop/core/scene.h>

BL_BEGIN_NS

Viewport::Viewport(Layout* layout, int viewport_index, int grid_x, int grid_y) : Painter(this),
    layout(layout),
    viewport_index(viewport_index),
    viewport_grid_x(grid_x),
    viewport_grid_y(grid_y)
{
    //camera.viewport = this;
}

Viewport::~Viewport()
{
    if (scene)
    {
        // Unmount sim from viewport
        scene->registerUnmount(this);

        // If sim is no longer mounted to any viewports, it's safe to destroy
        if (scene->mounted_to_viewports.size() == 0)
        {
            scene->sceneDestroy();
            delete scene;
        }
    }
}

void Viewport::draw()
{
    // Set defaults
    setFont(getDefaultFont());
    setTextAlign(TextAlign::ALIGN_LEFT);
    setTextBaseline(TextBaseline::BASELINE_TOP);

    // Snapshot default transformation (unscaled unrotated top-left viewport)
    default_viewport_transform = SimplePainter::currentTransform();
    SimplePainter::resetTransform();

    print_stream.str("");
    print_stream.clear();

    // Draw mounted project to this viewport
    scene->viewportDraw(this);

    // Draw print() lines at top-left of viewport
    save();
    saveCameraTransform();
    stageMode();
    setTextAlign(TextAlign::ALIGN_LEFT);
    setTextBaseline(TextBaseline::BASELINE_TOP);
    setFillStyle(255, 255, 255);

    print_stream.seekg(0);
    int line_index = 0;
    std::string line;
    while (std::getline(print_stream, line, '\n'))
        fillText(line, 5.0, 5.0 + (line_index++ * getGlobalScale() * 16.0f));

    restoreCameraTransform();
    restore();
}

///void Viewport::printTouchInfo()
///{
///    double pinch_zoom_factor = camera.touchDist() / camera.panDownTouchDist();
///
///    setFont(getDefaultFont());
///    print() << "cam.position          = (" << camera.x() << ",  " << camera.y() << ")\n";
///    print() << "cam.pan               = (" << camera.panX() << ",  " << camera.panY() << ")\n";
///    print() << "cam.rotation          = " << camera.rotation() << "\n";
///    ///print() << "cam.zoom              = (" << camera.zoomX() << ", " << camera.zoomY() << ")\n\n";
///    print() << "--------------------------------------------------------------\n";
///    print() << "pan_down_touch_x      = " << camera.panDownTouchX() << "\n";
///    print() << "pan_down_touch_y      = " << camera.panDownTouchY() << "\n";
///    print() << "pan_down_touch_dist   = " << camera.panDownTouchDist() << "\n";
///    print() << "pan_down_touch_angle  = " << camera.panDownTouchAngle() << "\n";
///    print() << "--------------------------------------------------------------\n";
///    print() << "pan_beg_cam_x         = " << (double)camera.panBegCamX() << "\n";
///    print() << "pan_beg_cam_y         = " << (double)camera.panBegCamY() << "\n";
///    print() << "pan_beg_cam_zoom_x    = " << (double)camera.panBegCamZoom() << "\n";
///    print() << "pan_beg_cam_angle     = " << camera.panBegCamAngle() << "\n";
///    print() << "--------------------------------------------------------------\n";
///    print() << "cam.touchAngle()      = " << camera.touchAngle() << "\n";
///    print() << "cam.touchDist()       = " << camera.touchDist() << "\n";
///    print() << "--------------------------------------------------------------\n";
///    print() << "pinch_zoom_factor     = " << pinch_zoom_factor << "x\n";
///    print() << "--------------------------------------------------------------\n";
///
///    int i = 0;
///    for (auto finger : camera.pressedFingers())
///    {
///        print() << "Finger " << i << ": [id: " << finger.fingerId << "] - (" << finger.x << ", " << finger.y << ")\n";
///        i++;
///    }
///}


BL_END_NS
