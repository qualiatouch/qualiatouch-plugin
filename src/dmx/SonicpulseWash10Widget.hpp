#pragma once
#include "../plugin.hpp"
#include "SonicpulseWash10.hpp"
#include "AbstractDmxModuleWidget.hpp"

using namespace std;

using namespace rack;

struct SonicpulseWash10;

struct SonicpulseWash10Widget : AbstractDmxModuleWidget {
    SonicpulseWash10* module;
    FramebufferWidget* frameBufferWidget;
    DmxChannelDisplayWidget* dmxChannelDisplayWidget;

    SonicpulseWash10Widget(SonicpulseWash10* moduleParam);
    ~SonicpulseWash10Widget();
};
