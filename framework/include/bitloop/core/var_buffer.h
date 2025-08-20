#include "debug.h"

template<typename T>
concept VarBufferConcept = requires(T t, const T & rhs)
{
    { t.populateUI() } -> std::same_as<void>;
};

struct BaseVariable
{
    std::string name;

    BaseVariable(std::string_view name) : name(name) {}

    virtual BaseVariable* clone() { return nullptr; }
    //virtual std::string toString() { return "<na>"; }
    virtual void setValue(const BaseVariable* rhs) = 0;
    virtual bool equals(const BaseVariable* rhs) const = 0;
};

template<typename T>
struct Variable final : public BaseVariable
{
    using element_type =
        std::conditional_t<std::is_array_v<T>,
        std::remove_extent_t<T>,
        T>;

    using storage_ptr = element_type*;

    storage_ptr ptr = nullptr;
    bool owns_data = false;

    Variable(std::string_view name, storage_ptr v, bool auto_destruct)
        : BaseVariable(name), ptr(v), owns_data(auto_destruct)
    {}

    ~Variable()
    {
        if (owns_data && ptr)
        {
            // Make sure we only delete the clones, not variables which were allocated by simulations
            delete ptr;
            ptr = nullptr;
        }
    }

    void setValue(const BaseVariable* rhs) override
    {
        const auto rhs_var = dynamic_cast<const Variable<T>*>(rhs);
        if (!rhs_var)
            return; // bad cast

        if constexpr (std::is_array_v<T>)
        {
            constexpr std::size_t N = std::extent_v<T>;
            std::copy(rhs_var->ptr, rhs_var->ptr + N, ptr);
        }
        else
        {
            *ptr = *rhs_var->ptr;
        }
    }

    bool equals(const BaseVariable* rhs) const override
    {
        const auto rhs_var = dynamic_cast<const Variable<T>*>(rhs);
        if (!rhs_var)
            return false; // bad cast

        if constexpr (std::is_array_v<T>)
        {
            constexpr std::size_t N = std::extent_v<T>;
            return std::equal(ptr, ptr + N, rhs_var->ptr);
        }
        else if constexpr (std::is_standard_layout_v<T> && std::is_trivially_copyable_v<T>)
        {
            // Safe to compare as raw memory
            return std::memcmp(ptr, rhs_var->ptr, sizeof(T)) == 0;
        }
        else
        {
            /// If you get an error here, your data type requires an operator==(const T&) overload
            return *ptr == *rhs_var->ptr;
        }
    }

    Variable* clone() override
    {
        if constexpr (std::is_array_v<T>)
        {
            constexpr std::size_t N = std::extent_v<T>;
            auto* new_arr = new element_type[N];
            std::copy(ptr, ptr + N, new_arr);
            return new Variable(name, new_arr, true);
        }
        else
        {
            return new Variable(name, new element_type(*ptr), true);
        }
    }

    //std::string toString() override
    //{
    //    std::stringstream ss;
    //    ss << *ptr;
    //    return ss.str();
    //}
};

struct VariableEntry
{
    int id;
    std::string name;

    // Actual live/shadow buffers
    BaseVariable* shadow_ptr    = nullptr;
    BaseVariable* live_ptr      = nullptr;

    // "Before" buffers to compare against and detect changes
    BaseVariable* shadow_marked = nullptr;
    BaseVariable* live_marked   = nullptr;

    void updateLive()           { live_ptr->setValue(shadow_ptr); }
    void updateShadow()         { shadow_ptr->setValue(live_ptr); }
    void markLiveValue()        { live_marked->setValue(live_ptr); }
    void markShadowValue()      { shadow_marked->setValue(shadow_ptr); }
    bool liveChanged()   const  { return !live_ptr->equals(live_marked); }
    bool shadowChanged() const  { return !shadow_ptr->equals(shadow_marked); }

    //void print()
    //{
    //    BL::print() << "Shadow: " << shadow_ptr->toString() 
    //                << "  Live: " << live_ptr->toString();
    //}
};

struct VariableMap : public std::vector<VariableEntry>
{
    void markLiveValues()   { for (size_t i=0; i<size(); i++) at(i).markLiveValue(); }
    void markShadowValues() { for (size_t i=0; i<size(); i++) at(i).markShadowValue(); }
    bool changedLive() {
        for (size_t i = 0; i < size(); i++) {
            if (at(i).liveChanged())
                return true;
        }
        return false;
    }
    bool changedShadow() {
        for (size_t i = 0; i < size(); i++)  {
            if (at(i).shadowChanged())
                return true;
        }
        return false;
    }
};

struct VarBuffer
{
    std::vector<BaseVariable*> buffer_var_ptrs;

    VarBuffer() = default;
    VarBuffer(const VarBuffer&) = delete;

    #define sync(v) _sync(#v, v)

    template<typename T>
    void _sync(const char* name, T& v)
    {
        if constexpr (std::is_array_v<T>)
            buffer_var_ptrs.push_back(new Variable<T>(name, v, false));   // v  -> element*
        else
            buffer_var_ptrs.push_back(new Variable<T>(name, &v, false));  // &v -> T*
    }


    virtual void registerSynced() {};
    virtual void initData() {}
    virtual void populateUI() {}
};

template<typename T>
concept DerivedFromVarBuffer = std::derived_from<T, VarBuffer>;

// ======== DoubleBuffer ========
// Inherits from passed VarBuffer type, and stores an internal "shadow_attributes" VarBuffer
// 
// Call:  
//   sync(...)  inside VarBuffer::registerSynced()  to register a variable for tracking/syncing
// 
// Allows marking live/shadow variables states to detect changes that need syncing

template<DerivedFromVarBuffer VarBufferType>
class DoubleBuffer : public VarBufferType
{
public:

	VarBufferType shadow_attributes; // 2nd buffer, which is synced with the identical inherited VarBufferType
    VariableMap var_map;

    DoubleBuffer() : shadow_attributes()
    {
        // Let each buffer list their synced members with sync()
        shadow_attributes.registerSynced();
        VarBufferType::registerSynced();

        // Join up the lists into a single 'var_map'
        for (size_t i = 0; i < VarBufferType::buffer_var_ptrs.size(); i++)
        {
            VariableEntry entry;
            entry.shadow_ptr = shadow_attributes.buffer_var_ptrs[i];
            entry.live_ptr = VarBufferType::buffer_var_ptrs[i];
            entry.shadow_marked = entry.shadow_ptr->clone();
            entry.live_marked = entry.live_ptr->clone();

            entry.id = (int)i;
            entry.name = entry.live_ptr->name;
            //entry.print();

            var_map.push_back(entry);
        }

        // Initialize data on shadow *after* we start tracking, so that changes detected and synced
        shadow_attributes.initData();

        // Taken ownership of pointers, clear lists
        shadow_attributes.buffer_var_ptrs.clear();
        VarBufferType::buffer_var_ptrs.clear();
    }

    ///~DoubleBuffer()
    ///{
    ///    // todo: Clean up owned data pointers
    ///    for (VariableEntry& entry : var_map)
    ///    {
    ///
    ///    }
    ///}

    void updateLiveBuffer()
    {
        for (VariableEntry& entry : var_map) {
            if (entry.shadowChanged())
                entry.updateLive();
        }
    }
    void updateShadowBuffer()
    {
        for (VariableEntry& entry : var_map) {
            if (entry.liveChanged())
                entry.updateShadow();
        }
    }

    void markLiveValues() { var_map.markLiveValues(); }
    void markShadowValues() { var_map.markShadowValues(); }

    bool changedLive() { return var_map.changedLive(); }
    bool changedShadow() { return var_map.changedShadow(); }

    //std::string pad(const std::string& str, int width) const
    //{
    //    return str + std::string(width > str.size() ? width - str.size() : 0, ' ');
    //}
    //std::string toString() const
    //{
    //    static const int name_len = 28;
    //    static const int val_len = 10;
    //
    //    std::stringstream txt;
    //    for (const auto& entry : var_map)
    //        txt << pad(std::to_string(entry.id) + ". ", 4) << pad(entry.name, name_len)
    //        << "SHADOW_MARKED: " << pad(entry.shadow_marked->toString(), val_len) << "   SHADOW_VAL: " << pad(entry.shadow_ptr->toString(), val_len)
    //        << "    LIVE_MARKED:   " << pad(entry.live_marked->toString(), val_len) << "   LIVE_VAL: " << pad(entry.live_ptr->toString(), val_len)
    //        << "\n";
    //
    //    return txt.str();
    //}
};
