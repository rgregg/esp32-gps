#pragma once

#include "ScreenRenderer.h"

class MonoScreenRenderer : public ScreenRenderer 
{
public:
    MonoScreenRenderer(Display* display, FS* fileSys);
};