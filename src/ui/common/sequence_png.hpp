#pragma once

#include "raylib.h"
#include "asset_manager.hpp"
#include "src/sim/warlock/warlock_sim.hpp"
#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace warlock {

inline bool export_spell_sequence_png(const std::vector<SpellCastLog>& sequence,
                                     const std::string& title,
                                     const std::string& filename) {
    constexpr int width = 1280;
    constexpr int header_height = 112;
    constexpr int row_height = 40;
    constexpr int max_dimension = 16000;
    const size_t visible_rows = std::min(sequence.size(), static_cast<size_t>((max_dimension - header_height) / row_height));
    const int height = header_height + static_cast<int>(visible_rows) * row_height;
    Image image = GenImageColor(width, height, Color{16, 20, 29, 255});
    ImageDrawRectangle(&image, 0, 0, width, 94, Color{29, 36, 49, 255});
    ImageDrawRectangle(&image, 0, 91, width, 3, Color{180, 139, 58, 255});
    ImageDrawText(&image, title.c_str(), 28, 18, 28, Color{255, 222, 145, 255});
    ImageDrawText(&image, TextFormat("%zu events  |  Time ordered  |  Damage and critical hits shown per event", sequence.size()),
                  30, 58, 16, Color{177, 190, 210, 255});
    ImageDrawText(&image, "#", 28, 98, 13, Color{129, 151, 177, 255});
    ImageDrawText(&image, "TIME", 82, 98, 13, Color{129, 151, 177, 255});
    ImageDrawText(&image, "SPELL", 175, 98, 13, Color{129, 151, 177, 255});
    ImageDrawText(&image, "EVENT", 495, 98, 13, Color{129, 151, 177, 255});
    ImageDrawText(&image, "RESULT", 700, 98, 13, Color{129, 151, 177, 255});
    ImageDrawText(&image, "DETAIL", 870, 98, 13, Color{129, 151, 177, 255});

    std::map<int, Image> icon_images;
    for (size_t i = 0; i < visible_rows; ++i) {
        const auto& event = sequence[i];
        const int y = header_height + static_cast<int>(i) * row_height;
        if ((i & 1u) == 0) ImageDrawRectangle(&image, 16, y, width - 32, row_height, Color{24, 29, 40, 255});
        if (i > 0) ImageDrawLine(&image, 24, y, width - 24, y, Color{42, 49, 63, 255});

        const int icon_key = static_cast<int>(event.spell_id);
        auto icon_it = icon_images.find(icon_key);
        if (icon_it == icon_images.end()) {
            const Texture2D& texture = AssetManager::get().get_icon(spell_id_to_icon(event.spell_id));
            icon_it = icon_images.emplace(icon_key, LoadImageFromTexture(texture)).first;
        }
        ImageDrawImagePro(&image, icon_it->second,
                          Rectangle{0, 0, static_cast<float>(icon_it->second.width), static_cast<float>(icon_it->second.height)},
                          Rectangle{125, static_cast<float>(y + 4), 32, 32}, Vector2{0, 0}, 0.0f, WHITE);

        ImageDrawText(&image, TextFormat("%zu", i + 1), 28, y + 12, 14, Color{133, 151, 176, 255});
        ImageDrawText(&image, TextFormat("%.1fs", event.time), 82, y + 12, 15, Color{191, 205, 224, 255});
        ImageDrawText(&image, spell_id_to_name(event.spell_id), 175, y + 12, 15, Color{238, 241, 247, 255});
        ImageDrawText(&image, event.event_type.c_str(), 495, y + 12, 14, Color{161, 180, 205, 255});
        if (event.damage > 0.0) {
            const char* result = TextFormat("%.0f %s", event.damage, event.is_crit ? "CRIT" : "damage");
            ImageDrawText(&image, result, 700, y + 12, 15,
                          event.is_crit ? Color{255, 202, 89, 255} : Color{127, 220, 158, 255});
        } else if (event.is_miss) {
            ImageDrawText(&image, "MISS", 700, y + 12, 15, Color{255, 119, 119, 255});
        } else {
            ImageDrawText(&image, "—", 700, y + 12, 15, Color{130, 143, 161, 255});
        }
        std::string detail = event.tag == event.event_type ? "" : event.tag;
        if (detail.size() > 42) detail.resize(39), detail += "...";
        if (!detail.empty()) ImageDrawText(&image, detail.c_str(), 870, y + 12, 13, Color{160, 172, 191, 255});
    }

    const bool saved = ExportImage(image, filename.c_str());
    for (auto& icon : icon_images) UnloadImage(icon.second);
    UnloadImage(image);
    return saved;
}

} // namespace warlock
