#pragma once

#include "internal/ui/widget.h"

namespace ui {

enum TextAlignment {
    LEFT,
    CENTER,
    RIGHT
};

constexpr mkb::Rgb24 DEFAULT_YELLOW = {0xFF, 0xFF, 0x00};
constexpr mkb::Rgb24 UNSELECTED_YELLOW = {0x80, 0x80, 0x00};
constexpr mkb::Rgb24 DEFAULT_ORANGE = {0xFF, 0x80, 0x00};

class Text : public Widget {
public:
    Text(const char* text);
    Text(const char* text, const Vec2d pos);
    ~Text();
    virtual void disp() override;

protected:
    const char* m_text;
    alignas(4) mkb::FontStyle m_font_style = 0x0;
    int m_spacing = 1;
    alignas(4) mkb::Rgb24 m_color = DEFAULT_ORANGE;
    alignas(4) bool m_drop_shadow = true;
    alignas(4) mkb::SpriteAlignment m_alignment = mkb::ALIGN_LOWER_CENTER;
    alignas(4) Vec2d m_offset = Vec2d{0.f, 0.f};

public:
    mkb::FontStyle get_m_font_style() const {
        return m_font_style;
    }
    void set_font_style(mkb::FontStyle m_font_style) {
        Text::m_font_style = m_font_style;
    }
    int get_spacing() const {
        return m_spacing;
    }
    void set_spacing(int spacing) {
        m_spacing = spacing;
    }
    const mkb::Rgb24& get_color() const {
        return m_color;
    }
    void set_color(const mkb::Rgb24& color) {
        m_color = color;
    }
    bool has_drop_shadow() const {
        return m_drop_shadow;
    }
    void set_drop_shadow(bool drop_shadow) {
        m_drop_shadow = drop_shadow;
    }
    mkb::SpriteAlignment get_alignment() const {
        return m_alignment;
    }
    // Basic alignment helper
    // Left - the text will be left-aligned and continue to the right of the widget origin
    // Center - the text will be center-aligned and expand on both sides of the widget origin, and the widget origin is in the middle of the text
    // Right - the text will be right-aligned and continue to the left of the widget origin
    void set_alignment(ui::TextAlignment alignment) {
        switch (alignment) {
            case LEFT:
                m_alignment = mkb::ALIGN_LOWER_RIGHT;
                break;
            case CENTER:
                // Ditto
                m_alignment = mkb::ALIGN_CENTER;
                break;
            case RIGHT:
                // Ditto
                m_alignment = mkb::ALIGN_LOWER_LEFT;
                break;
        }
    }

    void set_alignment(mkb::SpriteAlignment alignment) {
        m_alignment = alignment;
    }
    const Vec2d& get_offset() const {
        return m_offset;
    }
    void set_offset(const Vec2d& offset) {
        m_offset = offset;
    }
};

}// namespace ui
