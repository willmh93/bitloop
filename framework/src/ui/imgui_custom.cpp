#include <ui/imgui_custom.h>

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#include "nanosvg/nanosvgrast.h"
#include "stb_image.h"

bool LoadPixelsRGBA(const char* path, int* outW, int* outH, std::vector<unsigned char>& outPixels)
{
    int w, h, n;
    unsigned char* data = stbi_load(BL::Platform()->path(path).c_str(), &w, &h, &n, 4);  // force RGBA
    if (!data) return false;

    outPixels.assign(data, data + (w * h * 4));
    stbi_image_free(data);
    *outW = w;
    *outH = h;
    return true;
}

GLuint CreateGLTextureRGBA8(const void* pixels, int w, int h)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0,
        GL_RGBA, // internal format
        w, h, 0,
        GL_RGBA, // format
        GL_UNSIGNED_BYTE,
        pixels);

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

GLuint loadSVG(const char* path, int outputWidth, int outputHeight)
{
    // Step 1: Parse SVG
    NSVGimage* image = nsvgParseFromFile(BL::Platform()->path(path).c_str(), "px", 96.0f); // 96 dpi, or 72 dpi depending on your needs
    if (!image) {
        printf("Could not open SVG image: %s\n", path);
        return 0;
    }

    // Step 2: Create a rasterizer
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(image);
        printf("Could not init rasterizer\n");
        return 0;
    }

    // Step 3: Allocate buffer for the output
    int stride = outputWidth * 4;  // 4 bytes per pixel (RGBA)
    unsigned char* img = (unsigned char*)malloc(outputWidth * outputHeight * 4);
    if (!img) {
        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);
        printf("Could not allocate image buffer\n");
        return 0;
    }

    // Clear buffer to transparent
    memset(img, 0, outputWidth * outputHeight * 4);

    // Step 4: Rasterize
    float scaleX = (float)outputWidth / image->width;
    float scaleY = (float)outputHeight / image->height;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;  // Keep aspect ratio

    nsvgRasterize(rast, image, 0, 0, scale, img, outputWidth, outputHeight, stride);

    // Step 5: Create OpenGL texture
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, outputWidth, outputHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Cleanup
    free(img);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    return tex;
}

void DestroyTexture(GLuint tex)
{
    glDeleteTextures(1, &tex);
}


std::ostream& operator<<(std::ostream& os, const ImSpline::Spline& spline)
{
    (void)spline;
    return os << "Spline";
}

std::ostream& operator<<(std::ostream& os, const ImGradient& gradient)
{
    (void)gradient;
    return os << "Gradient";
}

namespace ImGui
{
    bool Section(const char* name, bool open_by_default, float header_spacing, float body_margin_top)
    {
        ImGui::Dummy(BL::ScaleSize(0.0f, header_spacing));
        bool ret = ImGui::CollapsingHeader(name, open_by_default ? ImGuiTreeNodeFlags_DefaultOpen : 0);
        if (ret) ImGui::Dummy(BL::ScaleSize(0.0f, body_margin_top));
        return ret;
    }

    bool ResetBtn(const char* id)
    {
        static int size = (int)BL::Platform()->line_height();
        static int reset_icon = loadSVG("/data/icon/reset.svg", size, size);
        return ImGui::ImageButton(id, reset_icon, ImVec2((float)size, (float)size));
    }

    bool InlResetBtn(const char* id)
    {
        ImGui::SameLine();
        bool ret = ResetBtn(id);
        return ret;
    }

    bool RevertableSliderDouble(const char* label, double* v, double* initial, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        bool ret = false;
        ImGui::PushItemWidth(ImGui::CalcItemWidth() - BL::Platform()->line_height());
        ret |= SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
        ImGui::PopItemWidth();

        ImGui::PushID("reset_");
        if (*v != *initial && InlResetBtn(label))
        {
            *v = *initial;
            ret |= true;
        }
        ImGui::PopID();
        return ret;
    }

    bool RevertableDragDouble(const char* label, double* v, double* initial, double v_speed, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        bool ret = false;

        ImGui::PushItemWidth(ImGui::CalcItemWidth() - BL::Platform()->line_height());
        ret |= DragScalar(label, ImGuiDataType_Double, v, (float)v_speed, &v_min, &v_max, format, flags);
        ImGui::PopItemWidth();

        ImGui::PushID("reset_");
        if (*v != *initial && InlResetBtn(label))
        {
            *v = *initial;
            ret |= true;
        }
        ImGui::PopID();
        return ret;
    }

    bool RevertableSliderDouble2(const char* label, double v[2], double initial[2], double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        bool ret = false;

        ImGui::PushItemWidth(ImGui::CalcItemWidth() - BL::Platform()->line_height());
        ret |= SliderScalarN(label, ImGuiDataType_Double, v, 2, &v_min, &v_max, format, flags);
        ImGui::PopItemWidth();

        ImGui::PushID("reset_");
        if ((v[0] != initial[0] || v[1] != initial[1]) && InlResetBtn(label))
        {
            v[0] = initial[0];
            v[1] = initial[1];
            ret |= true;
        }
        ImGui::PopID();
        return ret;
    }

    bool SliderDouble(const char* label, double* v, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return SliderScalar(label, ImGuiDataType_Double, v, &v_min, &v_max, format, flags);
    }

    bool DragDouble(const char* label, double* v, double v_speed, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return DragScalar(label, ImGuiDataType_Double, v, (float)v_speed, &v_min, &v_max, format, flags);
    }

    bool SliderDouble2(const char* label, double v[2], double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return SliderScalarN(label, ImGuiDataType_Double, v, 2, &v_min, &v_max, format, flags);
    }

    bool DragDouble2(const char* label, double v[2], double v_speed, double v_min, double v_max, const char* format, ImGuiSliderFlags flags)
    {
        return DragScalarN(label, ImGuiDataType_Double, v, 2, (float)v_speed, &v_min, &v_max, format, flags);
    }

    float old_work_rect_width = 0.0f;
    float old_conent_rect_width = 0.0f;

    float region_padding = 0.0f;

    void BeginPaddedRegion(float padding) 
    {
        //const float fullW = ImGui::GetContentRegionAvail().x;
        ImVec2 p0 = ImGui::GetCursorScreenPos() + ImVec2(padding, padding);
        ImGui::SetCursorScreenPos(p0);
        ImGui::BeginGroup();

        region_padding = padding;

        old_work_rect_width = ImGui::GetCurrentWindow()->WorkRect.Max.x;
        old_conent_rect_width = ImGui::GetCurrentWindow()->ContentRegionRect.Max.x;
        ImGui::GetCurrentWindow()->WorkRect.Max.x -= padding;
        ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= padding;
        ImGui::PushTextWrapPos(old_conent_rect_width - padding);
    }
    void EndPaddedRegion()
    {
        ImGui::PopTextWrapPos();
        ImGui::GetCurrentWindow()->ContentRegionRect.Max.x = old_conent_rect_width;
        ImGui::GetCurrentWindow()->WorkRect.Max.x = old_work_rect_width;

        ImGui::EndGroup();
        ImVec2 p1 = ImGui::GetItemRectMax() + ImVec2(region_padding, region_padding);
        ImGui::Dummy(ImVec2(region_padding, region_padding));
        region_padding = 0.0f;
    }
}
