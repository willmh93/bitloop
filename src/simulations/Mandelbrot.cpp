#include "Mandelbrot.h"

//SIM_SORT_KEY(0)
SIM_DECLARE(Mandelbrot, "Mandelbrot", "Viewer")


///---------///
/// Project ///
///---------///

void Mandelbrot_Project::projectAttributes()
{

}

void Mandelbrot_Project::projectPrepare()
{
    auto& layout = newLayout();

    Mandelbrot_Scene::Config config1;
    //auto config2 = make_shared<Test_Scene::Config>(Test_Scene::Config());

    create<Mandelbrot_Scene>(config1)->mountTo(layout);
}

///-----------///
/// SceneBase ///
///-----------///

void Mandelbrot_Scene::sceneAttributes()
{
    ImGui::SliderDouble("Camera Rotatation", &camera_rotation, 0.0, pi * 2.0); // updated in realtime
    ImGui::SliderDouble("Camera X", &camera_x, -500.0, 500.0); // updated in realtime
    ImGui::SliderDouble("Camera Y", &camera_y, -500.0, 500.0); // updated in realtime
    ImGui::SliderDouble("Zoom X", &zoom_x, -2.0, 2.0); // updated in realtime
    ImGui::SliderDouble("Zoom Y", &zoom_y, -2.0, 2.0); // updated in realtime

}

void Mandelbrot_Scene::sceneStart()
{
    bmp.create(700, 700);
}

void Mandelbrot_Scene::sceneMounted(Viewport* viewport)
{
    camera->setOriginViewportAnchor(Anchor::CENTER);
}

void Mandelbrot_Scene::sceneDestroy()
{
}

void Mandelbrot_Scene::sceneProcess()
{
    //for (int y = 0; y < bmp.height(); y++)
    //{
    //    for (int x = 0; x < bmp.width(); x++)
    //    {
    //        bmp.setPixel(x, y, rand()%255, rand()%255, rand()%255);
    //    }
    //}
    
}

void Mandelbrot_Scene::viewportProcess(Viewport* ctx)
{
    camera->x = camera_x;
    camera->y = camera_y;
    camera->zoom_x = zoom_x;
    camera->zoom_y = zoom_y;
    camera->rotation = camera_rotation;

    double fw = ctx->width;
    double fh = ctx->height;
    int iw = static_cast<int>(fw);
    int ih = static_cast<int>(fh);

    bmp.setStageRect(0, 0, ctx->width, ctx->height);
    bmp.setBitmapSize(iw, ih);

    if (bmp.needsReshading(camera))
    {
        bmp.forEachWorldPixel(camera, [&](int x, int y, double wx, double wy)
        {
            bmp.setPixel(x, y, wx, 0, wy);
        });
    }
}

void Mandelbrot_Scene::viewportDraw(Viewport* ctx)
{

    ctx->drawImage(bmp);
}

/// User Interaction

/*void Test_Scene::mouseDown()
{
    //ball_pos.x = mouse.world_x;
    //ball_pos.y = mouse.world_y;
}
void Test_Scene::mouseUp()
{
    //ball_pos.x = mouse.world_x;
    //ball_pos.y = mouse.world_y;
}
void Test_Scene::mouseMove()
{
    ball_pos.x = mouse->world_x;
    ball_pos.y = mouse->world_y;
}
void Test_Scene::mouseWheel() {}*/

SIM_END(Mandelbrot)
