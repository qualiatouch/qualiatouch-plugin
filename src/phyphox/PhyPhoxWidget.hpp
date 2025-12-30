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
};

enum SensorLimitFieldName {
    MIN_X,
    MAX_X,
    MIN_Y,
    MAX_Y,
    MIN_Z,
    MAX_Z
};

struct SensorLimitField : ui::TextField {
    PhyPhoxSensor* module;
    SensorLimitFieldName fieldName;

    SensorLimitField(PhyPhoxSensor* moduleParam, SensorLimitFieldName fieldNameParam);

    void onSelectKey(const event::SelectKey& e) override;
};

struct SensorLimitMenuItem : ui::MenuItem {
    PhyPhoxSensor* module;
    SensorLimitFieldName fieldName;
    float currentValue;

    SensorLimitMenuItem(PhyPhoxSensor* moduleParam, SensorLimitFieldName fieldNameParam, float currentValue);
    std::string getDisplayText(SensorLimitFieldName name);
    Menu* createChildMenu() override;
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
