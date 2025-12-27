#include "PhyPhoxWidget.hpp"

IpAddressField::IpAddressField(PhyPhoxSensor* moduleParam) {
    module = moduleParam;
    box.size.x = 200;
    placeholder = "192.168.1.25:8080";
}

void IpAddressField::onSelectKey(const event::SelectKey& e) {
    if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ENTER) {
        std::string ip = text;
        if (module) {
            module->setIpAddress(ip);
        }
        ui::MenuOverlay* overlay = getAncestorOfType<ui::MenuOverlay>();
        if (overlay) {
            overlay->requestDelete();
        }
        e.consume(this);
    }
    if (!e.getTarget()) {
        TextField::onSelectKey(e);
    }
}

Menu* IpAddressMenuItem::createChildMenu() {
    Menu* menu = new Menu;

    IpAddressField* ipField = new IpAddressField(module);
    ipField->text = module->ip;
    menu->addChild(ipField);

    return menu;
}

void SensorTypeWidget::draw(const DrawArgs& args) {
    std::string fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");
    std::shared_ptr<Font> font = APP->window->loadFont(fontPath);

    if (font) {
        nvgFontFaceId(args.vg, font->handle);
        nvgFontSize(args.vg, 13.0);
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
        std::string text = "";
        nvgText(args.vg, 0.0, 10.0, text.c_str(), NULL);
    } else {
        cerr << "failed to load font " << fontPath << endl;
    }
}

void PhyPhoxWidget::setDirty() {
    frameBufferWidget->setDirty();
}

PhyPhoxWidget::PhyPhoxWidget(PhyPhoxSensor* moduleParam) {
    module = moduleParam;
    setModule(module);

    setPanel(createPanel(asset::plugin(pluginInstance, "res/phyphox-sensor.svg")));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(7.625, 42.5)), module, PhyPhoxSensor::STATUS_LIGHT_RED));

    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 52.5)), module, PhyPhoxSensor::OUT_X));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 72.5)), module, PhyPhoxSensor::OUT_Y));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 92.5)), module, PhyPhoxSensor::OUT_Z));

    frameBufferWidget = new FramebufferWidget;
    addChild(frameBufferWidget);

    sensorTypeWidget = createWidget<SensorTypeWidget>(Vec(13.0, 120.0));
    sensorTypeWidget->setSize(Vec(100, 100));
    frameBufferWidget->addChild(sensorTypeWidget);
}

void PhyPhoxWidget::appendContextMenu(Menu* menu) {
    menu->addChild(new MenuSeparator);
    menu->addChild(createMenuLabel("Sensor settings (PhyPhox app)"));

    menu->addChild(createIndexPtrSubmenuItem("Sensor type",
        {
            "Magnetic",
            "Acceleration",
            "Light",
            "Tilt",
            "Sound intensity",
            "Color (HSV converted to RGB)",
            "Gyroscope"
        },
        &module->sensorModeParam
    ));

    menu->addChild(createIndexPtrSubmenuItem("Output voltage mode",
        {
            "Unipolar (0V => +10V)",
            "Bipolar (-5V => +5V)",
        },
        &module->voltageMode
    ));

    IpAddressMenuItem* ipItem = new IpAddressMenuItem;
    ipItem->text = "IP & port";
    ipItem->rightText = module->ip + " " + RIGHT_ARROW;
    ipItem->module = module;
    menu->addChild(ipItem);

    menu->addChild(rack::createBoolPtrMenuItem("Debug Mode", "", &module->debug));
}
