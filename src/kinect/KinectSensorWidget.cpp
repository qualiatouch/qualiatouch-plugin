#include "KinectSensorWidget.hpp"

KinectSensorWidget::KinectSensorWidget(KinectSensor* moduleParam) {
    module = moduleParam;
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/kinect.svg")));

    addChild(createWidget<ScrewSilver>(Vec(0, 0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH * 2, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addParam(createParamCentered<Trimpot>(mm2px(Vec(7.625, 110)), module, KinectSensor::THRESHOLD_PARAM));

    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 30)), module, KinectSensor::OUT_HAND_X));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 50)), module, KinectSensor::OUT_HAND_Y));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 70)), module, KinectSensor::OUT_HAND_DEPTH));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 90)), module, KinectSensor::OUT_HAND_DEPTH_THRESHOLD));
}

void KinectSensorWidget::appendContextMenu(Menu* menu) {
    menu->addChild(new MenuSeparator);
    menu->addChild(createIndexPtrSubmenuItem("Sensor tilt",
        {
            "0°",
            "5°",
            "10°",
            "15°",
            "20°",
            "25°",
            "30°"
        },
        &module->tiltRequest));

    menu->addChild(rack::createBoolPtrMenuItem("Debug", "", &module->debug));

    if (module->debug) {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Debug info"));
        menu->addChild(createMenuLabel("HasDevice " + to_string(module->hasDevice)));
        menu->addChild(createMenuLabel("Current tilt " + to_string(module->currentTilt)));
        menu->addChild(createMenuLabel("Requested tilt " + to_string(module->tiltRequest)));
    }
}
