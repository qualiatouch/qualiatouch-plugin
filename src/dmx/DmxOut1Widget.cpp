#include "DmxOut1Widget.hpp"

DmxOut1Widget::DmxOut1Widget(DmxOut1* moduleParam) {
    module = moduleParam;
    setModule(module);

    setPanel(createPanel(asset::plugin(pluginInstance, "res/dmx-out-1.svg")));

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 42.5)), module, DmxOut1::INPUT_CHANNEL_0));
    addParam(createParamCentered<CKD6>(mm2px(Vec(7.625, 90.0)), module, DmxOut1::BLACKOUT_BUTTON));
    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(7.625, 100.0)), module, DmxOut1::BLACKOUT_LIGHT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 110.0)), module, DmxOut1::INPUT_BLACKOUT));

    addChild(createWidget<ScrewSilver>(Vec(0, 0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH * 2, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

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
    menu->addChild(useOwnDmxAddressItem);

    if (module->useOwnDmxAddress) {
        DmxAddressMenuItem* addressItem = new DmxAddressMenuItem(module);
        menu->addChild(addressItem);
    }

    menu->addChild(rack::createBoolPtrMenuItem("Blackout triggered", "", &module->blackoutTriggered));
    menu->addChild(rack::createBoolPtrMenuItem("Debug", "", &module->debug));

    menu->addChild(new MenuSeparator);
    menu->addChild(createMenuLabel("DMX global settings"));
    menu->addChild(createMenuLabel(" (common to all modules)"));

    DmxUniverseMenuItem* universeItem = new DmxUniverseMenuItem(module);
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
