#include "ImageTest.h"




SIM_DECLARE(ImageTest, "Framework Tests", "Image Transforms")


/// =========================
/// ======== Project ========
/// =========================

void ImageTest_Project_Vars::populate()
{
    ImGui::SliderInt("Viewport Count", &viewport_count, 1, 8);
}

void ImageTest_Project::projectPrepare(Layout& layout)
{
    /// Create multiple instance of a single Scene, mount to separate viewports
    layout << create<ImageTest_Scene>(viewport_count);

    /// Or create a single Scene instance and view on multiple Viewports
    //auto* scene = create<Test_Scene>();
    //for (int i = 0; i < viewport_count; ++i)
    //    layout << scene;
}

/// =========================
/// ========= Scene =========
/// =========================

void ImageTest_Scene_Attributes::populate()
{
}

void ImageTest_Scene::sceneStart()
{
}

void ImageTest_Scene::sceneMounted(Viewport*)
{
    camera->setOriginViewportAnchor(Anchor::CENTER);
}

void ImageTest_Scene::sceneDestroy()
{
    // Destroy scene
}

void ImageTest_Scene::sceneProcess()
{
}

void ImageTest_Scene::viewportProcess(Viewport*, double)
{
}

void ImageTest_Scene::viewportDraw(Viewport* ctx) const
{
    ctx->drawWorldAxis();
}

void ImageTest_Scene::onEvent(Event e)
{
    handleWorldNavigation(e, true);
}

//void Test_Scene::onPointerDown(PointerEvent e) {}
//void Test_Scene::onPointerUp(PointerEvent e) {}
//void Test_Scene::onPointerMove(PointerEvent e) {}
//void Test_Scene::onWheel(PointerEvent e) {}
//void Test_Scene::onKeyDown(KeyEvent e) {}
//void Test_Scene::onKeyUp(KeyEvent e) {}


SIM_END(ImageTest)
