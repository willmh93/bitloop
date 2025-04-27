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
}

void Mandelbrot_Scene::sceneStart()
{
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
}

void Mandelbrot_Scene::viewportProcess(Viewport* ctx)
{
}

void Mandelbrot_Scene::viewportDraw(Viewport* ctx)
{
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
