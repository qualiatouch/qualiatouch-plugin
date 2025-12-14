#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;

    p->addModel(modelDmxOut1);
    p->addModel(modelPhyPhoxSensor);
    p->addModel(modelKinectSensor);
}
