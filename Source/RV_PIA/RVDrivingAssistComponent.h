#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RVDrivingAssistComponent.generated.h"

class AWheeledVehiclePawn;
class UChaosWheeledVehicleMovementComponent;
class UStaticMeshComponent;
class ARVUrbanScenario;

/** Runtime training rules, independent of editor Python and input-device backend. */
UCLASS(ClassGroup=(RutaVirtual), meta=(BlueprintSpawnableComponent))
class RV_PIA_API URVDrivingAssistComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URVDrivingAssistComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Training", meta=(ClampMin="5", ClampMax="60"))
    float ForwardLimitKmh = 30.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Training", meta=(ClampMin="1", ClampMax="10"))
    float ReverseLimitKmh = 5.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Training", meta=(ClampMin="0", ClampMax="2"))
    float GearChangeLimitKmh = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Training", meta=(ClampMin="200"))
    float CorridorHalfWidthCm = 450.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Training", meta=(ClampMin="2000"))
    float CorridorLengthCm = 6500.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Training") bool bReverseMode = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Training") bool bPracticePaused = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Training") int32 PracticeStage = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Urban") bool bUrbanMode = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Urban") int32 UrbanCheckpointIndex = 0;
    UFUNCTION(BlueprintCallable, Category="Training") bool TryToggleReverse();
    UFUNCTION(BlueprintCallable, Category="Training") void ResetPractice();
    UFUNCTION(BlueprintCallable, Category="Training") void TogglePracticePause();
    UFUNCTION(BlueprintPure, Category="Training") FString GetInstruction() const;
    UFUNCTION(BlueprintPure, Category="Training") float GetSpeedKmh() const;
    UFUNCTION(BlueprintPure, Category="Training") float GetDistanceToGoalMeters() const;
    UFUNCTION(BlueprintPure, Category="Urban") FString GetUrbanSummary() const;
    UFUNCTION(BlueprintCallable, Category="Urban") void RequestHorn();

private:
    UPROPERTY() TObjectPtr<AWheeledVehiclePawn> Vehicle;
    UPROPERTY() TObjectPtr<UChaosWheeledVehicleMovementComponent> Movement;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> SteeringWheel;
    UPROPERTY() TObjectPtr<ARVUrbanScenario> UrbanScenario;
    FTransform InitialTransform;
    FQuat InitialWheelRotation;
    FString Status;
    float StatusSeconds = 0.f;
    bool bInputTickLinked = false;
    bool bRequireThrottleRelease = false;
    bool bHadController = true;
    void StopInputs();
    void SetStatus(const FString& Message);
};
