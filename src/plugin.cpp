#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;

    p->addModel(modelDmx1Out);
    p->addModel(modelPhyPhoxSensor);
}
