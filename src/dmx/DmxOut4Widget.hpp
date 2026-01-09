#pragma once
#include "../plugin.hpp"
#include "DmxOut4.hpp"
#include "AbstractDmxModuleWidget.hpp"

using namespace std;
using namespace rack;

struct DmxOut4;

struct DmxOut4Widget : AbstractDmxModuleWidget {
    DmxOut4* module;
    FramebufferWidget* frameBufferWidget;
    DmxChannelDisplayWidget* dmxChannelDisplayWidget;

    DmxOut4Widget(DmxOut4* moduleParam);
    ~DmxOut4Widget();
};
