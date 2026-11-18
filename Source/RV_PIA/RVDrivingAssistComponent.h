#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RVWheelInput.h"
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
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Training") bool bWheelMode = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Training") int32 SelectedGear = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Training") bool bPracticePaused = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Training") int32 PracticeStage = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Urban") bool bUrbanMode = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Urban") int32 UrbanCheckpointIndex = 0;
    UFUNCTION(BlueprintCallable, Category="Training") bool TryToggleReverse();
    UFUNCTION(BlueprintCallable, Category="Training") void SubmitWheelSample(float Steering, float Throttle, float Brake, float Clutch, int32 Gear);
    UFUNCTION(BlueprintPure, Category="Training") FString GetGearLabel() const;
    UFUNCTION(BlueprintPure, Category="Training") FString GetPhysicalShifterLabel() const;
    UFUNCTION(BlueprintPure, Category="Training") bool IsWaitingForWheelReport() const { return !bKeyboardOnly && WheelInput && WheelInput->IsAutomaticWheelPresent() && !bHasPhysicalWheelSample; }
    UFUNCTION(BlueprintPure, Category="Training") bool HasWheelReading() const { return WheelInput && WheelInput->HasReading(); }
    int32 GetCalibrationStep() const { return WheelInput ? WheelInput->GetCalibrationStep() : INDEX_NONE; }
    bool NeedsWheelProfile() const { return WheelInput && !bKeyboardOnly && !WheelInput->HasLayout(); }
    bool NeedsWheelNeutral() const { return bWheelMode && bRequireWheelNeutral; }
    bool IsWaitingForClutch() const { return bWaitingForClutch; }
    bool HasDrivingNotice() const { return bWaitingForClutch || bShiftRejected || bRequireThrottleRelease || StatusSeconds > 0.f; }
    const FRVWheelSample& GetLastWheelSample() const { return LastWheelSample; }
    UFUNCTION(BlueprintCallable, Category="Training") void ResetPractice();
    UFUNCTION(BlueprintCallable, Category="Training") void TogglePracticePause();
    UFUNCTION(BlueprintPure, Category="Training") FString GetInstruction() const;
    UFUNCTION(BlueprintPure, Category="Training") float GetSpeedKmh() const;
    UFUNCTION(BlueprintPure, Category="Training") float GetPostedSpeedKmh() const;
    UFUNCTION(BlueprintPure, Category="Training") float GetDistanceToGoalMeters() const;
    UFUNCTION(BlueprintPure, Category="Urban") FString GetUrbanSummary() const;
    UFUNCTION(BlueprintCallable, Category="Urban") void RequestHorn();

    bool IsParkingPractice() const { return Parking != nullptr; }
    void ToggleWheelCalibration();
    void CaptureWheelCalibrationStep();
    void ToggleWheelMode();

private:
    UPROPERTY() TObjectPtr<AWheeledVehiclePawn> Vehicle;
    UPROPERTY() TObjectPtr<UChaosWheeledVehicleMovementComponent> Movement;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> SteeringWheel;
    UPROPERTY() TObjectPtr<class URVCabinComponent> Cabin;
    UPROPERTY() TObjectPtr<ARVUrbanScenario> UrbanScenario;
    UPROPERTY() TObjectPtr<class ARVParkingScenario> Parking;
    FTransform InitialTransform;
    FQuat InitialWheelRotation;
    FString Status;
    float StatusSeconds = 0.f;
    bool bInputTickLinked = false;
    bool bRequireThrottleRelease = false;
    bool bHadController = true;
    bool bKeyboardOnly = false;
    bool bHasPhysicalWheelSample = false;
    bool bRequireWheelNeutral = true;
    bool bShiftRejected = false;
    int32 LastShifterGear = 0;
    bool bWaitingForClutch = false;
    float ClutchWindowSeconds = 0.f;
    float ShifterStableSeconds = 0.f;
    TUniquePtr<FRVWheelInput> WheelInput;
    TOptional<FRVWheelSample> PendingWheelSample;
    FRVWheelSample LastWheelSample;
    void* GetWheelWindow() const;
    void SetWheelMode(bool bEnabled);
    void DisarmWheel();
    bool ApplyWheelInput(float DeltaTime, float& RequestedThrottle);
    void StopInputs();
    void SetStatus(const FString& Message);
};
