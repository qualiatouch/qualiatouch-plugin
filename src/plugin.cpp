#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;

    p->addModel(modelDmxOut1);
    p->addModel(modelPhoneSensor);
    p->addModel(modelKinectSensor);
}
