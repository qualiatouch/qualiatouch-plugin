#pragma once
#include "../plugin.hpp"
#include "AbstractDmxModule.hpp"

using namespace std;

using namespace rack;

struct UseOwnDmxAddressItem : ui::MenuItem {
    AbstractDmxModule* module;

    UseOwnDmxAddressItem(AbstractDmxModule* moduleParam);

    void onAction(const event::Action& e) override;

    void step() override;
};
