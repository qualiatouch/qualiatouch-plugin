#include "DmxChannelDisplayWidget.hpp"

void DmxChannelDisplayWidget::setModule(AbstractDmxModule* moduleParam) {
    module = moduleParam;
}

void DmxChannelDisplayWidget::setParent(FramebufferWidget* parent) {
    parentFrameBufferWidget = parent;
}

void DmxChannelDisplayWidget::step() {
    if (!module) {
        return;
    }

    if (module->updateDmxChannelDisplayWidget) {
        module->updateDmxChannelDisplayWidget = false;
        if (module->debug) {
            cout << "updating channel display widget" << endl;
        }
        parentFrameBufferWidget->setDirty();
    }
}

void DmxChannelDisplayWidget::draw(const DrawArgs& args) {
    std::string fontPath = asset::system("res/fonts/ShareTechMono-Regular.ttf");
    std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
    std::string boldFontPath = asset::system("res/fonts/ShareTechMono-Bold.ttf");
    std::shared_ptr<Font> boldFont = APP->window->loadFont(fontPath);

    if (!font) {
        cerr << "failed to load font " << fontPath << endl;
    }
    if (!boldFont) {
        cerr << "failed to load bold font " << boldFontPath << endl;
    }

    nvgFontSize(args.vg, 16.0);
    nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

    char text[4];

    if (!module) {
        nvgText(args.vg, 0.0, 10, "DMX", NULL);
        return;
    }

    if (module->useOwnDmxAddress) {
        nvgFontFaceId(args.vg, boldFont->handle);
        nvgFillColor(args.vg, nvgRGBf(1.f, 1.f, 1.f));
    } else {
        nvgFontFaceId(args.vg, font->handle);
        nvgFillColor(args.vg, nvgRGBf(0.7f, 0.7f, 0.7f));
    }

    snprintf(text, 4, "%03d", module->dmxChannel);

    nvgText(args.vg, 0.0, 12, text, NULL);
}
