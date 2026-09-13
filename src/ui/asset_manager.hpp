#pragma once
#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace warlock {

class AssetManager {
private:
    std::unordered_map<std::string, Texture2D> textures_;
    std::unordered_map<std::string, std::string> file_lookup_;
    Texture2D fallback_texture_;
    std::string assets_dir_;
    bool initialized_ = false;

    static std::string to_lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return s;
    }

    static std::string strip_ext(const std::string& filename) {
        size_t dot = filename.rfind('.');
        if (dot != std::string::npos) {
            return filename.substr(0, dot);
        }
        return filename;
    }

    void scan_assets_dir() {
        namespace fs = std::filesystem;

        // Candidate search paths
        std::vector<std::string> candidates = {
            "assets",
            "../assets",
            "/home/deck/warlock_sim/assets"
        };
        const char* app_dir = GetApplicationDirectory();
        if (app_dir && app_dir[0] != '\0') {
            candidates.push_back(std::string(app_dir) + "assets");
            candidates.push_back(std::string(app_dir) + "../assets");
        }

        for (const auto& cand : candidates) {
            if (fs::exists(cand) && fs::is_directory(cand)) {
                assets_dir_ = cand;
                break;
            }
        }

        if (assets_dir_.empty()) {
            assets_dir_ = "assets";
        }

        // Index all icons (over 33,000 WoW icons)
        std::string icons_dir = assets_dir_ + "/icons";
        if (fs::exists(icons_dir)) {
            for (const auto& entry : fs::directory_iterator(icons_dir)) {
                if (!entry.is_regular_file()) continue;
                std::string filename = entry.path().filename().string();
                std::string full_path = entry.path().string();
                std::string lower_fn = to_lower(filename);
                std::string lower_stem = to_lower(entry.path().stem().string());

                file_lookup_[lower_fn] = full_path;
                file_lookup_[lower_stem] = full_path;
            }
        }

        // Index character models
        std::string models_dir = assets_dir_ + "/models";
        if (fs::exists(models_dir)) {
            for (const auto& entry : fs::directory_iterator(models_dir)) {
                if (!entry.is_regular_file()) continue;
                std::string filename = entry.path().filename().string();
                std::string full_path = entry.path().string();
                std::string lower_fn = to_lower(filename);
                std::string lower_stem = to_lower(entry.path().stem().string());

                file_lookup_[lower_fn] = full_path;
                file_lookup_[lower_stem] = full_path;
            }
        }

        // Index background images
        std::string bg_dir = assets_dir_ + "/backgrounds";
        if (fs::exists(bg_dir)) {
            for (const auto& entry : fs::directory_iterator(bg_dir)) {
                if (!entry.is_regular_file()) continue;
                std::string filename = entry.path().filename().string();
                std::string full_path = entry.path().string();
                std::string lower_fn = to_lower(filename);
                std::string lower_stem = to_lower(entry.path().stem().string());

                file_lookup_[lower_fn] = full_path;
                file_lookup_[lower_stem] = full_path;
            }
        }
    }

public:
    static AssetManager& get() {
        static AssetManager instance;
        return instance;
    }

    void init() {
        if (initialized_) return;

        scan_assets_dir();

        // Create a 40x40 dark purple fallback slot texture
        Image img = GenImageColor(40, 40, Color{ 35, 20, 50, 255 });
        ImageDrawRectangleLines(&img, Rectangle{ 0, 0, 40, 40 }, 1, Color{ 100, 60, 140, 255 });
        fallback_texture_ = LoadTextureFromImage(img);
        UnloadImage(img);

        initialized_ = true;
    }

    void shutdown() {
        if (!initialized_) return;
        for (auto& pair : textures_) {
            if (pair.second.id > 0) {
                UnloadTexture(pair.second);
            }
        }
        textures_.clear();
        file_lookup_.clear();
        if (fallback_texture_.id > 0) {
            UnloadTexture(fallback_texture_);
        }
        initialized_ = false;
    }

    const Texture2D& get_icon(const std::string& icon_name) {
        if (icon_name.empty()) return fallback_texture_;

        std::string key = to_lower(icon_name);
        auto it = textures_.find(key);
        if (it != textures_.end()) {
            return it->second;
        }

        // Check file lookup by full name or stripped stem
        std::string path;
        auto fit = file_lookup_.find(key);
        if (fit != file_lookup_.end()) {
            path = fit->second;
        } else {
            std::string stem = to_lower(strip_ext(icon_name));
            fit = file_lookup_.find(stem);
            if (fit != file_lookup_.end()) {
                path = fit->second;
            } else {
                fit = file_lookup_.find(stem + ".png");
                if (fit != file_lookup_.end()) {
                    path = fit->second;
                }
            }
        }

        if (!path.empty()) {
            Texture2D tex = LoadTexture(path.c_str());
            if (tex.id > 0) {
                SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
                textures_[key] = tex;
                return textures_[key];
            } else {
                // If JPG failed to load (e.g. unsupported format in raylib build), try .png sibling
                std::string stem = to_lower(strip_ext(icon_name));
                auto png_fit = file_lookup_.find(stem + ".png");
                if (png_fit != file_lookup_.end()) {
                    tex = LoadTexture(png_fit->second.c_str());
                    if (tex.id > 0) {
                        SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
                        textures_[key] = tex;
                        return textures_[key];
                    }
                }
            }
        }

        return fallback_texture_;
    }

    const Texture2D& get_model(const std::string& model_name) {
        return get_icon(model_name);
    }

    const Texture2D& get_fallback() const {
        return fallback_texture_;
    }
};

} // namespace warlock
