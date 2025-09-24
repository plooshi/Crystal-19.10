#include "pch.h"
#include "Utils.h"

UObject* Utils::FindObject(const wchar_t* ObjectPath, UClass* Class)
{
    auto Object = InternalFindObject(ObjectPath, Class);
    return Object ? Object : InternalLoadObject(ObjectPath, Class);
}

TArray<AActor*> Utils::GetAll(UClass* Class)
{
    TArray<AActor*> ret;
    UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), Class, &ret);
    return ret;
}

// Basic.hpp
FName::FName(FString str) {
    *this = UKismetStringLibrary::Conv_StringToName(str);
}

UObject* SDK::InternalGet(FSoftObjectPtr* Ptr, UClass* Class)
{
    if (!Ptr)
        return nullptr;

    auto Ret = Ptr->WeakPtr.ObjectIndex && Ptr->WeakPtr.ObjectSerialNumber ? Ptr->Get() : nullptr;
    if ((!Ret || !Ret->IsA(Class)) && Ptr->ObjectID.AssetPathName.ComparisonIndex > 0) {
        Ptr->WeakPtr = Ret = Utils::FindObject(Ptr->ObjectID.AssetPathName.ToWString().c_str(), Class);
    }
    return Ret;
}

enum ESpawnActorNameMode : uint8
{
    Required_Fatal,
    Required_ErrorAndReturnNull,
    Required_ReturnNull,
    Requested
};

struct FActorSpawnParameters
{
public:
    FName Name;

    AActor* Template;
    AActor* Owner;
    APawn* Instigator;
    ULevel* OverrideLevel;
    UChildActorComponent* OverrideParentComponent;
    ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride;

private:
    uint8	bRemoteOwned : 1;
public:
    uint8	bNoFail : 1;
    uint8	bDeferConstruction : 1;
    uint8	bAllowDuringConstructionScript : 1;
    ESpawnActorNameMode NameMode;
    EObjectFlags ObjectFlags;
};

AActor* Utils::SpawnActor(UClass* Class, FTransform& Transform, AActor* Owner)
{
    FActorSpawnParameters addr{};

    addr.Owner = Owner;
    addr.bDeferConstruction = false;
    addr.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    addr.NameMode = ESpawnActorNameMode::Requested;

    return ((AActor * (*)(UWorld*, UClass*, FTransform const*, FActorSpawnParameters*))(ImageBase + 0xd70e2c))(UWorld::GetWorld(), Class, &Transform, &addr);
    //return UGameplayStatics::FinishSpawningActor(UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, Owner), Transform);
}

AActor* Utils::SpawnActor(UClass* Class, FVector& Loc, FRotator& Rot, AActor* Owner)
{
    FTransform Transform(Loc, Rot);
    return SpawnActor(Class, Transform, Owner);
}

void FFrame::StepCompiledIn(void* const Result, bool ForceExplicitProp)
{
    if (Code && !ForceExplicitProp)
    {
        Funcs::Step(this, Object, Result);
    }
    else
    {
        FField* _Prop = PropertyChainForCompiledIn;
        PropertyChainForCompiledIn = _Prop->Next;
        Funcs::StepExplicitProperty(this, Result, _Prop);
    }
}


void FFrame::IncrementCode() {
    Code = (uint8_t*)(__int64(Code) + (bool)Code);
}

float FRotator::UnwindDegrees(float Angle) {

    Angle = fmod(Angle, 360.0f); // rat

    if (Angle < 0.)
        Angle += 360.f;

    if (Angle > 180.)
        Angle -= 360.f;

    return Angle;
}

float FRotator::ClampAxis(float Angle)
{
    Angle = fmod(Angle, 360.f); // rat

    if (Angle < 0.)
        Angle += 360.;
    return Angle;
}

FRotator FQuat::Rotator()
{
    const float SingularityTest = Z * X - W * Y;
    const float YawY = 2.f * (W * Z + X * Y);
    const float YawX = (1.f - 2.f * ((Y * Y) + (Z * Z)));

    const float SINGULARITY_THRESHOLD = 0.4999995f;
    const float RAD_TO_DEG = 57.29577951308232f;
    FRotator RotatorFromQuat{};

    if (SingularityTest < -SINGULARITY_THRESHOLD)
    {
        RotatorFromQuat.Pitch = -90.f;
        RotatorFromQuat.Yaw = atan2f(YawY, YawX) * RAD_TO_DEG;
        RotatorFromQuat.Roll = FRotator::NormalizeAxis(-RotatorFromQuat.Yaw - (2.f * atan2f(X, W) * RAD_TO_DEG));
    }
    else if (SingularityTest > SINGULARITY_THRESHOLD)
    {
        RotatorFromQuat.Pitch = 90.f;
        RotatorFromQuat.Yaw = atan2f(YawY, YawX) * RAD_TO_DEG;
        RotatorFromQuat.Roll = FRotator::NormalizeAxis(RotatorFromQuat.Yaw - (2.f * atan2f(X, W) * RAD_TO_DEG));
    }
    else
    {
        RotatorFromQuat.Pitch = asinf(2.f * SingularityTest) * RAD_TO_DEG;
        RotatorFromQuat.Yaw = atan2f(YawY, YawX) * RAD_TO_DEG;
        RotatorFromQuat.Roll = atan2f(-2.f * (W * X + Y * Z), (1.f - 2.f * ((X * X) + (Y * Y)))) * RAD_TO_DEG;
    }

    return RotatorFromQuat;
}

void UC::_TArrayAdd(void*& Data, int32& NumElements, int32& MaxElements, int32 _Elem_Sz, const void* Element) {
    if (NumElements + 1 > MaxElements) Data = FMemory::Realloc(Data, (MaxElements = NumElements + 1) * _Elem_Sz);

    __movsb(PBYTE(Data) + (NumElements * _Elem_Sz), (const PBYTE)Element, _Elem_Sz);
    NumElements++;
}

float Utils::EvaluateScalableFloat(FScalableFloat& Float)
{
    if (!Float.Curve.CurveTable)
        return Float.Value;

    float Out;
    UDataTableFunctionLibrary::EvaluateCurveTableRow(Float.Curve.CurveTable, Float.Curve.RowName, (float)0, nullptr, &Out, FString());
    return Out;
}