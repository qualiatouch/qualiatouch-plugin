#include "SonicpulseWash10Widget.hpp"
#include "DmxChannelDisplayWidget.hpp"
#include "SonicpulseWash10.hpp"

using namespace std;

using namespace rack;

SonicpulseWash10Widget::SonicpulseWash10Widget(SonicpulseWash10* moduleParam) {
    module = moduleParam;
    setModule(module);
    dmxModule = moduleParam;

    setPanel(createPanel(asset::plugin(pluginInstance, "res/sonicpulse-wash-10.svg")));

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 30)), dmxModule, SonicpulseWash10::INPUT_CHANNEL_PAN));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625 * 5, 30)), dmxModule, SonicpulseWash10::INPUT_CHANNEL_TILT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625 * 9, 30)), dmxModule, SonicpulseWash10::INPUT_CHANNEL_SPEED));
    
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10, 45)), dmxModule, SonicpulseWash10::INPUT_CHANNEL_DIMMER));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30, 45)), dmxModule, SonicpulseWash10::INPUT_CHANNEL_STROBE));

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 60)), dmxModule, SonicpulseWash10::INPUT_CHANNEL_RED));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625 * 3, 60)), dmxModule, SonicpulseWash10::INPUT_CHANNEL_GREEN));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625 * 5, 60)), dmxModule, SonicpulseWash10::INPUT_CHANNEL_BLUE));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625 * 7, 60)), dmxModule, SonicpulseWash10::INPUT_CHANNEL_WHITE));

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30, 75)), dmxModule, SonicpulseWash10::INPUT_CHANNEL_FUNCTION));

    addParam(createParamCentered<CKD6>(mm2px(Vec(7.625, 90.0)), dmxModule, SonicpulseWash10::BLACKOUT_BUTTON));
    addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(7.625, 100.0)), dmxModule, SonicpulseWash10::BLACKOUT_LIGHT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.625, 110.0)), dmxModule, SonicpulseWash10::INPUT_BLACKOUT));

    addChild(createWidget<ScrewBlack>(Vec(0, 0)));
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH * 14, 0)));
    addChild(createWidget<ScrewBlack>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH * 14, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    frameBufferWidget = new FramebufferWidget;
    addChild(frameBufferWidget);

    dmxChannelDisplayWidget = createWidget<DmxChannelDisplayWidget>(Vec(100, 40));
    dmxChannelDisplayWidget->setModule(dmxModule);
    dmxChannelDisplayWidget->setParent(frameBufferWidget);
    dmxChannelDisplayWidget->setSize(Vec(25, 12));
    frameBufferWidget->addChild(dmxChannelDisplayWidget);
}

SonicpulseWash10Widget::~SonicpulseWash10Widget() {}
