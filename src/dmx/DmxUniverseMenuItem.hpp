#pragma once
#include "../plugin.hpp"
#include "AbstractDmxModule.hpp"

using namespace std;

using namespace rack;

struct DmxUniverseField : ui::TextField {
    AbstractDmxModule* module;

    DmxUniverseField(AbstractDmxModule* moduleParam);

    void onSelectKey(const event::SelectKey& event) override;
};

struct DmxUniverseMenuItem : ui::MenuItem {
    AbstractDmxModule* module;

    Menu* createChildMenu() override;
};
