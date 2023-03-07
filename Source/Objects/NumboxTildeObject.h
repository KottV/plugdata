/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Utility/DraggableNumber.h"

typedef struct _numbox {
    t_object x_obj;
    t_clock* x_clock_update;
    t_symbol* x_fg;
    t_symbol* x_bg;
    t_glist* x_glist;
    t_float x_display;
    t_float x_in_val;
    t_float x_out_val;
    t_float x_set_val;
    t_float x_max;
    t_float x_min;
    t_float x_sr_khz;
    t_float x_inc;
    t_float x_ramp_step;
    t_float x_ramp_val;
    int x_ramp_ms;
    int x_rate;
    int x_numwidth;
    int x_fontsize;
    int x_clicked;
    int x_width, x_height;
    int x_zoom;
    int x_outmode;
    char x_buf[32]; // number buffer
} t_numbox;

class NumboxTildeObject final : public ObjectBase
    , public Timer {
    DraggableNumber input;

    int nextInterval = 100;
    std::atomic<int> mode = 0;

    Value interval, ramp, init;

    Value min = Value(0.0f);
    Value max = Value(0.0f);

    Value primaryColour;
    Value secondaryColour;

public:
    NumboxTildeObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
        , input(false)
    {
        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();

            if (editor != nullptr) {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };

        input.onEditorHide = [this]() {
            sendFloatValue(input.getText().getFloatValue());
        };

        addAndMakeVisible(input);

        input.setText(input.formatNumber(getValue()), dontSendNotification);

        min = getMinimum();
        max = getMaximum();

        auto* object = static_cast<t_numbox*>(ptr);
        interval = object->x_rate;
        ramp = object->x_ramp_ms;
        init = object->x_set_val;

        primaryColour = "ff" + String::fromUTF8(object->x_fg->s_name + 1);
        secondaryColour = "ff" + String::fromUTF8(object->x_bg->s_name + 1);

        auto fg = Colour::fromString(primaryColour.toString());
        getLookAndFeel().setColour(Label::textColourId, fg);
        getLookAndFeel().setColour(Label::textWhenEditingColourId, fg);
        getLookAndFeel().setColour(TextEditor::textColourId, fg);

        addMouseListener(this, true);

        input.valueChanged = [this](float value) { sendFloatValue(value); };

        mode = static_cast<t_numbox*>(ptr)->x_outmode;

        startTimer(nextInterval);
        repaint();
    }

    Rectangle<int> getPdBounds() override
    {
        pd->lockAudioThread();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        auto bounds = Rectangle<int>(x, y, w, h);

        pd->unlockAudioThread();

        return bounds;
    }

    bool checkBounds(Rectangle<int> oldBounds, Rectangle<int> newBounds, bool resizingOnLeft) override
    {
        auto* nbx = static_cast<t_numbox*>(ptr);

        nbx->x_fontsize = getHeight() - 4;

        int width = newBounds.reduced(Object::margin).getWidth();
        int numWidth = (2.0f * (-6.0f + width - nbx->x_fontsize)) / (4.0f + nbx->x_fontsize);
        width = (nbx->x_fontsize - (nbx->x_fontsize / 2) + 2) * (numWidth + 2) + 2;

        int height = jlimit(18, maxSize, newBounds.getHeight() - Object::doubleMargin);
        if (getWidth() != width || getHeight() != height) {
            object->setSize(width + Object::doubleMargin, height + Object::doubleMargin);
        }

        return true;
    }

    void setPdBounds(Rectangle<int> b) override
    {
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* nbx = static_cast<t_numbox*>(ptr);
        nbx->x_width = b.getWidth();
        nbx->x_height = b.getHeight();
        nbx->x_fontsize = b.getHeight() - 4;

        nbx->x_numwidth = (2.0f * (-6.0f + b.getWidth() - nbx->x_fontsize)) / (4.0f + nbx->x_fontsize);
    }

    void resized() override
    {
        input.setBounds(getLocalBounds().withTrimmedLeft(getHeight() - 4));
        input.setFont(getHeight() - 6);
    }

    ObjectParameters getParameters() override
    {
        return {
            { "Minimum", tFloat, cGeneral, &min, {} },
            { "Maximum", tFloat, cGeneral, &max, {} },
            { "Interval (ms)", tFloat, cGeneral, &interval, {} },
            { "Ramp time (ms)", tFloat, cGeneral, &ramp, {} },
            { "Initial value", tFloat, cGeneral, &init, {} },
            { "Foreground", tColour, cAppearance, &primaryColour, {} },
            { "Background", tColour, cAppearance, &secondaryColour, {} },
        };
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min)) {
            setMinimum(static_cast<float>(min.getValue()));
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(static_cast<float>(max.getValue()));
        } else if (value.refersToSameSourceAs(interval)) {
            auto* nbx = static_cast<t_numbox*>(ptr);
            nbx->x_rate = static_cast<float>(interval.getValue());

        } else if (value.refersToSameSourceAs(ramp)) {
            auto* nbx = static_cast<t_numbox*>(ptr);
            nbx->x_ramp_ms = static_cast<float>(ramp.getValue());
        } else if (value.refersToSameSourceAs(init)) {
            auto* nbx = static_cast<t_numbox*>(ptr);
            nbx->x_set_val = static_cast<float>(init.getValue());
        } else if (value.refersToSameSourceAs(primaryColour)) {
            setForegroundColour(primaryColour.toString());
        } else if (value.refersToSameSourceAs(secondaryColour)) {
            setBackgroundColour(secondaryColour.toString());
        }
    }

    void setForegroundColour(String colour)
    {
        // Remove alpha channel and add #

        ((t_numbox*)ptr)->x_fg = pd->generateSymbol("#" + colour.substring(2));

        auto col = Colour::fromString(colour);
        getLookAndFeel().setColour(Label::textColourId, col);
        getLookAndFeel().setColour(Label::textWhenEditingColourId, col);
        getLookAndFeel().setColour(TextEditor::textColourId, col);

        repaint();
    }

    void setBackgroundColour(String colour)
    {
        ((t_numbox*)ptr)->x_bg = pd->generateSymbol("#" + colour.substring(2));
        repaint();
    }

    void paintOverChildren(Graphics& g) override
    {
        auto iconBounds = Rectangle<int>(2, 0, getHeight(), getHeight());
        Fonts::drawIcon(g, mode ? Icons::ThinDown : Icons::Sine, iconBounds, object->findColour(PlugDataColour::dataColourId));
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colour::fromString(secondaryColour.toString()));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }

    void timerCallback() override
    {
        auto val = getValue();

        if (!mode) {
            input.setText(input.formatNumber(val), dontSendNotification);
        }

        startTimer(nextInterval);
    }

    float getValue()
    {
        auto* obj = static_cast<t_numbox*>(ptr);

        mode = obj->x_outmode;

        nextInterval = obj->x_rate;

        return mode ? obj->x_display : obj->x_in_val;
    }

    float getMinimum()
    {
        return (static_cast<t_numbox*>(ptr))->x_min;
    }

    float getMaximum()
    {
        return (static_cast<t_numbox*>(ptr))->x_max;
    }

    void setMinimum(float minValue)
    {
        static_cast<t_numbox*>(ptr)->x_min = minValue;

        input.setMinimum(minValue);
    }

    void setMaximum(float maxValue)
    {
        static_cast<t_numbox*>(ptr)->x_max = maxValue;

        input.setMaximum(maxValue);
    }
};
