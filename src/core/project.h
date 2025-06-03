#pragma once

#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <functional>
#include <type_traits>
#include <random>

#include "platform.h"
#include "helpers.h"
#include "types.h"
#include "json.h"
#include "event.h"
#include "camera.h"
#include "nano_canvas.h"
#include "imgui_custom.h"
#include "project_worker.h"

using std::max;
using std::min;

// Provide macros for easy BasicProject registration
template<typename... Ts>
std::vector<std::string> VectorizeArgs(Ts&&... args) { return { std::forward<Ts>(args)... }; }

#define SIM_BEG(ns) namespace ns{
#define SIM_DECLARE(ns, ...) namespace ns { AutoRegisterProject<ns##_Project> register_##ns(VectorizeArgs(__VA_ARGS__));
#define SIM_END(ns) } using ns::ns##_Project;



/// =================================================================================
/// ======= varcpy overloads (helper for safely copying data between threads) =======
/// =================================================================================

// --- varcpy (non-primative requiring ::copyFrom method) ---
template<typename T>
concept ImpliedThreadSafeCopyable = requires(T t, const T & rhs) {
    { t.copyFrom(rhs) } -> std::same_as<void>;
};

template<ImpliedThreadSafeCopyable T>
void varcpy(T& dst, const T& src) {
    dst.copyFrom(src);
}

// --- varcpy (primative copyable) ---
template<typename T>
concept PrimitiveCopyable = std::is_trivially_copyable_v<T> && std::is_assignable_v<T&, const T&>;

template<PrimitiveCopyable T>
void varcpy(T& dst, const T& src) {
    dst = src;
}

// --- varcpy (trivially copyable array) ---
template<typename T, size_t N>
void varcpy(T(&dst)[N], const T(&src)[N]) {
    static_assert(std::is_trivially_copyable_v<T>,
        "varcpy: array element type must be trivially copyable");
    std::memcpy(dst, src, sizeof(T) * N);
}

// --- varcpy (trivially copyable base part of a possibly non-trivially copyable derived class) ---
template<typename Base, typename Derived>
void varcpy(Derived& dst, const Derived& src) {
    varcpy(*static_cast<Base*>(&dst), *static_cast<const Base*>(&src));
}

/// ---------------------------------------------------------
/// 
/// Tiny helper macros to insert sentinel markers around data
/// which will be automatically synchronized between the live
/// data buffer and GUI data buffer.
/// 
/// Note: Only one sync-block is permitted per VarBuffer.
/// 
/// ---------------------------------------------------------

#define sync_struct std::byte __sync_beg__; struct
#define sync_end    ; std::byte __sync_end__;

template<class T>
constexpr std::span<std::byte> writable_sync_span(T& obj)
{
    auto* beg = reinterpret_cast<std::byte*>(&obj.__sync_beg__) + 1;
    auto* end = reinterpret_cast<std::byte*>(&obj.__sync_end__);
    return { beg, end };
}

template<class T>
constexpr std::span<const std::byte> readonly_sync_span(const T& obj)
{
    auto* beg = reinterpret_cast<const std::byte*>(&obj.__sync_beg__) + 1;
    auto* end = reinterpret_cast<const std::byte*>(&obj.__sync_end__);
    return { beg, end };
}

template<typename T>
concept VarBufferConcept = requires(T t, const T & rhs) {
    { t.populate() } -> std::same_as<void>;
    //{ t.copyFrom(rhs) } -> std::same_as<void>;
};



template<typename T>
concept HasSyncMembers = requires(T & t) {
    { t.__sync_beg__ } -> std::convertible_to<std::byte>;
    { t.__sync_end__ } -> std::convertible_to<std::byte>;
};

/*template<typename T>
struct synced
{
    static VarTracker* tracker;
    T value;

    synced(T&& v) : value(std::forward<T>(v))
    {
        tracker->sync(value);
    }

    synced() = default;
    operator T& () { return value; }
    operator const T& () const { return value; }
};*/

struct IVarData
{
    uint64_t offset = 0xFFFFFFFFFFFFFFFFull;
    uint64_t size = 0xFFFFFFFFFFFFFFFFull;

    virtual ~IVarData() = default;

    virtual std::string liveToString() = 0;
    virtual std::string shadowToString() = 0;
    virtual std::string liveMarkedToString() = 0;
    virtual std::string shadowMarkedToString() = 0;

    virtual void* shadowData() = 0;

    virtual void updateLive(void* ptr) = 0;
    virtual void updateShadow(void* ptr) = 0;
    virtual void markLiveValue(void* ptr) = 0;
    virtual void markShadowValue(void* ptr) = 0;
    virtual bool liveChanged(void* ptr) const = 0;
    virtual bool shadowChanged(void* ptr) const = 0;
};

template<typename T>
struct VarData final : IVarData
{
    mutable T* ptr;
    mutable T shadowValue{};
    mutable T markedShadowValue{};
    mutable T markedLiveValue{};
    bool hasShadow = false;

    explicit VarData(T* _ptr, uint64_t off, uint64_t _size) : ptr(_ptr)
    {
        offset = off;
        size = _size;
    }

    std::string liveToString()
    {
        std::stringstream ss;
        ss << *ptr;
        return ss.str();
    }

    std::string shadowToString()
    {
        std::stringstream ss;
        ss << shadowValue;
        return ss.str();
    }

    std::string liveMarkedToString()
    {
        std::stringstream ss;
        ss << markedLiveValue;
        return ss.str();
    }

    std::string shadowMarkedToString()
    {
        std::stringstream ss;
        ss << markedShadowValue;
        return ss.str();
    }

    void* shadowData()
    {
        return static_cast<void*>(&shadowValue);
    }

    void updateLive(void* ptr) override
    {
        if (hasShadow)
            *static_cast<T*>(ptr) = shadowValue;
    }

    void updateShadow(void* ptr) override
    {
        shadowValue = *static_cast<T*>(ptr);
        hasShadow = true;
    }

    void markLiveValue(void* ptr) override
    {
        markedLiveValue = *static_cast<T*>(ptr);
    }

    void markShadowValue(void* /*ptr*/) override
    {
        if (hasShadow)
            markedShadowValue = shadowValue;
    }

    bool liveChanged(void* ptr) const override
    {
        return markedLiveValue != *static_cast<T*>(ptr);
    }

    bool shadowChanged(void* /*ptr*/) const override
    {
        return hasShadow && markedShadowValue != shadowValue;
    }
};


struct BaseVariable
{
    std::string name;

    BaseVariable(std::string_view name) : name(name)
    {}

    virtual BaseVariable* clone() 
    {
        return nullptr;
    }
    virtual std::string toString()
    {
        return "null";
    }
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
        else
        {
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

    std::string toString()
    {
        std::stringstream ss;
        ss << *ptr;
        return ss.str();
    }
};


/*template<typename T>
struct VariablePtr final : public BaseVariable
{
    T* ptr = nullptr;

    VariablePtr(T* v) : ptr(v)
    {
    }

    BaseVariable* clone() override
    {
        if constexpr (std::is_array_v<T>)
        {
            using ElementType = std::remove_extent_t<T>;
            using NonConstElementType = std::remove_const_t<ElementType>;
            constexpr size_t N = std::extent_v<T>;

            NonConstElementType* new_arr = new NonConstElementType[N];
            std::copy(std::begin(*ptr), std::end(*ptr), new_arr);

            return new Variable(new_arr, true);
        }
        else
        {
            return new Variable(new T(*ptr), true);
        }
    }

    std::string toString()
    {
        std::stringstream ss;
        ss << *ptr;
        return ss.str();
    }
};*/

struct VariableEntry
{
    int id;
    std::string name;

    BaseVariable* shadow_ptr    = nullptr;
    BaseVariable* live_ptr      = nullptr;
    BaseVariable* shadow_marked = nullptr;
    BaseVariable* live_marked   = nullptr;

    void updateLive()      { /* if (id==1) DebugPrint("live:          cam_x = %s", shadow_ptr->toString().c_str()); */ live_ptr->setValue(shadow_ptr); }
    void updateShadow()    { /* if (id==1) DebugPrint("shadow:        cam_x = %s", live_ptr->toString().c_str());   */ shadow_ptr->setValue(live_ptr); }
    void markLiveValue()   { /* if (id==1) DebugPrint("marked_live:   cam_x = %s", live_ptr->toString().c_str());   */ live_marked->setValue(live_ptr); }
    void markShadowValue() { /* if (id==1) DebugPrint("marked_shadow: cam_x = %s", shadow_ptr->toString().c_str()); */ shadow_marked->setValue(shadow_ptr); }
    bool liveChanged()   const  { return !live_ptr->equals(live_marked); }
    bool shadowChanged() const  { return !shadow_ptr->equals(shadow_marked); }

    bool pendingChange() const  { return !shadow_ptr->equals(live_ptr); }

    void print()
    {
        DebugPrint("Shadow: %s  Live: %s", 
            shadow_ptr->toString().c_str(),
            live_ptr->toString().c_str()
        );
    }
};

struct VariableMap : public std::vector<VariableEntry>
{
    void markLiveValues()   { for (size_t i=0; i<size(); i++) at(i).markLiveValue(); }
    void markShadowValues() { for (size_t i=0; i<size(); i++) at(i).markShadowValue(); }
    bool changedShadow() {
        for (size_t i = 0; i < size(); i++)  {
            if (at(i).shadowChanged())
                return true;
        }
        return false;
    }
};

struct VarTracker
{
    std::vector<BaseVariable*> buffer_var_ptrs;

    template<typename T>
    void sync2(const char*name, T& v)
    {
        if constexpr (std::is_array_v<T>)
            buffer_var_ptrs.push_back(new Variable<T>(name, v, false));   // v  -> element*
        else
            buffer_var_ptrs.push_back(new Variable<T>(name, &v, false));  // &v -> T*
    }

    #define sync3(v) sync2(#v, v)

    /// Ownership of ptrs claimed by Scene
    //~VarTracker()
    //{
    //    for (BaseVariable* v : buffer_var_ptrs)
    //        delete v;
    //    buffer_var_ptrs.clear();
    //}



    // Old implementation
    static VarTracker* active_tracker;
    mutable std::unordered_map<void*, std::unique_ptr<IVarData>> varMap;

    VarTracker()
    {
        active_tracker = this;
    }

    

    /*std::string toString() const
    {
        std::stringstream txt;
        for (const auto& pair : varMap)
            txt << "Offset: " 
              << pair.second->offset
              << " bytes,  Size: " 
              << pair.second->size
              << "     [shadow: " << pair.second->shadowToString() << ", marked: " << pair.second->shadowMarkedToString() << "]"
              << "     [live: "   << pair.second->liveToString()   << ", marked: " << pair.second->liveMarkedToString() << "]"
              << "\n";

        return txt.str();
    }*/

    //bool populating_ui = false;

    template<typename T>
    auto sync(T const& v) const -> std::remove_cv_t<T>&
    {
        using NonConstT = std::remove_cv_t<T>;

        // pointer to the non-const view of v
        auto* ptr = const_cast<NonConstT*>(&v);

        //if (populating_ui)
        {
            if (!varMap.contains(ptr))
            {
                std::uint64_t offset =
                    static_cast<std::uint64_t>(
                    reinterpret_cast<std::byte*>(ptr) -
                    reinterpret_cast<std::byte const*>(this));

                varMap[ptr] =
                    std::make_unique<VarData<NonConstT>>(ptr, offset, sizeof(NonConstT));

                updateShadow(ptr);   // todo: Avoid worker process during shadow initialization, unsafe
            }
            
            // return a non-const reference to the shadow copy
            return *static_cast<NonConstT*>(varMap[ptr]->shadowData());
        }
        //else
        {
            return *ptr;
        }
    }

    template<typename... Ts>
    auto expose(Ts&... vars) const 
    {
        return std::forward_as_tuple(sync(vars)...);  // returns tuple of T&
    }
    
    /*template<typename T, int N, std::size_t... I>
    constexpr void syncEachVar(const T(&items)[N], std::index_sequence<I...>) {
        (sync(items[I]), ...);
    }


    template<typename T, int N>
    constexpr T(&sync(const T(&items)[N]))[N]
    {
        syncEachVar(items, std::make_index_sequence<N>{});
        return &items[0];
    }*/

    //template<typename T, int N>
    //constexpr T* sync(const T(&items)[N])
    //{
    //    syncEachVar(items, std::make_index_sequence<N>{});
    //    return &items[0];
    //}

    void updateLive(void* ptr)          { varMap[ptr]->updateLive(ptr); }
    void updateShadow(void* ptr) const  { varMap[ptr]->updateShadow(ptr); }
    void markLiveValue(void* ptr)       { varMap[ptr]->markLiveValue(ptr); }
    void markShadowValue(void* ptr)     { varMap[ptr]->markShadowValue(ptr); }
    bool liveChanged(void* ptr) const   { return varMap.at(ptr)->liveChanged(ptr); }
    bool shadowChanged(void* ptr) const { return varMap.at(ptr)->shadowChanged(ptr); }

    bool changedShadow()
    {
        for (const auto& pair : varMap)
        {
            if (pair.second->shadowChanged(pair.first))
                return true;
        }
        return false;
    }

    void markLiveValues()
    {
        for (auto& [ptr, data] : varMap)
            data->markLiveValue(ptr);
    }

    void markShadowValues()
    {
        for (auto& [ptr, data] : varMap)
            data->markShadowValue(ptr);
    }

    void copyChangedShadowVarsToLive()
    {
        for (auto& [ptr, data] : varMap)
        {
            if (data->shadowChanged(ptr))
            {
                //DebugPrint("Shadow [%s] changed to [%s] - Updating Live", 
                //    data->shadowMarkedToString().c_str(), data->shadowToString().c_str());

                //std::string old_live = data->liveToString();
                
                data->updateLive(ptr);

                //DebugPrint("     Live [%s] updated to [%s]",
                //        old_live.c_str(),  data->liveToString().c_str());
            }
        }
    }

    void copyChangedLiveVarsToShadow()
    {
        for (auto& [ptr, data] : varMap)
        {
            //if (!data->shadowChanged(ptr)) // if we have not changed shadow since start of worker frame
            if (data->liveChanged(ptr))
            {
                //DebugPrint("Live [%s] changed to [%s] - Updating Live",
                //    data->liveMarkedToString().c_str(), data->liveToString().c_str());

                //std::string old_shadow = data->shadowToString();

                data->updateShadow(ptr);

                //DebugPrint("     Shadow [%s] updated to [%s]",
                //    old_shadow.c_str(), data->shadowToString().c_str());
            }
        }
    }
};



/*template<Buf which>
struct synced_interpreter
{

};

struct synced_container
{
    template<Buf which>
    synced_interpreter<which>* get()
    {
        return nullptr;
    }
};*/

enum class Buf : std::uint8_t { Live = 0, Shadow = 1 };

/*template<typename T, Buf which=Buf::Live>
struct synced
{
    T value;

    synced()
    {
        // Force instantiation of both live/shadow versions of synced<T> class
        if constexpr (which == Buf::Shadow)
            (void)sizeof(synced<T, Buf::Live>);
        else
            (void)sizeof(synced<T, Buf::Shadow>);
    }

    synced(const T& v) 
    {
        operator=(v);
    }

    operator T& () 
    { 
        if constexpr (which == Buf::Shadow)
            return VarTracker::active_tracker->sync(value);
        else
            return value;
    }
    operator const T& () const
    {
        if constexpr (which == Buf::Shadow)
            return VarTracker::active_tracker->sync(value);
        else
            return value;
    }

    T* operator&() noexcept 
    { 
        if constexpr (which == Buf::Shadow)
            return &VarTracker::active_tracker->sync(value);
        else
            return &value;
    }

    const T* operator&() const noexcept
    {
        if constexpr (which == Buf::Shadow)
            return &VarTracker::active_tracker->sync(value);
        else
            return &value;
    }

    T& operator=(const T& rhs) noexcept
    {
        if constexpr (which == Buf::Shadow)
            VarTracker::active_tracker->sync(value) = rhs;
        else
            value = rhs;
        
        return *this;
    }
};*/

/*template<typename T, typename ContainerView>
struct synced;

template<typename Derived, Buf Mode = Buf::Live>
struct SyncedContainerView
{
    static constexpr Buf mode = Mode;

    template<Buf Other>
    using rebind = SyncedContainerView<Derived, Other>;

    template<typename T>
    using synced_t = synced<T, SyncedContainerView<Derived, Mode>>;

    //using ShadowType = SyncedContainerView<Buf::Shadow>;
    using Live = SyncedContainerView<Derived, Buf::Live>;
    using Shadow = SyncedContainerView<Derived, Buf::Shadow>;
};
*/

struct synced_base
{
    static std::vector<synced_base*> vars;
};

template<typename T, Buf which= Buf::Live>
struct synced : synced_base
{
    T value;
    T shadow;

    synced()
    {
        // Force instantiation of both live/shadow versions of synced<T> class
        if constexpr (which == Buf::Shadow)
            (void)sizeof(synced<T, Buf::Live>);
        else
            (void)sizeof(synced<T, Buf::shadow>);

        synced_base::vars.push_back(this);
    }

    ~synced()
    {
        // remove ptr
    }

    synced(const T& v)
    {
        operator=(v);
    }

    operator T& ()
    {
        if constexpr (which == Buf::Shadow)
            return shadow;
        else
            return value;
    }
    operator const T& () const
    {
        if constexpr (which == Buf::Shadow)
            return shadow;
        else
            return value;
    }

    T* operator&() noexcept
    {
        if constexpr (which == Buf::Shadow)
            return &shadow;
        else
            return &value;
    }

    const T* operator&() const noexcept
    {
        if constexpr (which == Buf::Shadow)
            return &shadow;
        else
            return &value;
    }

    T& operator=(const T& rhs) noexcept
    {
        if constexpr (which == Buf::Shadow)
            shadow = rhs;
        else
            value = rhs;

        return *this;
    }

    /*operator T& ()
    {
        if constexpr (ContainerView::Mode == Buf::Shadow)
            return VarTracker::active_tracker->sync(value);
        else
            return value;
    }
    operator const T& () const
    {
        if constexpr (ContainerView::Mode == Buf::Shadow)
            return VarTracker::active_tracker->sync(value);
        else
            return value;
    }

    T* operator&() noexcept
    {
        if constexpr (ContainerView::Mode == Buf::Shadow)
            return &VarTracker::active_tracker->sync(value);
        else
            return &value;
    }

    const T* operator&() const noexcept
    {
        if constexpr (ContainerView::Mode == Buf::Shadow)
            return &VarTracker::active_tracker->sync(value);
        else
            return &value;
    }

    T& operator=(const T& rhs) noexcept
    {
        if constexpr (ContainerView::Mode == Buf::Shadow)
            VarTracker::active_tracker->sync(value) = rhs;
        else
            value = rhs;

        return *this;
    }*/
};

/*
template<typename T, typename View>
struct synced
{
    static_assert(std::is_same_v<decltype(View::mode), const Buf>,
        "Second template parameter must be a SceneBase view type");

    T value{};                                     // the actual object

    using OppositeView = typename View::template rebind<
        (View::mode == Buf::Live ? Buf::Shadow : Buf::Live)>;

    synced()
    {
        (void)sizeof(synced<T, OppositeView>);
    }

    explicit synced(const T& v) { *this = v; }

private:
    static constexpr bool is_shadow = (View::mode == Buf::Shadow);

    T& access() const
    {
        if constexpr (is_shadow)
            return VarTracker::active_tracker->sync(const_cast<T&>(value));
        else
            return const_cast<T&>(value);
    }

public:
    operator T& () { return access(); }
    operator const T& () const { return access(); }

    T* operator&()       noexcept { return &access(); }
    const T* operator&() const noexcept { return &access(); }

    T& operator=(const T& rhs) noexcept
    {
        access() = rhs;
        return access();
    }
};



struct ImaginaryScene : SyncedContainerView<ImaginaryScene>          // default = Live
{
    using SyncedContainerView<ImaginaryScene>::Live::synced_t;

    synced_t<double> test_num;
    ///synced_t<double>      test_num;
    ///synced_t<std::string> test_str;

    void process() { 
        ++test_num; 
    }

    static void sceneAttributes(ImaginaryScene<Buf::Shadow>&& s)
    {
        s.test_num = 5;               // shadow semantics
    }
};
*/

    //synced_t<std::string> test_str;
/*struct VarData
{
    uint64_t offset; 
    size_t size; 
    uint8_t* shadow_value; 
    uint8_t* marked_shadow_value; 
    uint8_t* marked_live_value; 
};

struct VarTracker
{
    std::unordered_map<void*, VarData> var_map;

    template<typename T> T* Var(T* ptr)
    {
        if (!var_map.contains(ptr))
        {
            uint64_t offset = static_cast<uint64_t>(reinterpret_cast<uint8_t*>(ptr) - reinterpret_cast<uint8_t*>(this));
            var_map[ptr] = { offset, sizeof(T), nullptr, new uint8_t[sizeof(T)], new uint8_t[sizeof(T)] };
        }
        return ptr;
    }

    void updateLive(void* var_ptr)
    {
        if (var_map[var_ptr].shadow_value)
            memcpy(var_ptr, var_map[var_ptr].shadow_value, var_map[var_ptr].size);
    }

    void updateShadow(void* var_ptr)
    {
        if (!var_map[var_ptr].shadow_value)
            var_map[var_ptr].shadow_value = new uint8_t[var_map[var_ptr].size];

        memcpy(var_map[var_ptr].shadow_value, var_ptr, var_map[var_ptr].size);
    }

    void markLiveValue(void* var_ptr)
    {
        memcpy(var_map[var_ptr].marked_live_value, var_ptr, var_map[var_ptr].size);
    }

    void markShadowValue(void* var_ptr)
    {
        if (var_map[var_ptr].shadow_value)
            memcpy(var_map[var_ptr].marked_shadow_value, var_map[var_ptr].shadow_value, var_map[var_ptr].size);
    }

    void markLiveValues()
    {
        for (const auto& item : var_map)
            markLiveValue(item.first);
    }

    void markShadowValues()
    {
        for (const auto& item : var_map)
            markShadowValue(item.first);
    }

    void copyChangedShadowVarsToLive()
    {
        for (const auto& item : var_map)
        {
            if (shadowChanged(item.first))
                updateLive(item.first);
        }
    }
    
    void copyChangedLiveVarsToShadow()
    {
        for (const auto& item : var_map)
        {
            if (liveChanged(item.first))
                updateShadow(item.first);
        }
    }

    bool liveChanged(void* var_ptr)
    {
        return memcmp(var_map[var_ptr].marked_live_value, var_ptr, var_map[var_ptr].size) != 0;
    }

    bool shadowChanged(void* var_ptr)
    {
        if (var_map[var_ptr].shadow_value)
            return memcmp(var_map[var_ptr].marked_shadow_value, var_map[var_ptr].shadow_value, var_map[var_ptr].size) != 0;
        return false;
    }

    std::string toString() const
    {
        std::stringstream txt;
        for (const auto& pair : var_map)
            txt << "Offset: " << pair.second.offset << " bytes,  Size: " << pair.second.size << "\n";
        return txt.str();
    }
};*/

class Layout;
class Viewport;
class ProjectBase;
class ProjectManager;
struct ImDebugLog;


class SceneBase : public VariableChangedTracker//, public VarTracker
{
    mutable std::mt19937 gen;

    int dt_sceneProcess = 0;

    mutable std::vector<Math::MovingAverage::MA> dt_scene_ma_list;
    mutable std::vector<Math::MovingAverage::MA> dt_project_ma_list;
    //mutable std::vector<Math::MovingAverage::MA> dt_project_draw_ma_list;

    mutable size_t dt_call_index = 0;
    mutable size_t dt_process_call_index = 0;
    //size_t dt_draw_call_index = 0;

protected:

    friend class Viewport;
    friend class ProjectBase;

    ProjectBase* project = nullptr;

    int scene_index = -1;
    std::vector<Viewport*> mounted_to_viewports;

    // Mounting to/from viewport
    void registerMount(Viewport* viewport);
    void registerUnmount(Viewport* viewport);

    //
    bool has_var_buffer = false;
    virtual void _sceneAttributes() {}

    virtual bool changedShadow() { return false; }
    virtual void copyChangedShadowVarsToLive() {}
    virtual void copyChangedLiveVarsToShadow() {}
    virtual void markLiveValues() {}
    virtual void markShadowValues() {}

    //
    
    void _onEvent(Event e) 
    {
        onEvent(e);

        switch (e.type())
        {
        case SDL_FINGERDOWN:       onPointerDown(PointerEvent(e));  break;
        case SDL_MOUSEBUTTONDOWN:  onPointerDown(PointerEvent(e));  break;
        case SDL_FINGERUP:         onPointerUp(PointerEvent(e));    break;
        case SDL_MOUSEBUTTONUP:    onPointerUp(PointerEvent(e));    break;
        case SDL_FINGERMOTION:     onPointerMove(PointerEvent(e));  break;
        case SDL_MOUSEMOTION:      onPointerMove(PointerEvent(e));  break;
        case SDL_MOUSEWHEEL:       onWheel(PointerEvent(e));        break;

        case SDL_KEYDOWN:          onKeyDown(KeyEvent(e));          break;
        case SDL_KEYUP:            onKeyUp(KeyEvent(e));            break;
        }
    }

public:

    std::shared_ptr<void> temporary_environment;

    Camera* camera = nullptr;
    MouseInfo* mouse = nullptr;

    struct Config {};

    SceneBase() : gen(std::random_device{}())
    {
        ///ImaginaryScene test;

        ///test.process();

        ///ImaginaryScene::Shadow::
        //ImaginaryScene::Shadow::sceneAttributes(
        //    reinterpret_cast<ImaginaryScene::Shadow&>(test));

    }
    virtual ~SceneBase() = default;

    void mountTo(Viewport* viewport);
    void mountTo(Layout& viewports);
    void mountToAll(Layout& viewports);

    virtual void sceneStart() {}
    virtual void sceneMounted(Viewport* ctx) {}
    virtual void sceneStop() {}
    virtual void sceneDestroy() {}
    virtual void sceneProcess() {}
    virtual void viewportProcess(Viewport* ctx) {}
    virtual void viewportDraw(Viewport* ctx) const = 0;

    virtual void onEvent(Event e) {}

    void pollEvents();
    void pullDataFromShadow();

    virtual void onPointerEvent(PointerEvent e) {}
    virtual void onPointerDown(PointerEvent e) {}
    virtual void onPointerUp(PointerEvent e) {}
    virtual void onPointerMove(PointerEvent e) {} 
    virtual void onWheel(PointerEvent e) {}
    virtual void onKeyDown(KeyEvent e) {}
    virtual void onKeyUp(KeyEvent e) {}

    void handleWorldNavigation(Event e, bool single_touch_pan);

    virtual std::string name() const { return "Scene"; }
    [[nodiscard]] int sceneIndex() const { return scene_index; }

    [[nodiscard]] double frame_dt(int average_samples = 1) const;
    [[nodiscard]] double scene_dt(int average_samples = 1) const;
    [[nodiscard]] double project_dt(int average_samples = 1) const;

    [[nodiscard]] double fps(int average_samples = 1) const { return 1000.0 / frame_dt(average_samples); }

    ///FRect combinedViewportsRect()
    ///{
    ///    FRect ret{};
    ///    for (Viewport* ctx : viewports)
    ///    {
    ///        ctx->viewportRect();
    ///    }
    ///}

    [[nodiscard]] double random(double min = 0, double max = 1) const
    {
        std::uniform_real_distribution<> dist(min, max);
        return dist(gen);
    }

    [[nodiscard]] DVec2 Offset(double stage_offX, double stage_offY) const
    {
        return camera->toWorldOffset({ stage_offX, stage_offY });
    }

    void logMessage(std::string_view, ...);
    void logClear();
};

struct VarBuffer : public VarTracker
{
    VarBuffer() = default;
    VarBuffer(const VarBuffer&) = delete;

    virtual void setup() {};
    virtual void populate() {}
};

template<typename T>
concept DerivedFromVarBuffer = std::derived_from<T, VarBuffer>;

template<DerivedFromVarBuffer VarBufferType>
class Scene : public SceneBase, public VarBufferType
{
    friend class ProjectBase;

    VarBufferType shadow_attributes;

    VariableMap var_map;

public:

    Scene() : SceneBase(), shadow_attributes()
    {
        has_var_buffer = true;
        ///shadow_attributes.live_attributes = this;

        // Build both buffer sync lists
        shadow_attributes.setup();
        VarBufferType::setup();

        for (size_t i = 0; i < VarBufferType::buffer_var_ptrs.size(); i++)
        {
            VariableEntry entry;
            entry.shadow_ptr    = shadow_attributes.buffer_var_ptrs[i];
            entry.live_ptr      = VarBufferType::buffer_var_ptrs[i];
            entry.shadow_marked = entry.shadow_ptr->clone();
            entry.live_marked   = entry.live_ptr->clone();

            entry.id = (int)i;
            entry.name = entry.live_ptr->name;
            //entry.print();

            var_map.push_back(entry);
        }

        // Taken ownership of pointers, clear lists
        shadow_attributes.buffer_var_ptrs.clear();
        VarBufferType::buffer_var_ptrs.clear();
    }


protected:

    //bool shadow_marked = false;

    bool changedShadow() override { return var_map.changedShadow(); }
    void markLiveValues() override 
    {
        DebugPrint("markLiveValues()");
        var_map.markLiveValues();
    }
    void markShadowValues() override
    { 
        //if (!shadow_marked) 
        { 
            DebugPrint("markShadowValues()");
            var_map.markShadowValues();
            //shadow_marked = true; 
        }
    }

    void copyChangedShadowVarsToLive() override 
    {
        DebugPrint("updateLiveBuffer()");
        for (VariableEntry& entry : var_map) {
            //if (entry.pendingChange()) // if we have not changed live since start of worker frame
            //if (!entry.shadowChanged()) // if we have not changed live since start of worker frame
                entry.updateLive();
        }
        //shadow_marked = false;
    }
    void copyChangedLiveVarsToShadow() override {
        DebugPrint("updateShadowBuffer()");
        for (VariableEntry& entry : var_map) {
            if (entry.liveChanged()) // if we have not changed shadow since start of worker frame
            //if (entry.pendingChange()) // if we have not changed shadow since start of worker frame
            {
                entry.updateShadow();
            }
        }
    }

    virtual void _sceneAttributes() override 
    {
        shadow_attributes.populate();
    }

    std::string pad(const std::string& str, int width) const
    {
        return str + std::string(width > str.size() ? width - str.size() : 0, ' ');
    }

    std::string toString() const
    {
        static const int name_len = 28;
        static const int val_len = 10;

        std::stringstream txt;
        for (const auto& entry : var_map)
            txt << pad(std::to_string(entry.id) + ". ", 4) << pad(entry.name, name_len)
            << "SHADOW_MARKED: " << pad(entry.shadow_marked->toString(), val_len) << "   SHADOW_VAL: " << pad(entry.shadow_ptr->toString(), val_len)
            << "    LIVE_MARKED:   " << pad(entry.live_marked->toString(), val_len) << "   LIVE_VAL: " << pad(entry.live_ptr->toString(), val_len)
            << "\n";

        return txt.str();
    }
};

class Viewport : public Painter
{
    std::string print_text;
    std::stringstream print_stream;

protected:

    friend class ProjectBase;
    friend class SceneBase;
    friend class Layout;
    friend class Camera;

    int viewport_index;
    int viewport_grid_x;
    int viewport_grid_y;

    Layout* layout = nullptr;
    SceneBase* scene = nullptr;

    double x = 0;
    double y = 0;

public:
    
    double width = 0;
    double height = 0;

    Viewport(
        Layout* layout,
        Canvas *canvas,
        int viewport_index,
        int grid_x,
        int grid_y
    );

    ~Viewport();

    void draw();

    [[nodiscard]] int viewportIndex() { return viewport_index; }
    [[nodiscard]] int viewportGridX() { return viewport_grid_x; }
    [[nodiscard]] int viewportGridY() { return viewport_grid_y; }
    [[nodiscard]] DRect viewportRect() { return DRect(x, y, x + width, y + height); }
    [[nodiscard]] DQuad worldQuad() { return camera.toWorldQuad(0, 0, width, height); }

    template<typename T>
    T* mountScene(T* _sim)
    {
        DebugPrint("Mounting existing scene to Viewport: %d", viewport_index);
        scene = _sim;
        scene->registerMount(this);
        return _sim;
    }

    // Viewport-specific draw helpers (i.e. size of viewport needed)
    void printTouchInfo();

    void drawWorldAxis(
        double axis_opacity = 0.3,
        double grid_opacity = 0.04,
        double text_opacity = 0.4
    );

    std::stringstream& print() {
        return print_stream;
    }
};


struct SimSceneList : public std::vector<SceneBase*>
{
    void mountTo(Layout& viewports);
};

class Layout
{
    std::vector<Viewport*> viewports;

protected:

    friend class ProjectBase;
    friend class SceneBase;

    // If 0, viewports expand in that direction. If both 0, expand whole grid.
    int targ_viewports_x = 0;
    int targ_viewports_y = 0;
    int cols = 0;
    int rows = 0;

    std::vector<SceneBase*> all_scenes;
    ProjectBase* project = nullptr;

public:

    using iterator = typename std::vector<Viewport*>::iterator;
    using const_iterator = typename std::vector<Viewport*>::const_iterator;

    ~Layout()
    {
        // Only invoked when you switch project
        clear();
    }

    const std::vector<SceneBase*>& scenes() {
        return all_scenes;
    }

    void expandCheck(size_t count);
    void add(int _viewport_index, int _grid_x, int _grid_y);
    void resize(size_t viewport_count);
    void clear()
    {
        // layout freed each time you call setLayout
        for (Viewport* p : viewports) delete p;
        viewports.clear();
    }

    void setSize(int targ_viewports_x, int targ_viewports_y)
    {
        this->targ_viewports_x = targ_viewports_x;
        this->targ_viewports_y = targ_viewports_y;
    }


    [[nodiscard]] Viewport* operator[](size_t i) { expandCheck(i + 1); return viewports[i]; }
    Layout&   operator<<(SceneBase* scene) { scene->mountTo(*this); return *this; }
    Layout&   operator<<(std::shared_ptr<SimSceneList> scenes) { 
        for (SceneBase *scene : *scenes)
            scene->mountTo(*this);
        return *this;
    }

    [[nodiscard]] iterator begin() { return viewports.begin(); }
    [[nodiscard]] iterator end() { return viewports.end(); }

    [[nodiscard]] const_iterator begin() const { return viewports.begin(); }
    [[nodiscard]] const_iterator end() const { return viewports.end(); }

    [[nodiscard]] int count() const {
        return static_cast<int>(viewports.size());
    }
};

struct ProjectInfoNode
{
    std::shared_ptr<ProjectInfo> project_info = nullptr;
    std::vector<ProjectInfoNode> children;
    std::string name;
    bool open = true;

    ProjectInfoNode(std::string node_name) : name(node_name) {}
    ProjectInfoNode(std::shared_ptr<ProjectInfo> project)
    {
        project_info = project;
        name = project->path.back();
    }
};

class ProjectBase
{
    Layout viewports;
    int scene_counter = 0;
    int sim_uid = -1;

    int dt_projectProcess = 0;
    int dt_frameProcess = 0;

    std::chrono::steady_clock::time_point last_frame_time 
        = std::chrono::steady_clock::now();

    Viewport* ctx_focused = nullptr;
    Viewport* ctx_hovered = nullptr;

protected:

    friend class ProjectWorker;
    friend class Layout;
    friend class SceneBase;

    Canvas* canvas = nullptr;
    ImDebugLog* project_log = nullptr;

    // ----------- States -----------
    bool started = false;
    bool paused = false;
    bool done_single_process = false;

    void configure(int sim_uid, Canvas* canvas, ImDebugLog* project_log);

    [[nodiscard]] DVec2 surfaceSize(); // Dimensions of canvas (or FBO if recording)
    void updateViewportRects();

    // -------- Attributes --------
    bool has_var_buffer = false;
    void _populateAllAttributes();
    virtual void _projectAttributes() {}
    virtual void _updateLiveProjectAttributes() {}
    virtual void _updateShadowProjectAttributes() {}

    ///void commitVariableChangeTracker()
    ///{
    ///    for (SceneBase* scene : viewports.all_scenes)
    ///        scene->variableChangedUpdateCurrent();
    ///}
    
    bool changedShadow()
    {
        for (SceneBase* scene : viewports.all_scenes)
        {
            if (scene->changedShadow())
                return true;
        }
        return false;
    }

    void markLiveValues()
    {
        for (SceneBase* scene : viewports.all_scenes)
            scene->markLiveValues();
    }
    void markShadowValues()
    {
        for (SceneBase* scene : viewports.all_scenes)
            scene->markShadowValues();
    }

    // ---- Project Management ----
    void _projectPrepare();
    void _projectStart();
    void _projectStop();
    void _projectPause();
    void _projectDestroy();
    void _projectProcess();
    void _projectDraw();

    std::vector<FingerInfo> pressed_fingers;
    void _onEvent(SDL_Event& e);

public:

    MouseInfo mouse;

    virtual ~ProjectBase() = default;
    
    // BasicProject Factory methods
    static [[nodiscard]] std::vector<std::shared_ptr<ProjectInfo>>& projectInfoList()
    {
        static std::vector<std::shared_ptr<ProjectInfo>> info_list;
        return info_list;
    }

    static [[nodiscard]] ProjectInfoNode& projectTreeRootInfo()
    {
        static ProjectInfoNode root("root");
        return root;
    }

    static [[nodiscard]] std::shared_ptr<ProjectInfo> findProjectInfo(int sim_uid)
    {
        for (auto& info : projectInfoList())
        {
            if (info->sim_uid == sim_uid)
                return info;
        }
        return nullptr;
    }

    static [[nodiscard]] std::shared_ptr<ProjectInfo> findProjectInfo(const char *name)
    {
        for (auto& info : projectInfoList())
        {
            if (info->path.back() == name)
                return info;
        }
        return nullptr;
    }

    static void addProjectInfo(const std::vector<std::string>& tree_path, const ProjectCreatorFunc& func)
    {
        static int factory_sim_index = 0;

        std::vector<std::shared_ptr<ProjectInfo>>& project_list = projectInfoList();

        auto project_info = std::make_shared<ProjectInfo>(ProjectInfo(
            tree_path,
            func,
            factory_sim_index++,
            ProjectInfo::INACTIVE
        ));

        project_list.push_back(project_info);

        ProjectInfoNode* current = &projectTreeRootInfo();

        // Traverse tree and insert info in correct nested category
        for (size_t i = 0; i < tree_path.size(); i++)
        {
            const std::string& insert_name = tree_path[i];
            bool is_leaf = (i == (tree_path.size() - 1));

            if (is_leaf)
            {
                current->children.push_back(ProjectInfoNode(project_info));
                break;
            }
            else
            {
                // Does category already exist?
                ProjectInfoNode* found_existing = nullptr;
                for (ProjectInfoNode& existing_node : current->children)
                {
                    if (existing_node.name == insert_name)
                    {
                        // Yes
                        found_existing = &existing_node;
                        break;
                    }
                }
                if (found_existing)
                {
                    current = found_existing;
                }
                else
                {
                    current->children.push_back(ProjectInfoNode(insert_name));
                    current = &current->children.back();
                }
            }
        }

        std::sort(project_list.begin(), project_list.end(),
            [](auto a, auto b)
        { 
            return a->path < b->path; 
        });
    }

    [[nodiscard]] std::shared_ptr<ProjectInfo> getProjectInfo()
    {
        return findProjectInfo(sim_uid);
    }

    void setProjectInfoState(ProjectInfo::State state);

    /// todo: Find a way to make protected


    void copyChangedShadowVarsToLive()
    {
        _updateLiveProjectAttributes();
        for (SceneBase* scene : viewports.all_scenes)
            scene->copyChangedShadowVarsToLive();
    }
    void copyChangedLiveVarsToShadow()
    {
        _updateShadowProjectAttributes();
        for (SceneBase* scene : viewports.all_scenes)
            scene->copyChangedLiveVarsToShadow();
    }

    // Shared Scene creators

    template<typename SceneType>
    [[nodiscard]] SceneType* create()
    {
        auto config = std::make_shared<typename SceneType::Config>();
        SceneType* scene;

        if constexpr (std::is_constructible_v<SceneType, typename SceneType::Config&>)
        {
            scene = new SceneType(*config);
            scene->temporary_environment = config;

        }
        else
            scene = new SceneType();

        scene->project = this;
        scene->scene_index = scene_counter++;
        //scene->setGLFunctions(canvas->getGLFunctions());

        return scene;
    }

    template<typename SceneType> [[nodiscard]] SceneType* create(typename SceneType::Config config)
    {
        auto config_ptr = std::make_shared<typename SceneType::Config>(config);

        SceneType* scene = new SceneType(*config_ptr);
        scene->temporary_environment = config_ptr;
        scene->scene_index = scene_counter++;
        scene->project = this;
        //scene->setGLFunctions(canvas->getGLFunctions());

        return scene;
    }

    template<typename SceneType>
    [[nodiscard]] SceneType* create(std::shared_ptr<typename SceneType::Config> config)
    {
        if (!config)
            throw "Launch Config wasn't created";

        SceneType* scene = new SceneType(*config);
        scene->temporary_environment = config;
        scene->scene_index = scene_counter++;
        scene->project = this;
        //scene->setGLFunctions(canvas->getGLFunctions());

        return scene;
    }

    template<typename SceneType>
    [[nodiscard]] std::shared_ptr<SimSceneList> create(int count)
    {
        auto ret = std::make_shared<SimSceneList>();
        for (int i = 0; i < count; i++)
        {
            SceneType* scene = create<SceneType>();
            scene->project = this;
            //scene->setGLFunctions(canvas->getGLFunctions());

            ret->push_back(scene);
        }
        return ret;
    }

    template<typename SceneType>
    [[nodiscard]] std::shared_ptr<SimSceneList> create(int count, typename SceneType::Config config)
    {
        auto ret = std::make_shared<SimSceneList>();
        for (int i = 0; i < count; i++)
        {
            SceneType* scene = create<SceneType>(config);
            scene->project = this;
            //scene->setGLFunctions(canvas->getGLFunctions());

            ret->push_back(scene);
        }
        return ret;
    }

    Layout& newLayout();
    Layout& newLayout(int _viewports_x, int _viewports_y);
    [[nodiscard]] Layout& currentLayout()
    {
        return viewports;
    }

    [[nodiscard]] int fboWidth() { return canvas->fboWidth(); }
    [[nodiscard]] int fboHeight() { return canvas->fboHeight(); }

    virtual void projectAttributes() {}
    virtual void projectPrepare(Layout& layout) = 0;
    virtual void projectStart() {}
    virtual void projectStop() {}
    virtual void projectDestroy() {}

    virtual void onEvent(Event& e) {}

    void pullDataFromShadow();
    void pollEvents();

    void logMessage(const char* fmt, ...);
    void logClear();
};

template<VarBufferConcept VarBufferType>
class Project : public ProjectBase, public VarBufferType
{
    friend class ProjectBase;

    VarBufferType shadow_attributes;

public:

    Project() : ProjectBase(), shadow_attributes()
    {
        has_var_buffer = true;
    }

protected:

    virtual void _projectAttributes() override
    {
        shadow_attributes.populate();
    }

    void _updateLiveProjectAttributes() override
    {
        if constexpr (HasSyncMembers<VarBufferType>)
        {
            auto dst = writable_sync_span(*static_cast<VarBufferType*>(this));
            auto src = readonly_sync_span(shadow_attributes);
            std::memcpy(dst.data(), src.data(), dst.size());
        }

        // Give sim a chance to manually copy non-trivially copyable data
        VarBufferType* this_attributes = dynamic_cast<VarBufferType*>(this);
        this_attributes->copyFrom(shadow_attributes);
    }
    void _updateShadowProjectAttributes() override
    {
        if constexpr (HasSyncMembers<VarBufferType>)
        {
            auto dst = writable_sync_span(shadow_attributes);
            auto src = readonly_sync_span(*static_cast<VarBufferType*>(this));
            std::memcpy(dst.data(), src.data(), dst.size());
        }

        // Give sim a chance to manually copy non-trivially copyable data
        VarBufferType* this_attributes = dynamic_cast<VarBufferType*>(this);
        shadow_attributes.copyFrom(*this);
    }
};

typedef ProjectBase BasicProject;
typedef SceneBase BasicScene;

template <typename T>
struct AutoRegisterProject
{
    AutoRegisterProject(const std::vector<std::string>& tree_path)
    {
        ProjectBase::addProjectInfo(tree_path, []() -> ProjectBase* {
            return (ProjectBase*)(new T());
        });
    }
};
