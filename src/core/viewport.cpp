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

BL_END_NS
