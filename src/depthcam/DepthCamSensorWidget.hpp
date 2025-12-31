#pragma once
#include "../plugin.hpp"
#include "DepthCamSensor.hpp"

using namespace std;

using namespace rack;

struct DepthCamSensor;

struct DepthCamSensorWidget : ModuleWidget {
    DepthCamSensor* module;

    DepthCamSensorWidget(DepthCamSensor* moduleParam);

    void appendContextMenu(Menu* menu) override;
};