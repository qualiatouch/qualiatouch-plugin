#include "DmxAddressMenuItem.hpp"

DmxAddressField::DmxAddressField(AbstractDmxModule* moduleParam) {
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

DmxAddressMenuItem::DmxAddressMenuItem(AbstractDmxModule* moduleParam) {
    module = moduleParam;
    text = "DMX Address";
    rightText = std::to_string(module->dmxAddress) + " " + RIGHT_ARROW;
}

Menu* DmxAddressMenuItem::createChildMenu() {
    Menu* menu = new Menu;

    DmxAddressField* addressField = new DmxAddressField(module);
    addressField->text = std::to_string(module->dmxAddress);
    menu->addChild(addressField);

    return menu;
}
