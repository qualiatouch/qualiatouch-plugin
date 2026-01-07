#include "AbstractDmxModuleWidget.hpp"

void AbstractDmxModuleWidget::appendContextMenu(Menu* menu) {
    menu->addChild(new MenuSeparator);
    menu->addChild(createMenuLabel("DMX module settings"));

    UseOwnDmxAddressItem* useOwnDmxAddressItem = new UseOwnDmxAddressItem(dmxModule);
    menu->addChild(useOwnDmxAddressItem);

    if (dmxModule->getUseOwnDmxAddress()) {
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

    menu->addChild(rack::createBoolPtrMenuItem("Keep sending when disconnected", "", &DmxRegistry::instance().keepSendingWhenNotConnected));

    // debug info
    if (dmxModule->debug) {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Debug info"));
        menu->addChild(createMenuLabel("Id " + std::to_string(dmxModule->getId())));
        menu->addChild(createMenuLabel("Master " + std::to_string(dmxModule->isMaster())));
        menu->addChild(createMenuLabel("Chain size " + std::to_string(dmxModule->getModuleChainSize())));
        menu->addChild(createMenuLabel("Channel " + std::to_string(dmxModule->getDmxChannel())));
        menu->addChild(createMenuLabel("Use own address " + std::to_string(dmxModule->getUseOwnDmxAddress())));
        menu->addChild(rack::createBoolPtrMenuItem("Debug Chain", "", &dmxModule->debugChain));
        menu->addChild(rack::createBoolPtrMenuItem("Debug Registry", "", &DmxRegistry::instance().debug));
    }
}
