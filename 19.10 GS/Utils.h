#pragma once
#include "pch.h"

inline FName LogCategory;

#define Log(format, ...) ((void(*)(const char*, int32, const FName&, uint8, const wchar_t*, ...)) (ImageBase + 0xd34d3c))(nullptr, 0, LogCategory, 4, format, ##__VA_ARGS__);

class Utils {
    static inline void* _NpFH = nullptr;
public:
    template <class _Ot = void*>
    static void Hook(uint64_t _Ptr, void* _Detour, _Ot& _Orig = _NpFH) {
        MH_CreateHook((LPVOID)_Ptr, _Detour, (LPVOID*)(std::is_same_v<_Ot, void*> ? nullptr : &_Orig));
    }

    static FString generateIslandCodeThing()
    {
        std::wstringstream ss;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 9999);

        for (int i = 0; i < 3; ++i)
        {
            ss << std::setw(4) << std::setfill(L'0') << dist(gen);
            if (i < 2) ss << L"-";
        }

        return FString(ss.str().c_str());
    }

    static void KillGameserver()
    {
        std::thread([] {
            constexpr int delaySeconds = 5;
            auto start = std::chrono::steady_clock::now();

            while (true)
            {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();

                if (elapsed >= delaySeconds)
                    break;

                std::this_thread::yield();
            }

            std::system("taskkill /f /im FortniteClient-Win64-Shipping.exe");
            }).detach();
    }


    __forceinline static void _HookVT(void** _Vt, uint32_t _Ind, void* _Detour)
    {
        DWORD _Vo;
        VirtualProtect(_Vt + _Ind, 8, PAGE_EXECUTE_READWRITE, &_Vo);
        _Vt[_Ind] = _Detour;
        VirtualProtect(_Vt + _Ind, 8, _Vo, &_Vo);
    }

    template <typename _Ct, typename _Ot = void*>
    __forceinline static void Hook(uint32_t _Ind, void* _Detour, _Ot& _Orig = _NpFH)
    {
        auto _Vt = _Ct::GetDefaultObj()->VTable;
        if (!std::is_same_v<_Ot, void*>)
            _Orig = (_Ot)_Vt[_Ind];

        _HookVT(_Vt, _Ind, _Detour);
    }

    template <typename _Ct>
    __forceinline static void HookEvery(uint32_t _Ind, void* _Detour)
    {
        for (int i = 0; i < UObject::GObjects->Num(); i++) {
            auto Obj = UObject::GObjects->GetByIndex(i);
            if (Obj && Obj->IsA<_Ct>()) {
                _HookVT(Obj->VTable, _Ind, _Detour);
            }
        }
    }

    template <typename _Is>
    static __forceinline void Patch(uintptr_t ptr, _Is byte)
    {
        DWORD og;
        VirtualProtect(LPVOID(ptr), sizeof(_Is), PAGE_EXECUTE_READWRITE, &og);
        *(_Is*)ptr = byte;
        VirtualProtect(LPVOID(ptr), sizeof(_Is), og, &og);
    }

    __forceinline static UObject* InternalFindObject(const wchar_t* ObjectPath, UClass* Class)
    {
        return Funcs::StaticFindObject(Class, nullptr, ObjectPath, false);
    }

    __forceinline static UObject* InternalLoadObject(const wchar_t* ObjectPath, UClass* InClass, UObject* Outer = nullptr)
    {
        return Funcs::StaticLoadObject(InClass, Outer, ObjectPath, nullptr, 0, nullptr, false);
    }

    static UObject* FindObject(const wchar_t*, UClass*);

    template <typename _Ot>
    static _Ot* FindObject(const wchar_t* ObjectPath, UClass* Class = _Ot::StaticClass())
    {
        return (_Ot*)FindObject(ObjectPath, Class);
    }

    template <typename _Ot>
    static _Ot* FindObject(UEAllocatedWString ObjectPath, UClass* Class = _Ot::StaticClass())
    {
        return (_Ot*)FindObject(ObjectPath.c_str(), Class);
    }

    static TArray<AActor*> GetAll(UClass* Class);

    template <typename _At = AActor>
    __forceinline static TArray<_At*> GetAll(UClass* Class)
    {
        return GetAll(Class);
    }

    template <typename _At = AActor>
    __forceinline static TArray<_At*> GetAll()
    {
        return GetAll(_At::StaticClass());
    }

    static AActor* SpawnActor(UClass* Class, FTransform& Transform, AActor* Owner = nullptr);

    static AActor* SpawnActor(UClass* Class, FVector& Loc, FRotator& Rot, AActor* Owner = nullptr);

    template <typename T = AActor>
    static T* SpawnActorUnfinished(UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
    {
        FTransform Transform(Loc, Rot);

        return (T*)UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, Owner);
    }

    template <typename T = AActor>
    static T* FinishSpawnActor(AActor* Actor, FVector Loc, FRotator Rot)
    {
        FTransform Transform(Loc, Rot);

        return (T*)UGameplayStatics::FinishSpawningActor(Actor, Transform);
    };


    template <typename T>
    static T* SpawnActor(UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
    {
        return (T*)SpawnActor(Class, Loc, Rot, Owner);
    }

    template <typename T>
    static T* SpawnActor(UClass* Class, FTransform& Transform, AActor* Owner = nullptr)
    {
        return (T*)SpawnActor(Class, Transform, Owner);
    }

    template <typename T>
    static T* SpawnActor(FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
    {
        return (T*)SpawnActor(T::StaticClass(), Loc, Rot, Owner);
    }

    template <typename T>
    static T* SpawnActor(FTransform& Transform, AActor* Owner = nullptr)
    {
        return (T*)SpawnActor(T::StaticClass(), Transform, Owner);
    }

    template <typename _Ot = void*>
    __forceinline static void ExecHook(UFunction* _Fn, void* _Detour, _Ot& _Orig = _NpFH)
    {
        if (!_Fn)
            return;
        if (!std::is_same_v<_Ot, void*>)
            _Orig = (_Ot)_Fn->ExecFunction;

        _Fn->ExecFunction = reinterpret_cast<UFunction::FNativeFuncPtr>(_Detour);
    }


    template <typename _Ot = void*>
    __forceinline static void ExecHook(const wchar_t* _Name, void* _Detour, _Ot& _Orig = _NpFH)
    {
        UFunction* _Fn = FindObject<UFunction>(_Name);
        ExecHook(_Fn, _Detour, _Orig);
    }


    template <typename _It>
    static _It* GetInterface(UObject* Object)
    {
        return ((_It * (*)(UObject*, UClass*)) (ImageBase + 0x38ae380))(Object, _It::StaticClass());
    }

    static float precision(float f, float places)
    {
        float n = powf(10.f, places);
        return roundf(f * n) / n;
    }

    static float EvaluateScalableFloat(FScalableFloat& Float);
};

class alignas(0x8) FOutputDevice
{
public:
    bool bSuppressEventTag;
    bool bAutoEmitLineTerminator;
};

struct FOutParmRec
{
    UProperty* Property;
    uint8* PropAddr;
    FOutParmRec* NextOutParm;
};

class FFrame : public FOutputDevice
{
public:
    void** VTable;
    UFunction* Node;
    UObject* Object;
    uint8* Code;
    uint8* Locals;
    FProperty* MostRecentProperty;
    uint8_t* MostRecentPropertyAddress;
    TArray<void*> FlowStack;
    FFrame* PreviousFrame;
    FOutParmRec* OutParms;
    uint8_t _Padding1[0x20]; // wtf else do they store here
    FField* PropertyChainForCompiledIn;
    UFunction* CurrentNativeFunction;
    bool bArrayContextFailed;

public:
    void StepCompiledIn(void* const Result = nullptr, bool ForceExplicitProp = false);

    template <typename T>
    __forceinline T& StepCompiledInRef() {
        T TempVal{};
        MostRecentPropertyAddress = nullptr;

        if (Code)
        {
            Funcs::Step(this, Object, &TempVal);
        }
        else
        {
            FField* _Prop = PropertyChainForCompiledIn;
            PropertyChainForCompiledIn = _Prop->Next;
            Funcs::StepExplicitProperty(this, &TempVal, _Prop);
        }

        return MostRecentPropertyAddress ? *(T*)MostRecentPropertyAddress : TempVal;
    }

    void IncrementCode();
};
static_assert(offsetof(FFrame, Object) == 0x18, "FFrame::Object offset is wrong!");
static_assert(offsetof(FFrame, Code) == 0x20, "FFrame::Code offset is wrong!");
static_assert(offsetof(FFrame, MostRecentPropertyAddress) == 0x38, "FFrame::MostRecentPropertyAddress offset is wrong!");
static_assert(offsetof(FFrame, PropertyChainForCompiledIn) == 0x80, "FFrame::PropertyChainForCompiledIn offset is wrong!");


inline std::vector<void(*)()> _HookFuncs;
#define DefHookOg(_Rt, _Name, ...) static inline _Rt (*_Name##OG)(##__VA_ARGS__); static _Rt _Name(##__VA_ARGS__); 
#define DefUHookOg(_Name) static inline void (*_Name##OG)(UObject*, FFrame&); static void _Name(UObject*, FFrame&); 
#define DefUHookOgRet(_Rt, _Name) static inline void (*_Name##OG)(UObject*, FFrame&, _Rt*); static void _Name(UObject *, FFrame&, _Rt*);
#define InitHooks static void Hook(); static int _AddHook() { _HookFuncs.push_back(Hook); return 0; }; static inline auto _HookAdder = _AddHook();
#define callOG(_Tr, _Fn, _Th, ...) ([&](){ _Fn->ExecFunction = (UFunction::FNativeFuncPtr) _Th##OG; _Tr->_Th(##__VA_ARGS__); _Fn->ExecFunction = (UFunction::FNativeFuncPtr) _Th; })()
#define callOGWithRet(_Tr, _Fn, _Th, ...) ([&](){ _Fn->ExecFunction = (UFunction::FNativeFuncPtr) _Th##OG; auto _Rt = _Tr->_Th(##__VA_ARGS__); _Fn->ExecFunction = (UFunction::FNativeFuncPtr) _Th; return _Rt; })()
