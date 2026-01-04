#pragma once

#include <bitloop/core/types.h>
#include <bitloop/util/hashable.h>

BL_BEGIN_NS;

template <std::size_t N>
inline void assign_fixed_string(char(&dest)[N], std::string_view src)
{
    static_assert(N > 0, "Buffer must have space for null terminator.");

    const std::size_t len = std::min<std::size_t>(N - 1, src.size());
    if (len != 0)
        std::memcpy(dest, src.data(), len);

    dest[len] = '\0';
}

// Snapshot presets (reusable display description / render opts)
class SnapshotPreset : public Hashable
{
    char  alias[32]{}; // must be unique
    char  name[64]{};
    IVec2 size{};
    int   ssaa = 0;
    float sharpen = -1.0f;

    std::string list_name;
    bl::hash_t  hashed_alias = 0;

public:

    SnapshotPreset() {}
    SnapshotPreset(const SnapshotPreset& rhs)
    {
        strcpy(alias, rhs.alias);
        strcpy(name, rhs.name);
        size = rhs.size;
        ssaa = rhs.ssaa;
        sharpen = rhs.sharpen;
        updateCache();
    }
    SnapshotPreset(std::string_view _name, std::string_view _alias, IVec2 _size, int _ssaa = 0, float _sharpen = -1.0f)
    {
        strcpy(alias, _alias.data());
        strcpy(name, _name.data());
        size = _size;
        ssaa = _ssaa;
        sharpen = _sharpen;
        updateCache();
    }

    // getters
    [[nodiscard]] std::string_view getAlias() const noexcept { return std::string_view{ alias, std::char_traits<char>::length(alias) }; }
    [[nodiscard]] std::string_view getName()  const noexcept { return std::string_view{ name, std::char_traits<char>::length(name) }; }
    [[nodiscard]] IVec2 getResolution()       const noexcept { return size; }
    [[nodiscard]] int width()                 const noexcept { return size.x; }
    [[nodiscard]] int height()                const noexcept { return size.y; }
    [[nodiscard]] int getSSAA()               const noexcept { return ssaa; }
    [[nodiscard]] float getSharpening()       const noexcept { return sharpen; }

    [[nodiscard]] const char* alias_cstr()    const noexcept { return alias; }
    [[nodiscard]] const char* name_cstr()     const noexcept { return name; }
    [[nodiscard]] const char* description()   const noexcept { return list_name.c_str(); }

    [[nodiscard]] int maxAliasSize()          const noexcept { return sizeof(alias); }
    [[nodiscard]] int maxNameSize()           const noexcept { return sizeof(name); }
                                            
    [[nodiscard]] hash_t hashedAlias()        const noexcept { return hashed_alias; }

    // setters
    void setAlias(std::string_view value) { assign_fixed_string(alias, value); updateCache(); }
    void setName(std::string_view value)  { assign_fixed_string(name, value);  updateCache(); }
    void setResolution(IVec2 res)         { size = res;                        updateCache(); }
    void setResolution(int w, int h)      { size = { w, h };                   updateCache(); }
    void setSSAA(int _ssaa)               { ssaa = _ssaa;                      updateCache(); }
    void setSharpening(float _sharpen)    { sharpen = _sharpen;                updateCache(); }

    // compute hash for entire preset info
    hash_t compute_hash() const noexcept override
    {
        StableHasher h;
        h.add_string(alias);
        h.add_string(name);
        h.add(size.x);
        h.add(size.y);
        h.add(ssaa);
        h.add(sharpen);
        return h.finish();
    }

    void updateCache()
    {
        invalidate_hash();

        hashed_alias = Hasher::hash_string(alias);
        list_name = std::format("{} [{}x{}]", name, size.x, size.y);

        if (ssaa > 0) {
            list_name += " ssaa:";
            list_name += std::to_string(ssaa);
        }

        if (sharpen > 0) {
            list_name += " sharp:";
            list_name += std::to_string(sharpen);
        }
    }
};

typedef std::unordered_map<bl::hash_t, bool> SnapshotPresetHashMap;

using SnapshotCompleteCallback       = std::function<void(bytebuf&, const SnapshotPreset& preset)>;
using SnapshotBatchCompleteCallback  = std::function<void()>;

struct SnapshotBatchCallbacks
{
    SnapshotCompleteCallback       on_snapshot_complete;
    SnapshotBatchCompleteCallback  on_batch_complete;
};

class SnapshotPresetList
{
    bool lookup_dirty = false;

    std::vector<SnapshotPreset> items;
    std::unordered_map<bl::hash_t, SnapshotPreset*> lookup;

public:

    SnapshotPresetList() = default;
    SnapshotPresetList(const SnapshotPreset& item) { add(item); }
    SnapshotPresetList(const std::vector<SnapshotPreset>& _items)
    {
        for (const SnapshotPreset& item : _items)
            add(item);
    }

    // only refreshes hashed lookup on demand
    void updateLookup() {
        lookup.clear();
        for (auto& item : items)
            lookup[item.hashedAlias()] = &item;

        lookup_dirty = false;
    }

    void add(const SnapshotPreset& preset) {
        items.push_back(preset);
        lookup_dirty = true;
    }

    std::size_t count(bl::hash_t hash) {
        if (lookup_dirty) updateLookup();
        return lookup.count(hash);
    }

    // find by: hash
    SnapshotPreset* find(bl::hash_t hash) {
        if (lookup_dirty) updateLookup();
        if (lookup.count(hash) == 0) return nullptr;
        return lookup[hash];
    }

    // find by: index
    SnapshotPreset& operator[](int i)                   { return items[i]; }
    const SnapshotPreset& operator[](int i) const       { return items[i]; }
    SnapshotPreset& at(int i)                           { return items[i]; }
    const SnapshotPreset& at(int i) const               { return items[i]; }
    SnapshotPreset* at_safe(int i)                      { return (i < 0 || i >= (int)items.size()) ? nullptr : &items[i]; }

    // find by: alias
    SnapshotPreset* operator[](std::string_view alias)             { return findByAlias(alias); }
    const SnapshotPreset* operator[](std::string_view alias) const { return findByAlias(alias); }
    SnapshotPreset* findByAlias(std::string_view alias) {
        // hash alias, find(hash)?
        for (int i = 0; i < size(); i++) {
            if (items[i].getAlias() == alias)
                return &items[i];
        }
        return nullptr;
    }
    const SnapshotPreset* findByAlias(std::string_view alias) const {
        // hash alias, find(hash)?
        for (int i = 0; i < size(); i++) {
            if (items[i].getAlias() == alias)
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

    bool isUniqueAliasChange(int idx, const char* new_alias) const
    {
        for (int i = 0; i < size(); i++)
        {
            if (i == idx) continue;
            if (items[i].getAlias() == new_alias) return false;
            //if (strcmp(items[i].alias, new_alias) == 0) return false;
        }
        return true;
    }

    SnapshotPresetList filtered(SnapshotPresetHashMap& filter) const
    {
        SnapshotPresetList filtered_presets;
        for (auto& item : items)
        {
            if (filter.count(item.hashedAlias()) && filter.at(item.hashedAlias()))
                filtered_presets.add(item);
        }
        return filtered_presets;
    }
};

class SettingsPanel;
class SnapshotPresetManager
{
    friend class SettingsPanel;

    SnapshotPresetList    capture_presets;

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
    }

    SnapshotPresetList& allPresets()
    {
        return capture_presets;
    }
    const SnapshotPresetList& allPresets() const
    {
        return capture_presets;
    }
};

BL_END_NS;
