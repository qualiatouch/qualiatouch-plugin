#include "UseOwnDmxAddressItem.hpp"

UseOwnDmxAddressItem::UseOwnDmxAddressItem(AbstractDmxModule* moduleParam) {
    module = moduleParam;
    text = "Use own DMX address";

    if (!module->isLeftModuleDmx()) {
        disabled = true;
    }
}

void UseOwnDmxAddressItem::onAction(const event::Action& e) {
    module->toggleUseOwnDmxAddress();
}

void UseOwnDmxAddressItem::step() {
    rightText = module->useOwnDmxAddress ? "✔" : "";
    MenuItem::step();
}
