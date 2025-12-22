#pragma once
#include "../plugin.hpp"
#include "PhyPhoxSensor.hpp"

using namespace std;

using namespace rack;

struct PhyPhoxSensor;

struct IpAddressField : ui::TextField {
    PhyPhoxSensor* module;

    IpAddressField(PhyPhoxSensor* moduleParam);

    void onSelectKey(const event::SelectKey& e) override;
};

struct IpAddressMenuItem : ui::MenuItem {
    PhyPhoxSensor* module;

    Menu* createChildMenu() override;
    //void draw(const DrawArgs& args) override;
};

struct SensorTypeWidget : Widget {
	void draw(const DrawArgs& args) override;
};

struct PhyPhoxWidget : ModuleWidget {
    PhyPhoxSensor* module;
    FramebufferWidget* frameBufferWidget;
    SensorTypeWidget* sensorTypeWidget;

    PhyPhoxWidget(PhyPhoxSensor* moduleParam);

    void appendContextMenu(Menu* menu) override;

    void setDirty();
};
