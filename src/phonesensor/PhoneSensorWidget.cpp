#include "PhoneSensorWidget.hpp"

IpAddressField::IpAddressField(PhoneSensor* moduleParam) {
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
    ipField->text = module->getIpAddress();
    menu->addChild(ipField);

    return menu;
}

SensorLimitField::SensorLimitField(PhoneSensor* moduleParam, SensorLimitFieldName fieldNameParam) {
    module = moduleParam;
    fieldName = fieldNameParam;
    box.size.x = 200;
    placeholder = "";
}

void SensorLimitField::onSelectKey(const event::SelectKey& e) {
    if (e.action == GLFW_PRESS && e.key == GLFW_KEY_ENTER) {
        std::string ip = text;
        if (module) {
            switch (fieldName) {
                case MIN_X:
                    module->setSensorMinX(std::stof(text));
                    break;
                case MAX_X:
                    module->setSensorMaxX(std::stof(text));
                    break;
                case MIN_Y:
                    module->setSensorMinY(std::stof(text));
                    break;
                case MAX_Y:
                    module->setSensorMaxY(std::stof(text));
                    break;
                case MIN_Z:
                    module->setSensorMinZ(std::stof(text));
                    break;
                case MAX_Z:
                    module->setSensorMaxZ(std::stof(text));
                    break;
                default:
                    break;
            }
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

SensorLimitMenuItem::SensorLimitMenuItem(PhoneSensor* moduleParam, SensorLimitFieldName fieldNameParam, float currentValueParam) {
    module = moduleParam;
    fieldName = fieldNameParam;
    currentValue = currentValueParam;
    text = getDisplayText(fieldNameParam);
    rightText = to_string(currentValue) + " " + RIGHT_ARROW;
}

std::string SensorLimitMenuItem::getDisplayText(SensorLimitFieldName name) {
    switch (name) {
        case MIN_X:
            return "Minimum X value";
        case MAX_X:
            return "Maximum X value";
        case MIN_Y:
            return "Minimum Y value";
        case MAX_Y:
            return "Maximum Y value";
        case MIN_Z:
            return "Minimum Z value";
        case MAX_Z:
            return "Maximum Z value";
        default:
            return "";
    }
}

Menu* SensorLimitMenuItem::createChildMenu() {
    Menu* menu = new Menu;

    SensorLimitField* field = new SensorLimitField(module, fieldName);
    field->text = to_string(currentValue);
    menu->addChild(field);

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

void PhoneSensorWidget::setDirty() {
    frameBufferWidget->setDirty();
}

PhoneSensorWidget::PhoneSensorWidget(PhoneSensor* moduleParam) {
    module = moduleParam;
    setModule(module);

    setPanel(createPanel(asset::plugin(pluginInstance, "res/phone-sensor.svg")));

    addChild(createWidget<ScrewBlack>(Vec(0, 0)));
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH * 2, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(7.625, 42.5)), module, PhoneSensor::STATUS_LIGHT_RED));

    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 52.5)), module, PhoneSensor::OUT_X));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 72.5)), module, PhoneSensor::OUT_Y));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.625, 92.5)), module, PhoneSensor::OUT_Z));

    frameBufferWidget = new FramebufferWidget;
    addChild(frameBufferWidget);

    sensorTypeWidget = createWidget<SensorTypeWidget>(Vec(13.0, 120.0));
    sensorTypeWidget->setSize(Vec(100, 100));
    frameBufferWidget->addChild(sensorTypeWidget);
}

void PhoneSensorWidget::appendContextMenu(Menu* menu) {
    menu->addChild(new MenuSeparator);
    menu->addChild(createMenuLabel("PhyPhox app settings"));

    IpAddressMenuItem* ipItem = new IpAddressMenuItem;
    ipItem->text = "IP & port";
    ipItem->rightText = module->getIpAddress() + " " + RIGHT_ARROW;
    ipItem->module = module;
    menu->addChild(ipItem);

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

    menu->addChild(new MenuSeparator);
    menu->addChild(createMenuLabel("Sensor limit settings"));

    menu->addChild(new SensorLimitMenuItem(module, MIN_X, module->getSensorMinX()));
    menu->addChild(new SensorLimitMenuItem(module, MAX_X, module->getSensorMaxX()));
    menu->addChild(new SensorLimitMenuItem(module, MIN_Y, module->getSensorMinY()));
    menu->addChild(new SensorLimitMenuItem(module, MAX_Y, module->getSensorMaxY()));
    menu->addChild(new SensorLimitMenuItem(module, MIN_Z, module->getSensorMinZ()));
    menu->addChild(new SensorLimitMenuItem(module, MAX_Z, module->getSensorMaxZ()));

    menu->addChild(new MenuSeparator);
    menu->addChild(createMenuLabel("Output settings"));

    menu->addChild(createIndexPtrSubmenuItem("Output voltage mode",
        {
            "Unipolar (0V => +10V)",
            "Bipolar (-5V => +5V)",
        },
        &module->voltageMode
    ));

    menu->addChild(rack::createBoolPtrMenuItem("Debug Mode", "", &module->debug));
}
