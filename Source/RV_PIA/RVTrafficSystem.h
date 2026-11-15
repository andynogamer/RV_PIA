#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RVTrafficSystem.generated.h"

class USplineComponent;
class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UAudioComponent;

UENUM(BlueprintType)
enum class ERVSignalColor : uint8 { Red, Amber, Green };

/** A lens from the imported city's fixtures, controlled without changing its asset. */
USTRUCT(BlueprintType)
struct FRVSignalLampBinding
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") TObjectPtr<UStaticMeshComponent> Component;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") bool bXAxis = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") ERVSignalColor Color = ERVSignalColor::Red;
};

/** Opposing directions share an axis; conflicting axes never receive green together. */
UCLASS()
class RV_PIA_API ARVTrafficSignal : public AActor
{
    GENERATED_BODY()
public:
    ARVTrafficSignal();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float GreenSeconds = 12.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float AmberSeconds = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float ClearanceSeconds = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float CycleOffset = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float StopLineOffsetCm = 900.f;
    /** -1 automatic; 0 green Y, 1 amber Y, 2 all red, 3 green X, 4 amber X, 5 all red. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") int32 PhaseOverride = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") TArray<FRVSignalLampBinding> CityLamps;
    UFUNCTION(BlueprintPure, Category="Traffic") int32 GetPhase() const;
    UFUNCTION(BlueprintPure, Category="Traffic") ERVSignalColor GetSignalForAxis(bool bXAxis) const;
    UFUNCTION(BlueprintCallable, Category="Traffic") void RefreshLights();
private:
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Lamps;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> LampMaterials;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> CityLampMaterials;
    int32 LastVisualPhase = INDEX_NONE;
};

UCLASS()
class RV_PIA_API ARVTrafficRoute : public AActor
{
    GENERATED_BODY()
public:
    ARVTrafficRoute();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Traffic") TObjectPtr<USplineComponent> Spline;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic", meta=(ClampMin="0", ClampMax="12")) int32 VehicleCount = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float CruiseKmh = 20.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRVHornRequested, AActor*, Vehicle, FName, Reason);

/** Lightweight kinematic traffic, with collision sweeps and physical obstacle detection. */
UCLASS()
class RV_PIA_API ARVTrafficVehicle : public AActor
{
    GENERATED_BODY()
public:
    ARVTrafficVehicle();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Traffic") TObjectPtr<UBoxComponent> Collision;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") TObjectPtr<ARVTrafficRoute> Route;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") FLinearColor BodyColor = FLinearColor(0.15f, 0.3f, 0.6f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float DistanceAlongRoute = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float AccelerationCmS2 = 130.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float BrakingCmS2 = 300.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float HornWaitSeconds = 5.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float HornCooldownSeconds = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float PreferredSpeedFactor = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float TimeHeadwaySeconds = 1.4f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traffic") float ReactionDelaySeconds = .5f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Traffic") bool bBrakeLightsOn = true;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Traffic") TObjectPtr<UAudioComponent> EngineAudio;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Traffic") float SpeedCmS = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Traffic") FName StopReason = NAME_None;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Traffic") int32 HornEventCount = 0;
    UPROPERTY(BlueprintAssignable, Category="Traffic") FRVHornRequested OnHornRequested;
    UFUNCTION(BlueprintCallable, Category="Traffic") void TeleportToDistance(float DistanceCm);
    UFUNCTION(BlueprintPure, Category="Traffic") float GetSpeedKmh() const { return SpeedCmS * 0.036f; }
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Wheels;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BrakeLamps;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> BrakeMaterials;
    void UpdateBrakeLights();
    float BlockedSeconds = 0.f;
    float LastHornTime = -100.f;
    float ReactionRemaining = 0.f;
    bool bWasWaitingAtSignal = false;
    float WheelSpin = 0.f;
};

/** Marks an urban map and owns the scenario, bounds and traffic population. */
UCLASS()
class RV_PIA_API ARVUrbanScenario : public AActor
{
    GENERATED_BODY()
public:
    ARVUrbanScenario();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Urban") FVector BoundsMin = FVector(-38500,-38000,-150);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Urban") FVector BoundsMax = FVector(38500,38000,20000);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Urban") TArray<FVector> Checkpoints;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Urban") bool bSpawnTraffic = true;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Urban") int32 RedLightWarnings = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Urban") int32 HornEvents = 0;
    UPROPERTY(BlueprintAssignable, Category="Urban") FRVHornRequested OnHornRequested;
    UFUNCTION(BlueprintPure, Category="Urban") bool IsInsideBounds(FVector Position) const;
    UFUNCTION(BlueprintCallable, Category="Urban") void RequestPlayerHorn(AActor* Vehicle);
    UFUNCTION(BlueprintCallable, Category="Urban") void ResetSessionProgress();
private:
    FVector PreviousPlayerPosition = FVector::ZeroVector;
    bool bHasPreviousPlayerPosition = false;
    float LastPlayerHornTime = -100.f;
};
