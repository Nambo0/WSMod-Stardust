#include "widget_sprite.h"
#include "log.h"

namespace ui {

Sprite::Sprite(uint32_t sprite_id, const Vec2d dimensions) : Widget(), m_sprite_id(sprite_id) {
    m_dimensions = dimensions;
}

Sprite::Sprite(uint32_t sprite_id, const Vec2d pos, const Vec2d dimensions) : Widget(pos), m_sprite_id(sprite_id) {
    m_dimensions = dimensions;
}

Sprite::~Sprite() = default;

void Sprite::disp() {
    mkb::SpriteDrawRequest req;
    mkb::GXSetZMode_cached('\x01', mkb::GX_LEQUAL, '\x01');
    mkb::g_scale_sprite_for_widescreen(0x140);
    req.id = m_sprite_id;
    req.pos.x = (m_pos.x + m_dimensions.x / 2) + m_offset.x;
    req.pos.y = (m_pos.y + m_dimensions.y / 2) + m_offset.y;
    req.pos.z = m_depth;
    req.scale.x = m_scale.x;
    req.scale.y = m_scale.y;
    req.mult_color = (m_mult_color.red << 16) + (m_mult_color.green << 8) + m_mult_color.blue;
    req.add_color = (m_add_color.red << 16) + (m_add_color.green << 8) + m_add_color.blue;
    req.u1 = m_uv_1.x;
    req.v1 = m_uv_1.y;
    req.u2 = m_uv_2.x;
    req.v2 = m_uv_2.y;
    req.rot_z = m_z_rotation;
    req.alpha = m_alpha;
    req.g_unk1 = -1;

    if (!m_mirror) {
        req.flags = (0x20000) & 0xfffffffff0 | 10;
    }
    else {
        req.flags = (0x20000) & 0xfffffffff0 | 0x8000a;
    }

    mkb::GXSetZMode_cached('\x01', mkb::GX_ALWAYS, '\x01');
    mkb::draw_sprite_draw_request(&req);
    mkb::g_reset_sprite_mtx_for_widescreen();
}
}// namespace ui