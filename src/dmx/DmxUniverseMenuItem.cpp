#include "DmxUniverseMenuItem.hpp"

DmxUniverseField::DmxUniverseField(AbstractDmxModule* moduleParam) {
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
