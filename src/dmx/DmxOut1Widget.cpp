#include "DmxOut1Widget.hpp"

DmxOut1Widget::DmxOut1Widget(DmxOut1* moduleParam) {
    module = moduleParam;
    setModule(module);

    setPanel(createPanel(asset::plugin(pluginInstance, "res/dmx-out-1.svg")));

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 42.5)), module, DmxOut1::INPUT_CHANNEL_0));
    addParam(createParamCentered<CKD6>(mm2px(Vec(7.625, 90.0)), module, DmxOut1::BLACKOUT_BUTTON));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 107.5)), module, DmxOut1::INPUT_BLACKOUT));
    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(7.625, 100.0)), module, DmxOut1::BLACKOUT_LIGHT));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    frameBufferWidget = new FramebufferWidget;
    addChild(frameBufferWidget);

    dmxChannelDisplayWidget = createWidget<DmxChannelDisplayWidget>(Vec(11,150));
    dmxChannelDisplayWidget->setModule(module);
    dmxChannelDisplayWidget->setParent(frameBufferWidget);
    dmxChannelDisplayWidget->setSize(Vec(25, 12));
    frameBufferWidget->addChild(dmxChannelDisplayWidget);
}

void DmxOut1Widget::appendContextMenu(Menu* menu) {
    menu->addChild(new MenuSeparator);
    menu->addChild(createMenuLabel("DMX module settings"));

    UseOwnDmxAddressItem* useOwnDmxAddressItem = new UseOwnDmxAddressItem(module);
    useOwnDmxAddressItem->text = "Use own DMX address";

    if (!module->isLeftModuleDmx()) {
        useOwnDmxAddressItem->disabled = true;
    }
    menu->addChild(useOwnDmxAddressItem);

    if (module->useOwnDmxAddress) {
        DmxAddressMenuItem* addressItem = new DmxAddressMenuItem;
        addressItem->text = "DMX Address";
        addressItem->rightText = std::to_string(module->dmxAddress) + " " + RIGHT_ARROW;
        addressItem->module = module;
        menu->addChild(addressItem);
    }

    menu->addChild(rack::createBoolPtrMenuItem("Blackout triggered", "", &module->blackoutTriggered));
    menu->addChild(rack::createBoolPtrMenuItem("Debug", "", &module->debug));

    menu->addChild(new MenuSeparator);
    menu->addChild(createMenuLabel("DMX universe settings"));
    menu->addChild(createMenuLabel(" (common to all modules)"));
    DmxUniverseMenuItem* universeItem = new DmxUniverseMenuItem;
    universeItem->text = "DMX universe";
    universeItem->rightText = std::to_string(module->getDmxUniverse()) + " " + RIGHT_ARROW;
    universeItem->module = module;
    menu->addChild(universeItem);

    // debug info
    if (module->debug) {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Debug info"));
        menu->addChild(createMenuLabel("Id " + std::to_string(module->getId())));
        menu->addChild(createMenuLabel("Master " + std::to_string(module->isMaster())));
        menu->addChild(createMenuLabel("Chain size " + std::to_string(module->moduleChainSize)));
        menu->addChild(createMenuLabel("Channel " + std::to_string(module->dmxChannel)));
        menu->addChild(createMenuLabel("Use own address " + std::to_string(module->useOwnDmxAddress)));
        menu->addChild(rack::createBoolPtrMenuItem("Debug Chain", "", &module->debugChain));
    }
}

UseOwnDmxAddressItem::UseOwnDmxAddressItem(DmxOut1* moduleParam) {
    module = moduleParam;
}

void UseOwnDmxAddressItem::onAction(const event::Action& e) {
    module->toggleUseOwnDmxAddress();
}

void UseOwnDmxAddressItem::step() {
    rightText = module->useOwnDmxAddress ? "✔" : "";
    MenuItem::step();
}

DmxAddressField::DmxAddressField(DmxOut1* moduleParam) {
    module = moduleParam;
    box.size.x = 100;
    placeholder = "0";
}

void DmxAddressField::onSelectKey(const event::SelectKey& event) {
    if (event.action == GLFW_PRESS && event.key == GLFW_KEY_ENTER) {
        int dmxAddress;
        try {
            dmxAddress = std::stoi(text);
        } catch (const std::exception& e) {
            event.consume(this);
            return;
        }
        if (module) {
            module->dmxAddress = dmxAddress;
            module->dmxChannel = dmxAddress;
            module->recalculateChain = true;
        }
        ui::MenuOverlay* overlay = getAncestorOfType<ui::MenuOverlay>();
        if (overlay) {
            overlay->requestDelete();
        }
        event.consume(this);
    }
    if (!event.getTarget()) {
        TextField::onSelectKey(event);
    }
}

Menu* DmxAddressMenuItem::createChildMenu() {
    Menu* menu = new Menu;

    DmxAddressField* addressField = new DmxAddressField(module);
    addressField->text = std::to_string(module->dmxAddress);
    menu->addChild(addressField);

    return menu;
}

DmxUniverseField::DmxUniverseField(DmxOut1* moduleParam) {
    module = moduleParam;
    box.size.x = 100;
    placeholder = "1";
}

void DmxUniverseField::onSelectKey(const event::SelectKey& event) {
    if (event.action == GLFW_PRESS && event.key == GLFW_KEY_ENTER) {
        int dmxUniverse;
        try {
            dmxUniverse = std::stoi(text);
        } catch (const std::exception& e) {
            event.consume(this);
            return;
        }
        if (module) {
            module->setDmxUniverse(dmxUniverse);
        }
        ui::MenuOverlay* overlay = getAncestorOfType<ui::MenuOverlay>();
        if (overlay) {
            overlay->requestDelete();
        }
        event.consume(this);
    }
    if (!event.getTarget()) {
        TextField::onSelectKey(event);
    }
}

Menu* DmxUniverseMenuItem::createChildMenu() {
    Menu* menu = new Menu;

    DmxUniverseField* universeField = new DmxUniverseField(module);
    universeField->text = std::to_string(module->getDmxUniverse());
    menu->addChild(universeField);

    return menu;
}

void DmxChannelDisplayWidget::setModule(DmxOut1* moduleParam) {
    module = moduleParam;
}

void DmxChannelDisplayWidget::setParent(FramebufferWidget* parent) {
    parentFrameBufferWidget = parent;
}

void DmxChannelDisplayWidget::step() {
    if (!module) {
        return;
    }

    if (module->updateDmxChannelDisplayWidget) {
        module->updateDmxChannelDisplayWidget = false;
        if (module->debug) {
            cout << "updating channel display widget" << endl;
        }
        parentFrameBufferWidget->setDirty();
    }
}

void DmxChannelDisplayWidget::draw(const DrawArgs& args) {
    std::string fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");
    std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
    std::string boldFontPath = asset::system("res/fonts/ShareTechMono-Bold.ttf");
    std::shared_ptr<Font> boldFont = APP->window->loadFont(fontPath);

    if (!font) {
        cerr << "failed to load font " << fontPath << endl;
    }
    if (!boldFont) {
        cerr << "failed to load bold font " << boldFontPath << endl;
    }

    nvgFontSize(args.vg, 16.0);
    nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

    char text[4];

    if (!module) {
        nvgText(args.vg, 0.0, 10, "DMX", NULL);
        return;
    }

    if (module->useOwnDmxAddress) {
        nvgFontFaceId(args.vg, boldFont->handle);
        nvgFillColor(args.vg, nvgRGBf(1.f, 1.f, 1.f));
    } else {
        nvgFontFaceId(args.vg, font->handle);
        nvgFillColor(args.vg, nvgRGBf(0.7f, 0.7f, 0.7f));
    }

    snprintf(text, 4, "%03d", module->dmxChannel);

    nvgText(args.vg, 0.0, 12, text, NULL);
}
