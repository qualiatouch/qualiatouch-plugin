#pragma once
#include "../plugin.hpp"
#include "DmxOut1.hpp"
#include "DmxChannelDisplayWidget.hpp"
#include "DmxUniverseMenuItem.hpp"
#include "DmxAddressMenuItem.hpp"
#include "UseOwnDmxAddressItem.hpp"

using namespace std;

using namespace rack;

struct DmxOut1;

struct DmxOut1Widget : ModuleWidget {
    DmxOut1* module;
    FramebufferWidget* frameBufferWidget;
    DmxChannelDisplayWidget* dmxChannelDisplayWidget;

    DmxOut1Widget(DmxOut1* moduleParam);

    void appendContextMenu(Menu* menu) override;
};
