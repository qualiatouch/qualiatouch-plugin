#pragma once
#include "../plugin.hpp"
#include "DmxOut1.hpp"

using namespace std;

using namespace rack;

struct DmxOut1;

struct UseOwnDmxAddressItem : ui::MenuItem {
    DmxOut1* module;

    UseOwnDmxAddressItem(DmxOut1* moduleParam);

    void onAction(const event::Action& e) override;

    void step() override;
};

struct DmxAddressField : ui::TextField {
    DmxOut1* module;

    DmxAddressField(DmxOut1* moduleParam);

    void onSelectKey(const event::SelectKey& event) override;
};

struct DmxAddressMenuItem : ui::MenuItem {
    DmxOut1* module;

    Menu* createChildMenu() override;
};

struct DmxUniverseField : ui::TextField {
    DmxOut1* module;

    DmxUniverseField(DmxOut1* moduleParam);

    void onSelectKey(const event::SelectKey& event) override;
};

struct DmxUniverseMenuItem : ui::MenuItem {
    DmxOut1* module;

    Menu* createChildMenu() override;
};

struct DmxChannelDisplayWidget : Widget {
    DmxOut1* module;
    FramebufferWidget* parentFrameBufferWidget;

    void setModule(DmxOut1* moduleParam);

    void setParent(FramebufferWidget* parent);

    void step() override;

	void draw(const DrawArgs& args) override;
};

struct DmxOut1Widget : ModuleWidget {
    DmxOut1* module;
    FramebufferWidget* frameBufferWidget;
    DmxChannelDisplayWidget* dmxChannelDisplayWidget;

    DmxOut1Widget(DmxOut1* moduleParam);

    void appendContextMenu(Menu* menu) override;
};
