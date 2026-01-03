#include "UseOwnDmxAddressItem.hpp"

UseOwnDmxAddressItem::UseOwnDmxAddressItem(AbstractDmxModule* moduleParam) {
    module = moduleParam;
}

void UseOwnDmxAddressItem::onAction(const event::Action& e) {
    module->toggleUseOwnDmxAddress();
}

void UseOwnDmxAddressItem::step() {
    rightText = module->useOwnDmxAddress ? "✔" : "";
    MenuItem::step();
}
