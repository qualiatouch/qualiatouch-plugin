#pragma once
#include "../plugin.hpp"
#include "AbstractDmxModule.hpp"

using namespace std;

using namespace rack;

struct DmxAddressField : ui::TextField {
    AbstractDmxModule* module;

    DmxAddressField(AbstractDmxModule* moduleParam);

    void onSelectKey(const event::SelectKey& event) override;
};

struct DmxAddressMenuItem : ui::MenuItem {
    AbstractDmxModule* module;

    Menu* createChildMenu() override;
};
