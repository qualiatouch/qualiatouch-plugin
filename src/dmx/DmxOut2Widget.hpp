#pragma once
#include "../plugin.hpp"
#include "DmxOut2.hpp"
#include "AbstractDmxModuleWidget.hpp"

using namespace std;
using namespace rack;

struct DmxOut2;

struct DmxOut2Widget : AbstractDmxModuleWidget {
    DmxOut2* module;
    FramebufferWidget* frameBufferWidget;
    DmxChannelDisplayWidget* dmxChannelDisplayWidget;

    DmxOut2Widget(DmxOut2* moduleParam);
    ~DmxOut2Widget();
};
