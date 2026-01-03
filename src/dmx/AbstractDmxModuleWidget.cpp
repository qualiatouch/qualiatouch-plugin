#include "AbstractDmxModuleWidget.hpp"

void AbstractDmxModuleWidget::appendContextMenu(Menu* menu) {
    menu->addChild(new MenuSeparator);
    menu->addChild(createMenuLabel("DMX module settings"));

    UseOwnDmxAddressItem* useOwnDmxAddressItem = new UseOwnDmxAddressItem(dmxModule);
    menu->addChild(useOwnDmxAddressItem);

    if (dmxModule->useOwnDmxAddress) {
        DmxAddressMenuItem* addressItem = new DmxAddressMenuItem(dmxModule);
        menu->addChild(addressItem);
    }

    menu->addChild(rack::createBoolPtrMenuItem("Blackout triggered", "", &dmxModule->blackoutTriggered));
    menu->addChild(rack::createBoolPtrMenuItem("Debug", "", &dmxModule->debug));

    menu->addChild(new MenuSeparator);
    menu->addChild(createMenuLabel("DMX global settings"));
    menu->addChild(createMenuLabel(" (common to all DMX modules)"));

    DmxUniverseMenuItem* universeItem = new DmxUniverseMenuItem(dmxModule);
    menu->addChild(universeItem);
    
    // debug info
    if (dmxModule->debug) {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Debug info"));
        menu->addChild(createMenuLabel("Id " + std::to_string(dmxModule->getId())));
        menu->addChild(createMenuLabel("Master " + std::to_string(dmxModule->isMaster())));
        menu->addChild(createMenuLabel("Chain size " + std::to_string(dmxModule->moduleChainSize)));
        menu->addChild(createMenuLabel("Channel " + std::to_string(dmxModule->dmxChannel)));
        menu->addChild(createMenuLabel("Use own address " + std::to_string(dmxModule->useOwnDmxAddress)));
        menu->addChild(rack::createBoolPtrMenuItem("Debug Chain", "", &dmxModule->debugChain));
        menu->addChild(rack::createBoolPtrMenuItem("Debug Registry", "", &DmxRegistry::instance().debug));
    }
}
