#pragma once
#include "../plugin.hpp"
#include "DmxOut1.hpp"
#include "AbstractDmxModuleWidget.hpp"

using namespace std;

using namespace rack;

struct DmxOut1;

struct DmxOut1Widget : AbstractDmxModuleWidget {
    DmxOut1* module;
    FramebufferWidget* frameBufferWidget;
    DmxChannelDisplayWidget* dmxChannelDisplayWidget;

    DmxOut1Widget(DmxOut1* moduleParam);
    ~DmxOut1Widget();
};
