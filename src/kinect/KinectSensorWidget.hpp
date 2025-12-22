#pragma once
#include "../plugin.hpp"
#include "KinectSensor.hpp"

using namespace std;

using namespace rack;

struct KinectSensor;

struct KinectSensorWidget : ModuleWidget {
    KinectSensor* module;

    KinectSensorWidget(KinectSensor* moduleParam);

    void appendContextMenu(Menu* menu) override;
};