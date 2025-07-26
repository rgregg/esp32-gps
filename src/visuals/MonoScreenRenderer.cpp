#include "MonoScreenRenderer.h"

// Render the various screens on a monochrome OLED display which is 128x64 pixels.

MonoScreenRenderer::MonoScreenRenderer(Display* display, FS* fileSys) : 
    ScreenRenderer(display, fileSys) {

}