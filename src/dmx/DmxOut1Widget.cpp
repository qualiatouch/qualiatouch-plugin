#include "DmxOut1Widget.hpp"
#include "DmxChannelDisplayWidget.hpp"
#include "DmxOut1.hpp"

using namespace std;

using namespace rack;

DmxOut1Widget::DmxOut1Widget(DmxOut1* moduleParam) {
    module = moduleParam;
    setModule(module);
    dmxModule = moduleParam;

    setPanel(createPanel(asset::plugin(pluginInstance, "res/dmx-out-1.svg")));

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 40)), dmxModule, DmxOut1::INPUT_CHANNEL_0));

    addParam(createParamCentered<CKD6>(mm2px(Vec(7.625, 100.0)), dmxModule, DmxOut1::BLACKOUT_BUTTON));
    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(7.625, 107.5)), dmxModule, DmxOut1::BLACKOUT_LIGHT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 115.0)), dmxModule, DmxOut1::INPUT_BLACKOUT));

    addChild(createWidget<ScrewBlack>(Vec(0, 0)));
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH * 2, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    frameBufferWidget = new FramebufferWidget;
    addChild(frameBufferWidget);

    dmxChannelDisplayWidget = createWidget<DmxChannelDisplayWidget>(Vec(11,85));
    dmxChannelDisplayWidget->setModule(dmxModule);
    dmxChannelDisplayWidget->setParent(frameBufferWidget);
    dmxChannelDisplayWidget->setSize(Vec(25, 12));
    frameBufferWidget->addChild(dmxChannelDisplayWidget);
}

DmxOut1Widget::~DmxOut1Widget() {}
