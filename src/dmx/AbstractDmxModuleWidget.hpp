#pragma once
#include "../plugin.hpp"
#include "AbstractDmxModule.hpp"
#include "DmxChannelDisplayWidget.hpp"
#include "DmxUniverseMenuItem.hpp"
#include "DmxAddressMenuItem.hpp"
#include "UseOwnDmxAddressItem.hpp"

using namespace std;

using namespace rack;

struct AbstractDmxModuleWidget : ModuleWidget {
    AbstractDmxModule* dmxModule;
    FramebufferWidget* frameBufferWidget;
    DmxChannelDisplayWidget* dmxChannelDisplayWidget;

    void appendContextMenu(Menu* menu) override;
};
