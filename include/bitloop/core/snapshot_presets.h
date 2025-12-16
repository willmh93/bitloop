#pragma once

#include <bitloop/core/types.h>
#include <bitloop/util/hashable.h>

BL_BEGIN_NS;

/// todo: Move to SnapshotPresetManager (snapshot_preset.h)
// Snapshot presets (reusable display description / render opts)
struct SnapshotPreset : public Hashable
{
    char name[64]{}; // must be unique
    char alias[32]{};

    IVec2 size{};
    int ssaa = 0;
    float sharpen = -1.0f;

    std::string list_name;
    bl::hash_t hashed_name = 0;

    SnapshotPreset() {}
    SnapshotPreset(const char* _name, const char* _alias, IVec2 _size, int _ssaa = 0, float _sharpen = -1.0f)
    {
        strcpy(name, _name);
        strcpy(alias, _alias);
        size = _size;
        ssaa = _ssaa;
        sharpen = _sharpen;

        updateCache();
    }

    // compute hash for entire preset info
    bl::hash_t compute_hash() const noexcept override
    {
        bl::hash_t seed = 0;
        hash_combine(seed, hash_string(name));
        hash_combine(seed, hash_string(alias));
        hash_combine(seed, hash_one(size.x));
        hash_combine(seed, hash_one(size.y));
        hash_combine(seed, hash_one(ssaa));
        hash_combine(seed, hash_one(sharpen));
        return seed;
    }

    void updateCache()
    {
        invalidate_hash();

        hashed_name = hash_string(alias);
        list_name = name;

        list_name += " [";
        list_name += std::to_string(size.x);
        list_name += "x";
        list_name += std::to_string(size.y);
        list_name += "]";

        if (ssaa > 0)
        {
            list_name += " ssaa:";
            list_name += std::to_string(ssaa);
        }

        if (sharpen > 0)
        {
            list_name += " sharp:";
            list_name += std::to_string(sharpen);
        }
    }
};

typedef std::unordered_map<bl::hash_t, bool> SnapshotPresetHashMap;

class SnapshotPresetList
{
    bool lookup_dirty = false;

    std::string list_name;

    std::vector<SnapshotPreset> items;
    std::unordered_map<bl::hash_t, SnapshotPreset*> lookup;

public:

    /// todo: Replace with "lookup dirty" flag - update lookup on item hash access - keep lookup/items private
    void updateLookup() {
        lookup.clear();
        for (auto& item : items)
            lookup[item.hashed_name] = &item;

        lookup_dirty = false;
    }

    // by index
    SnapshotPreset& operator[](int i) { return items[i]; }
    SnapshotPreset* operator[](std::string_view alias) { return findByAlias(alias); }
    SnapshotPreset* at_safe(int i) { return (i < 0 || i >= (int)items.size()) ? nullptr : &items[i]; }

    // by hash
    SnapshotPreset* find(bl::hash_t hash) {
        if (lookup_dirty) updateLookup();
        if (lookup.count(hash) == 0) return nullptr;
        return lookup[hash];
    }

    std::size_t count(bl::hash_t hash) {
        if (lookup_dirty) updateLookup();
        return lookup.count(hash);
    }

    // by alias
    SnapshotPreset* findByAlias(std::string_view alias) {
        // hash alias, find(hash)?
        for (int i = 0; i < size(); i++) {
            if (items[i].alias == alias)
                return &items[i];
        }
        return nullptr;
    }

    int  size() const { return (int)items.size(); }
    auto begin() noexcept { return items.begin(); }
    auto end()   noexcept { return items.end(); }
    auto begin() const noexcept { return items.begin(); }
    auto end()   const noexcept { return items.end(); }
    auto cbegin() const noexcept { return items.cbegin(); }
    auto cend()   const noexcept { return items.cend(); }

    void add(const SnapshotPreset& preset) {
        items.push_back(preset);
        lookup_dirty = true;
    }

    bool isUniqueAliasChange(int idx, const char* new_alias) const
    {
        for (int i = 0; i < size(); i++)
        {
            if (i == idx) continue;
            if (strcmp(items[i].alias, new_alias) == 0) return false;
        }
        return true;
    }

    SnapshotPresetList filtered(std::unordered_map<bl::hash_t, bool>& filter) const
    {
        SnapshotPresetList filtered_presets;
        for (auto& item : items)
        {
            if (filter.count(item.hashed_name) && filter.at(item.hashed_name))
                filtered_presets.add(item);
        }
        return filtered_presets;
    }
};

class SnapshotPresetManager
{
    SnapshotPresetList capture_presets;
    std::unordered_map<bl::hash_t, bool> enabled_presets;

public:

    SnapshotPresetManager()
    {
        // ------------------------------
        // Generic / common render targets
        // ------------------------------
        capture_presets.add(SnapshotPreset("Square 512", "square512", { 512,  512 }));
        capture_presets.add(SnapshotPreset("Square 1024", "square1024", { 1024, 1024 }));

        capture_presets.add(SnapshotPreset("FHD 1080p (16:9)", "fhd", { 1920, 1080 }));
        capture_presets.add(SnapshotPreset("QHD 1440p (16:9)", "qhd", { 2560, 1440 }));
        capture_presets.add(SnapshotPreset("UHD 4K (16:9)", "uhd4k", { 3840, 2160 }));

        capture_presets.add(SnapshotPreset("WUXGA (16:10)", "wuxga", { 1920, 1200 }));
        capture_presets.add(SnapshotPreset("WQXGA (16:10)", "wqxga", { 2560, 1600 }));

        capture_presets.add(SnapshotPreset("UltraWide FHD (21:9)", "uwfhd", { 2560, 1080 }));
        capture_presets.add(SnapshotPreset("UltraWide QHD (21:9)", "uwqhd", { 3440, 1440 }));

        capture_presets.add(SnapshotPreset("Dual FHD (32:9)", "dfhd", { 3840, 1080 }));
        capture_presets.add(SnapshotPreset("Dual QHD (32:9)", "dqhd", { 5120, 1440 }));
        capture_presets.add(SnapshotPreset("UltraWide 5K2K (21:9)", "uw5k2k", { 5120, 2160 }));

        // ------------------------------
        // 8K targets
        // ------------------------------
        capture_presets.add(SnapshotPreset("UHD 8K (16:9)", "uhd8k", { 7680, 4320 })); // “8K UHD”
        capture_presets.add(SnapshotPreset("DCI 8K (17:9)", "dci8k", { 8192, 4320 })); // cinema 8K
        capture_presets.add(SnapshotPreset("8K (16:10)", "8k16x10", { 7680, 4800 })); // very high-res desktop target
        capture_presets.add(SnapshotPreset("Dual UHD 8K-wide (32:9)", "duhd", { 7680, 2160 })); // effectively “dual 4K”

        // ------------------------------
        // Phones (portrait)
        // ------------------------------
        capture_presets.add(SnapshotPreset("Apple iPhone 16", "iphone16", { 1179, 2556 }));
        capture_presets.add(SnapshotPreset("Apple iPhone 16 Plus", "iphone16plus", { 1290, 2796 }));
        capture_presets.add(SnapshotPreset("Apple iPhone 16 Pro", "iphone16pro", { 1206, 2622 }));
        capture_presets.add(SnapshotPreset("Apple iPhone 16 Pro Max", "iphone16promax", { 1320, 2868 }));

        // Common Android portrait targets (used to replace duplicate device-specific sizes)
        capture_presets.add(SnapshotPreset("Android FHD+ (20:9)", "androidfhdplus", { 1080, 2400 })); // replaces Pixel 8a
        capture_presets.add(SnapshotPreset("Android FHD+ 1080x2340", "android1080x2340", { 1080, 2340 })); // replaces Galaxy S24 / S25
        capture_presets.add(SnapshotPreset("Android FHD+ 1080x2424", "android1080x2424", { 1080, 2424 })); // replaces Pixel 9 / 9a
        capture_presets.add(SnapshotPreset("Android QHD+ 1440x3120", "android1440x3120", { 1440, 3120 })); // replaces S24+ / S24 Ultra / S25+

        // Keep higher-res / less common phone targets
        capture_presets.add(SnapshotPreset("Google Pixel 9 Pro", "pixel9pro", { 1280, 2856 }));
        capture_presets.add(SnapshotPreset("Google Pixel 9 Pro XL", "pixel9proxl", { 1344, 2992 }));
        capture_presets.add(SnapshotPreset("OnePlus 12", "oneplus12", { 1440, 3168 }));

        // ------------------------------
        // Tablets / laptops (landscape)
        // ------------------------------
        capture_presets.add(SnapshotPreset("Apple iPad mini", "ipadmini", { 2266, 1488 }));
        capture_presets.add(SnapshotPreset("Apple iPad Air 11", "ipadair11", { 2360, 1640 }));
        capture_presets.add(SnapshotPreset("Apple iPad Air 13", "ipadair13", { 2732, 2048 }));
        capture_presets.add(SnapshotPreset("Apple iPad Pro 11", "ipadpro11", { 2420, 1668 }));
        capture_presets.add(SnapshotPreset("Apple iPad Pro 13", "ipadpro13", { 2752, 2064 }));

        capture_presets.add(SnapshotPreset("Apple MacBook Air 13", "macbookair13", { 2560, 1664 }));
        capture_presets.add(SnapshotPreset("Apple MacBook Pro 14", "macbookpro14", { 3024, 1964 }));

        // ------------------------------
        // Handhelds / gaming / other
        // ------------------------------
        capture_presets.add(SnapshotPreset("Valve Steam Deck OLED", "steamdeckoled", { 1280,  800 }));
        capture_presets.add(SnapshotPreset("Nintendo Switch OLED", "switcholed", { 1280,  720 }));

        // ------------------------------
        // Thumbnails / Icons
        // ------------------------------
        capture_presets.add(SnapshotPreset("Thumbnail", "thumb128x72", { 128, 72 }));
        capture_presets.add(SnapshotPreset("Thumbnail (HD)", "thumb128x72_hd", { 128, 72 }, 9));

        capture_presets.updateLookup();

        //enabled_presets[capture_presets.findByAlias("fhd")->hashed_name] = true;
        enabled_presets[capture_presets["fhd"]->hashed_name] = true;
    }

    SnapshotPresetList& allPresets()
    {
        return capture_presets;
    }

    SnapshotPresetHashMap& enabledHashes()
    {
        return enabled_presets;
    }

    //std::vector<SnapshotPreset> enabledPresets() const
    SnapshotPresetList enabledPresets() const
    {
        // todo: make capture_presets atomic in case of rare change while accessing in loop
        SnapshotPresetList ret;
        for (const auto& p : capture_presets)
        {
            //if (p.enabled)
            if (enabled_presets.count(p.hashed_name) > 0)
                ret.add(p);
        }
        ret.updateLookup();
        return ret;
    }
};

BL_END_NS;
