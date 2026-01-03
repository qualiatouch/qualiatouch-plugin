#pragma once
#include "../plugin.hpp"
#include "AbstractDmxModule.hpp"

using namespace std;

using namespace rack;

struct DmxChannelDisplayWidget : Widget {
    AbstractDmxModule* module;
    FramebufferWidget* parentFrameBufferWidget;

    void setModule(AbstractDmxModule* moduleParam);

    void setParent(FramebufferWidget* parent);

    void step() override;

	void draw(const DrawArgs& args) override;
};
