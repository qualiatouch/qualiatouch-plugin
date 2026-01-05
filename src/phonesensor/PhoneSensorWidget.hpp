#pragma once
#include "../plugin.hpp"
#include "PhoneSensor.hpp"

using namespace std;

using namespace rack;

struct PhoneSensor;

struct IpAddressField : ui::TextField {
    PhoneSensor* module;

    IpAddressField(PhoneSensor* moduleParam);

    void onSelectKey(const event::SelectKey& e) override;
};

struct IpAddressMenuItem : ui::MenuItem {
    PhoneSensor* module;

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
    PhoneSensor* module;
    SensorLimitFieldName fieldName;

    SensorLimitField(PhoneSensor* moduleParam, SensorLimitFieldName fieldNameParam);

    void onSelectKey(const event::SelectKey& e) override;
};

struct SensorLimitMenuItem : ui::MenuItem {
    PhoneSensor* module;
    SensorLimitFieldName fieldName;
    float currentValue;

    SensorLimitMenuItem(PhoneSensor* moduleParam, SensorLimitFieldName fieldNameParam, float currentValue);
    std::string getDisplayText(SensorLimitFieldName name);
    Menu* createChildMenu() override;
};

struct SensorTypeWidget : Widget {
	void draw(const DrawArgs& args) override;
};

struct PhoneSensorWidget : ModuleWidget {
    PhoneSensor* module;
    FramebufferWidget* frameBufferWidget;
    SensorTypeWidget* sensorTypeWidget;

    PhoneSensorWidget(PhoneSensor* moduleParam);

    void appendContextMenu(Menu* menu) override;

    void setDirty();
};
