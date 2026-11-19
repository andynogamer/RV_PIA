#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RVCabinComponent.generated.h"

class UCameraComponent;
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UCanvas;
class UCanvasRenderTarget2D;
class UChaosWheeledVehicleMovementComponent;
class URVDrivingAssistComponent;

UCLASS()
class RV_PIA_API URVCabinComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URVCabinComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick) override;
    UFUNCTION(BlueprintCallable, Category="Cabin") void CycleMirror();
    UFUNCTION(BlueprintCallable, Category="Cabin") void FinishAdjustment();
    FString MirrorName() const;
    bool CenterHeadsetOnSeat();
    int32 GetSelectedMirror() const { return SelectedMirror; }
    UFUNCTION(BlueprintCallable, Category="Cabin") void AdjustMirror(float HorizontalDegrees, float VerticalDegrees);
private:
    UPROPERTY() TObjectPtr<UStaticMesh> CenterMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> SideMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> TrimMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> LeverMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> Reflection;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Mirrors;
    UPROPERTY() TArray<TObjectPtr<USceneCaptureComponent2D>> Captures;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Lever;
    UPROPERTY() TObjectPtr<UStaticMesh> CabinShellMesh;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ExteriorBody;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> CabinShell;
    bool bCabinShellActive = false;
    UPROPERTY() TObjectPtr<UStaticMesh> DisplayPlane;
    UPROPERTY() TObjectPtr<UCanvasRenderTarget2D> ClusterDisplay;
    UPROPERTY() TObjectPtr<UCanvasRenderTarget2D> ConsoleDisplay;
    UPROPERTY() TObjectPtr<UChaosWheeledVehicleMovementComponent> Movement;
    UPROPERTY() TObjectPtr<UCameraComponent> CabinCamera;
    UPROPERTY() TObjectPtr<URVDrivingAssistComponent> Assist;
    FVector2D Angles[3]{};
    FQuat MirrorMeshToMount[3];
    FQuat LeverMeshToMount = FQuat::Identity;
    int32 SelectedMirror = INDEX_NONE;
    int32 NextMirror = 0;
    float CaptureCountdown = 0.f;
    int32 DisplaySpeed = -1;
    int32 DisplayRpm = -1;
    FString DisplayGear;
    FString DisplayInstruction;
    int32 DisplayPedal = -1;
    int32 DisplayShifter = 0;
    int32 DisplaySteering = 0;
    int32 DisplayStep = INDEX_NONE;
    float InstrumentCountdown = 0.f;
    void UpdateInstruments();
    UCanvasRenderTarget2D* InstallDisplay(FName Name, FVector Center, FVector2D Size, FIntPoint Resolution);
    UFUNCTION() void DrawCluster(UCanvas* Canvas, int32 Width, int32 Height);
    UFUNCTION() void DrawConsole(UCanvas* Canvas, int32 Width, int32 Height);
    FRotator OriginalCamera;
    bool bPreviewCamera = false;
    bool bPaddlesDown = false;
    bool bAnglesChanged = false;
    UStaticMeshComponent* AddMesh(FName Name, UStaticMesh* Mesh, const FVector& Position);
    void AimGlass(int32 Index);
    void UpdateCapture(int32 Index, const FVector& Eye);
};
