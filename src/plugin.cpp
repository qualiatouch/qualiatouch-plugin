#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;

    p->addModel(modelDmxOut1);
    p->addModel(modelDmxOut2);
    p->addModel(modelDmxOut4);
    p->addModel(modelPhoneSensor);
    p->addModel(modelDepthCamSensor);
}
