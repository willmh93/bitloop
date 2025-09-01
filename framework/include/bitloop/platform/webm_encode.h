#pragma once

#ifdef __cplusplus
extern "C" {
    #endif

    // Call this each time you have a new RGBA frame at ptr (w×h×4 bytes)
    void add_frame(unsigned char* ptr, int w, int h);

    // Call this when you’re done; it will trigger the download
    void finish_video(void);

    #ifdef __cplusplus
}
#endif