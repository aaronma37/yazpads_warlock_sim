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

    void index_directory_recursive(const std::string& dir_path, int max_depth = 4, int current_depth = 0) {
        namespace fs = std::filesystem;
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path) || current_depth > max_depth) return;

        try {
            for (const auto& entry : fs::directory_iterator(dir_path)) {
                if (entry.is_directory()) {
                    index_directory_recursive(entry.path().string(), max_depth, current_depth + 1);
                } else if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    std::string full_path = entry.path().string();
                    std::string lower_fn = to_lower(filename);
                    std::string lower_stem = to_lower(entry.path().stem().string());

                    file_lookup_[lower_fn] = full_path;
                    file_lookup_[lower_stem] = full_path;

                    // Also index relative to subfolder (e.g. "buttons/ui-panel-button-up")
                    std::string parent_name = to_lower(entry.path().parent_path().filename().string());
                    if (!parent_name.empty()) {
                        file_lookup_[parent_name + "/" + lower_fn] = full_path;
                        file_lookup_[parent_name + "/" + lower_stem] = full_path;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[AssetManager] Warning indexing " << dir_path << ": " << e.what() << std::endl;
        }
    }

    void scan_assets_dir() {
        namespace fs = std::filesystem;

        // Candidate search paths (portable: CWD, web FS root, parent, binary dir)
        std::vector<std::string> candidates = {
            "assets",
            "/assets",
            "../assets"
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

        // Index external Blizzard WoW UI textures first as a fallback. The
        // vendored copies under <assets>/wow_classic are indexed second so
        // they take precedence when both exist.
        std::vector<std::string> wow_texture_dirs = {
            "../wow_assets/classic",
            "../wow_assets/wow-ui-textures",
            "wow_assets/classic",
            "wow_assets/wow-ui-textures"
        };
        const char* home_env = std::getenv("HOME");
        if (home_env && home_env[0] != '\0') {
            wow_texture_dirs.push_back(std::string(home_env) + "/wow_assets/classic");
            wow_texture_dirs.push_back(std::string(home_env) + "/wow_assets/wow-ui-textures");
        }
        for (const auto& w_dir : wow_texture_dirs) {
            if (fs::exists(w_dir)) {
                std::cout << "[AssetManager] Indexing WoW UI textures from: " << w_dir << std::endl;
                index_directory_recursive(w_dir, 4);
                break;
            }
        }

        // Index project assets (icons, models, backgrounds, wow_classic, etc.)
        index_directory_recursive(assets_dir_);
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
        ImageDrawRectangleLines(&img, 0, 0, 40, 40, Color{ 100, 60, 140, 255 });
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

    const Texture2D& get_texture(const std::string& tex_name) {
        if (tex_name.empty()) return fallback_texture_;

        std::string key = to_lower(tex_name);
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
            std::string stem = to_lower(strip_ext(tex_name));
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
                // If JPG failed to load, try .png sibling
                std::string stem = to_lower(strip_ext(tex_name));
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

    const Texture2D& get_icon(const std::string& icon_name) {
        return get_texture(icon_name);
    }

    const Texture2D& get_model(const std::string& model_name) {
        return get_texture(model_name);
    }

    const Texture2D& get_fallback() const {
        return fallback_texture_;
    }
};

} // namespace warlock
