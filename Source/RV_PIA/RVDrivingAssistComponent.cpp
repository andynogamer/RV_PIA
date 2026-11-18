#include "RVDrivingAssistComponent.h"
#include "RVCabinComponent.h"
#include "WheeledVehiclePawn.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "RVTrafficSystem.h"
#include "RVParkingScenario.h"
#include "EngineUtils.h"
#include "RVDrivingFeedbackComponent.h"
#include "RVVehicleAudioComponent.h"
#include "Engine/GameViewportClient.h"
#include "GenericPlatform/GenericWindow.h"
#include "Widgets/SWindow.h"

namespace
{
    constexpr float GearClutchThreshold = 0.8f;
    constexpr float ThrottleReleased = 0.05f;
}

URVDrivingAssistComponent::URVDrivingAssistComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void URVDrivingAssistComponent::BeginPlay()
{
    Super::BeginPlay();
    Vehicle = Cast<AWheeledVehiclePawn>(GetOwner());
    if (!Vehicle) { SetComponentTickEnabled(false); return; }
    Movement = Cast<UChaosWheeledVehicleMovementComponent>(Vehicle->GetVehicleMovementComponent());
    if (!Movement) { SetComponentTickEnabled(false); return; }
    InitialTransform = Vehicle->GetActorTransform();
    Movement->AddTickPrerequisiteComponent(this);
    WheelInput = MakeUnique<FRVWheelInput>();
    auto& Transmission = Movement->TransmissionSetup;
    if (Transmission.ForwardGearRatios.Num() < 6)
    {
        // Keep the template ratios; extend a shorter gearbox with prototype overdrives.
        while (Transmission.ForwardGearRatios.Num() < 6)
        {
            const float Previous = Transmission.ForwardGearRatios.IsEmpty() ? 3.5f : Transmission.ForwardGearRatios.Last();
            Transmission.ForwardGearRatios.Add(Previous * 0.8f);
        }
        Movement->ResetVehicle();
    }
    // The stock template turns the brake into reverse. Training uses a separate selector.
    Movement->bReverseAsBrake = false;
    Movement->bThrottleAsBrake = false;
    Movement->SetUseAutomaticGears(true);
    Movement->SetTargetGear(1, true);
    for (TActorIterator<ARVUrbanScenario> It(GetWorld()); It; ++It) { UrbanScenario=*It; bUrbanMode=true; break; }
    Parking = Cast<ARVParkingScenario>(UrbanScenario);
    if (Parking) Parking->ResetParking();
    if (bUrbanMode) Movement->SetHandbrakeInput(true);
    if (!GetOwner()->FindComponentByClass<URVDrivingFeedbackComponent>())
    {
        auto* Feedback=NewObject<URVDrivingFeedbackComponent>(GetOwner(),TEXT("DrivingFeedback"));
        GetOwner()->AddInstanceComponent(Feedback);Feedback->RegisterComponent();
    }
    if (!GetOwner()->FindComponentByClass<URVVehicleAudioComponent>())
    {
        auto* Audio=NewObject<URVVehicleAudioComponent>(GetOwner(),TEXT("DrivingAudio"));
        GetOwner()->AddInstanceComponent(Audio);Audio->RegisterComponent();
    }
    Cabin = NewObject<URVCabinComponent>(Vehicle,TEXT("CabinaConductor"));
    Vehicle->AddInstanceComponent(Cabin);
    Cabin->RegisterComponent();
    TArray<UStaticMeshComponent*> Components;
    Vehicle->GetComponents(Components);
    for (UStaticMeshComponent* Component : Components)
    {
        if (Component->GetName().StartsWith(TEXT("VolanteVisual")))
        {
            SteeringWheel = Component;
            InitialWheelRotation = Component->GetRelativeRotation().Quaternion();
        }
    }
}

float URVDrivingAssistComponent::GetSpeedKmh() const
{
    return Movement ? FMath::Abs(Movement->GetForwardSpeed()) * 0.036f : 0.f;
}

void URVDrivingAssistComponent::StopInputs()
{
    if (!Movement) return;
    Movement->SetThrottleInput(0.f);
    Movement->SetSteeringInput(0.f);
    Movement->SetBrakeInput(1.f);
}

void URVDrivingAssistComponent::SetStatus(const FString& Message)
{
    Status = Message;
    StatusSeconds = 3.f;
}

bool URVDrivingAssistComponent::TryToggleReverse()
{
    if (!Movement || bPracticePaused || bWheelMode) return false;
    if (GetSpeedKmh() > GearChangeLimitKmh)
    {
        SetStatus(TEXT("Deten el vehiculo antes de cambiar de marcha."));
        return false;
    }
    if (Movement->GetThrottleInput() > 0.05f)
    {
        SetStatus(TEXT("Suelta el acelerador antes de cambiar de marcha."));
        return false;
    }
    bReverseMode = !bReverseMode;
    Movement->SetTargetGear(bReverseMode ? -1 : 1, true);
    StopInputs();
    Movement->SetBrakeInput(0.f);
    SetStatus(bReverseMode ? TEXT("Reversa seleccionada. W acelera; S frena.") : TEXT("Avance seleccionado. W acelera; S frena."));
    return true;
}

void* URVDrivingAssistComponent::GetWheelWindow() const
{
    if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
    {
        TSharedPtr<SWindow> Window = Viewport->GetWindow();
        if (Window && Window->GetNativeWindow()) return Window->GetNativeWindow()->GetOSWindowHandle();
    }
    return nullptr;
}

void URVDrivingAssistComponent::SubmitWheelSample(float Steering, float Throttle, float Brake, float Clutch, int32 Gear)
{
    if (bPracticePaused || !FMath::IsFinite(Steering) || !FMath::IsFinite(Throttle) ||
        !FMath::IsFinite(Brake) || !FMath::IsFinite(Clutch) || Gear < -1 || Gear > 6) return;
    PendingWheelSample = FRVWheelSample{ FMath::Clamp(Steering, -1.f, 1.f), FMath::Clamp(Throttle, 0.f, 1.f),
        FMath::Clamp(Brake, 0.f, 1.f), FMath::Clamp(Clutch, 0.f, 1.f), Gear };
}

void URVDrivingAssistComponent::DisarmWheel()
{
    bHasPhysicalWheelSample = false;
    bRequireWheelNeutral = true;
    LastWheelSample = {};
    bShiftRejected = false;
    LastShifterGear = SelectedGear = 0;
    bWaitingForClutch = false;
    ClutchWindowSeconds = 0.f;
    ShifterStableSeconds = 0.f;
    bReverseMode = false;
    PendingWheelSample.Reset();
    if (bWheelMode) Movement->SetTargetGear(0, true);
}

void URVDrivingAssistComponent::SetWheelMode(bool bEnabled)
{
    StopInputs();
    bWheelMode = bEnabled;
    Movement->SetUseAutomaticGears(!bEnabled);
    DisarmWheel();
    if (!bEnabled)
    {
        Movement->SetTargetGear(1, true);
        bRequireThrottleRelease = true;
    }
}

void URVDrivingAssistComponent::ToggleWheelMode()
{
    if (!Movement || !WheelInput || bPracticePaused) return;
    if (GetSpeedKmh() > GearChangeLimitKmh)
    {
        SetStatus(TEXT("Detén el vehículo para cambiar de controles."));
        return;
    }
    WheelInput->CancelCalibration();
    bKeyboardOnly = bWheelMode;
    SetWheelMode(!bWheelMode);
}

void URVDrivingAssistComponent::ToggleWheelCalibration()
{
    if (!Movement || !WheelInput || bPracticePaused) return;
    if (WheelInput->IsCalibrating())
    {
        WheelInput->CancelCalibration();
        bKeyboardOnly = true;
        SetWheelMode(false);
        return;
    }
    if (GetSpeedKmh() > GearChangeLimitKmh)
    {
        SetStatus(TEXT("Detén el vehículo para calibrar el G920."));
        return;
    }
    SetWheelMode(true);
    WheelInput->StartCalibration();
}

void URVDrivingAssistComponent::CaptureWheelCalibrationStep()
{
    if (bPracticePaused || !WheelInput || !WheelInput->IsCalibrating()) return;
    WheelInput->CaptureStep(GetWheelWindow());
    if (!WheelInput->IsCalibrating()) bKeyboardOnly = false;
}

bool URVDrivingAssistComponent::ApplyWheelInput(float DeltaTime, float& RequestedThrottle)
{
    if (WheelInput->IsCalibrating())
    {
        StopInputs();
        return false;
    }

    FRVWheelSample Sample;
    const bool bHadSample = PendingWheelSample.IsSet();
    if (bHadSample)
    {
        Sample = PendingWheelSample.GetValue();
        PendingWheelSample.Reset();
    }
    const bool bRead = bHadSample || (!bKeyboardOnly && WheelInput->Read(GetWheelWindow(), Sample));
    if (!bWheelMode && (bRead || (!bKeyboardOnly && WheelInput->IsAutomaticWheelPresent())) && GetSpeedKmh() <= GearChangeLimitKmh)
        SetWheelMode(true);
    if (!bWheelMode) return true;
    if (!bRead)
    {
        if (!bRequireWheelNeutral)
            if (auto* Feedback = GetOwner()->FindComponentByClass<URVDrivingFeedbackComponent>()) Feedback->SuspendFeedback();
        DisarmWheel();
        StopInputs();
        return false;
    }
    // Keep the raw gate for diagnostics. Both lower-right detents select reverse.
    LastWheelSample = Sample;
    if (Sample.Gear == 6) Sample.Gear = -1;
    bHasPhysicalWheelSample = !bHadSample;
    if (bRequireWheelNeutral)
    {
        const bool IsolatedAtRest = Sample.Clutch >= GearClutchThreshold && GetSpeedKmh() <= GearChangeLimitKmh;
        if ((Sample.Gear != 0 && !IsolatedAtRest) || Sample.Throttle > ThrottleReleased)
        {
            StopInputs();
            return false;
        }
        bRequireWheelNeutral = false;
    }
    RequestedThrottle = Sample.Throttle;

    // No torque while leaving a gate. Only a stable detent can select a gear.
    if (Sample.Gear != LastShifterGear)
    {
        LastShifterGear = Sample.Gear;
        ShifterStableSeconds = 0.f;
    }
    else ShifterStableSeconds += DeltaTime;

    const float SignedSpeedKmh = Movement->GetForwardSpeed() * 0.036f;
    const auto CanEngageDirection = [&](int32 Gear)
    {
        if (Gear < 0 && SelectedGear != -1 &&
            Vehicle->GetMesh()->GetPhysicsLinearVelocity().Size2D() * 0.036f > GearChangeLimitKmh) return false;
        return Gear == 0 || SignedSpeedKmh * FMath::Sign(Gear) >= -GearChangeLimitKmh;
    };
    // The clutch can precede the lever, overlap the detent or arrive just afterwards.
    ClutchWindowSeconds = Sample.Clutch >= GearClutchThreshold ? .16f : FMath::Max(0.f, ClutchWindowSeconds-DeltaTime);
    if (Sample.Gear == 0)
    {
        SelectedGear = 0;
        bShiftRejected = bWaitingForClutch = false;
    }
    else if (!bShiftRejected && Sample.Gear != SelectedGear && ShifterStableSeconds >= .05f)
    {
        if (!CanEngageDirection(Sample.Gear))
        {
            bShiftRejected = true;
            bWaitingForClutch = false;
            SelectedGear = 0;
            SetStatus(TEXT("Detén el vehículo y vuelve a N antes de cambiar de sentido."));
        }
        else if (ClutchWindowSeconds > 0.f)
        {
            SelectedGear = Sample.Gear;
            bWaitingForClutch = false;
        }
        else
        {
            bWaitingForClutch = true;
            SelectedGear = 0;
        }
    }

    if (!CanEngageDirection(SelectedGear))
    {
        bShiftRejected = true;
        SelectedGear = 0;
        SetStatus(TEXT("Detén el vehículo y vuelve a N antes de cambiar de sentido."));
    }
    const bool bCoupled = Sample.Gear == SelectedGear && Sample.Clutch < GearClutchThreshold;
    const int32 PhysicalGear = bCoupled ? SelectedGear : 0;
    Movement->SetTargetGear(PhysicalGear, true);
    bReverseMode = SelectedGear < 0;
    Movement->SetSteeringInput(Sample.Steering);
    Movement->SetBrakeInput(Sample.Brake);
    const float Engagement = 1.f - FMath::Clamp((Sample.Clutch - 0.1f) / (GearClutchThreshold - 0.1f), 0.f, 1.f);
    const bool bMatchingGear = Movement->GetCurrentGear() == PhysicalGear;
    Movement->SetThrottleInput(bMatchingGear && !bWaitingForClutch && !bShiftRejected && !bRequireThrottleRelease ?
        Sample.Throttle * (PhysicalGear == 0 ? 1.f : Engagement) : 0.f);
    return true;
}

FString URVDrivingAssistComponent::GetPhysicalShifterLabel() const
{
    if (!bHasPhysicalWheelSample || bPracticePaused || GetCalibrationStep() != INDEX_NONE) return {};
    const int32 Gear = LastWheelSample.Gear;
    return Gear == 6 ? TEXT("6 → R") : Gear < 0 ? TEXT("R") : Gear == 0 ? TEXT("N") : FString::FromInt(Gear);
}

FString URVDrivingAssistComponent::GetGearLabel() const
{
    if (bReverseMode) return TEXT("R");
    if (!bWheelMode) return TEXT("D");
    return SelectedGear == 0 ? TEXT("N") : FString::FromInt(SelectedGear);
}

void URVDrivingAssistComponent::ResetPractice()
{
    if (!Vehicle || !Movement) return;
    const bool bThrottleWasHeld = Movement->GetThrottleInput() > 0.05f;
    const bool bWasPaused = bPracticePaused;
    if (bPracticePaused) TogglePracticePause();
    StopInputs();
    Vehicle->SetActorTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
    Vehicle->GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Vehicle->GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    // Recreate Chaos' asynchronous state so it cannot restore the pre-teleport pose.
    Movement->ResetVehicle();
    Movement->SetHandbrakeInput(bUrbanMode);
    DisarmWheel();
    Movement->SetUseAutomaticGears(!bWheelMode);
    Movement->SetTargetGear(bWheelMode ? 0 : 1, true);
    Movement->SetBrakeInput(0.f);
    bRequireThrottleRelease = bRequireThrottleRelease || bThrottleWasHeld || bWasPaused;
    bReverseMode = false;
    PracticeStage = 0;
    UrbanCheckpointIndex = 0;
    if (UrbanScenario) UrbanScenario->ResetSessionProgress();
    if (Parking) Parking->ResetParking();
    if (auto* Feedback=GetOwner()->FindComponentByClass<URVDrivingFeedbackComponent>()) Feedback->ResetSession();
    if (auto* Audio=GetOwner()->FindComponentByClass<URVVehicleAudioComponent>()) Audio->SilenceTransientSounds();
    SetStatus(TEXT("Practica reiniciada. Avanza con suavidad."));
}

void URVDrivingAssistComponent::TogglePracticePause()
{
    if (!Vehicle || !Movement) return;
    StopInputs();
    if (bWheelMode) DisarmWheel();
    Vehicle->GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Vehicle->GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    bPracticePaused = !bPracticePaused;
    UGameplayStatics::SetGamePaused(GetWorld(), bPracticePaused);
    if (auto* Feedback=GetOwner()->FindComponentByClass<URVDrivingFeedbackComponent>()) Feedback->SuspendFeedback();
    if (bPracticePaused) if (auto* Audio=GetOwner()->FindComponentByClass<URVVehicleAudioComponent>()) Audio->SilenceTransientSounds();
    if (!bPracticePaused)
    {
        Movement->SetBrakeInput(0.f);
        bRequireThrottleRelease = true;
    }
}

float URVDrivingAssistComponent::GetPostedSpeedKmh() const
{
    return UrbanScenario ? UrbanScenario->PostedSpeedKmh : ForwardLimitKmh;
}

float URVDrivingAssistComponent::GetDistanceToGoalMeters() const
{
    if (Parking && Vehicle) return Parking->DistanceMeters(Vehicle->GetActorLocation());
    if (bUrbanMode && UrbanScenario && Vehicle)
    {
        if (PracticeStage>=3 || !UrbanScenario->Checkpoints.IsValidIndex(UrbanCheckpointIndex)) return 0;
        return FVector::Dist2D(Vehicle->GetActorLocation(),UrbanScenario->Checkpoints[UrbanCheckpointIndex])*.01f;
    }
    if (!Vehicle || PracticeStage >= 2) return 0.f;
    const float ForwardCm = PracticeStage == 0 ? 1200.f : 4000.f;
    const FVector Target = InitialTransform.GetLocation() + InitialTransform.GetRotation().GetForwardVector() * ForwardCm;
    return FVector::Dist2D(Vehicle->GetActorLocation(), Target) * 0.01f;
}

FString URVDrivingAssistComponent::GetInstruction() const
{
    if (bPracticePaused) return TEXT("PAUSA - P para continuar. Home para reiniciar.");
    if (WheelInput && WheelInput->IsCalibrating()) return WheelInput->GetPrompt();
    if (bWheelMode && NeedsWheelProfile() && HasWheelReading())
        return TEXT("Pulsa J para calibrar el G920 o K para usar el teclado.");
    if (bWheelMode && bRequireWheelNeutral)
    {
        const FString Position = GetPhysicalShifterLabel();
        if (!Position.IsEmpty())
            return FString::Printf(TEXT("Palanca en %s. Suelta el acelerador y pisa el embrague para empezar."), *Position);
        return TEXT("Esperando señal del G920. Vuelve a N con el juego en primer plano.");
    }
    if (bRequireThrottleRelease) return TEXT("Suelta el acelerador para continuar con seguridad.");
    if (bWaitingForClutch) return TEXT("Mantén la marcha y pisa el embrague para engranarla.");
    if (bShiftRejected) return TEXT("Detente y vuelve a N para cambiar de sentido.");
    if (StatusSeconds > 0.f) return Status;
    if (Parking && PracticeStage>0) return Parking->Instruction();
    if (bUrbanMode)
    {
        if (PracticeStage==0) return TEXT("Estacionado: pulsa y suelta Espacio para liberar el freno de mano.");
        if (PracticeStage==1) return TEXT("Observa el trafico e incorporate con suavidad al carril.");
        if (PracticeStage==2)
        {
            if (UrbanScenario && UrbanCheckpointIndex==UrbanScenario->Checkpoints.Num()-1 && GetDistanceToGoalMeters()<15)
                return TEXT("Estaciónate en la bahía señalizada y detente para completar el recorrido.");
            return TEXT("Continúa al siguiente objetivo y respeta la señalización.");
        }
        return TEXT("Recorrido completado. Puedes explorar o reiniciar con Home.");
    }
    if (PracticeStage == 0) return TEXT("Avanza despacio entre los conos. Limite: 30 km/h.");
    if (PracticeStage == 1) return TEXT("Frena y detente dentro de la zona final.");
    return TEXT("Practica completada. Home para repetir.");
}

FString URVDrivingAssistComponent::GetUrbanSummary() const
{
    if (Parking) return FString::Printf(TEXT("Estacionamiento   %d / 2"),Parking->CompletedBays);
    if (!UrbanScenario) return FString();
    FString Direction;
    if (Vehicle && PracticeStage>0 && PracticeStage<3 && UrbanScenario->Checkpoints.IsValidIndex(UrbanCheckpointIndex))
    {
        const FVector ToGoal=(UrbanScenario->Checkpoints[UrbanCheckpointIndex]-Vehicle->GetActorLocation()).GetSafeNormal2D();
        const float Ahead=FVector::DotProduct(Vehicle->GetActorForwardVector(),ToGoal);
        const float Side=FVector::DotProduct(Vehicle->GetActorRightVector(),ToGoal);
        Direction=Ahead<-.3f ? TEXT(" | Objetivo detras") : FMath::Abs(Side)>.35f ?
            (Side>0 ? TEXT(" | Hacia derecha") : TEXT(" | Hacia izquierda")) : TEXT(" | Sigue recto");
    }
    return FString::Printf(TEXT("Ruta %d/%d | Avisos en rojo: %d"),FMath::Min(UrbanCheckpointIndex+1,UrbanScenario->Checkpoints.Num()),
        UrbanScenario->Checkpoints.Num(),UrbanScenario->RedLightWarnings)+Direction;
}
void URVDrivingAssistComponent::RequestHorn()
{
    if (UrbanScenario && !bPracticePaused) UrbanScenario->RequestPlayerHorn(GetOwner());
    else if (!bPracticePaused) if (auto* Audio=GetOwner()->FindComponentByClass<URVVehicleAudioComponent>()) Audio->PlayHorn(GetOwner(),TEXT("manual"));
}

void URVDrivingAssistComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!Movement || !Vehicle) return;
    APlayerController* PC = Cast<APlayerController>(Vehicle->GetController());
    if (!PC || PC->GetPawn() != Vehicle)
    {
        if (bWheelMode) DisarmWheel();
        StopInputs(); bHadController = false; bRequireThrottleRelease = true; return;
    }
    if (!bHadController) { Movement->SetBrakeInput(0.f); bHadController = true; }
    // Input first, training rules second, movement third. Movement already ticks before its Pawn.
    if (!bInputTickLinked) { AddTickPrerequisiteActor(PC); bInputTickLinked = true; }
    if (bPracticePaused) { StopInputs(); return; }
    float RequestedThrottle = Movement->GetThrottleInput();
    const bool bCanDrive = ApplyWheelInput(DeltaTime, RequestedThrottle);
    if (SteeringWheel)
    {
        const float Steering = bWheelMode ? LastWheelSample.Steering : Movement->GetSteeringInput();
        // Rotate about the vehicle's column. From the seat the top moves right on positive steer.
        SteeringWheel->SetRelativeRotation(FQuat(FVector::ForwardVector,FMath::DegreesToRadians(-450.f*Steering))*InitialWheelRotation);
    }
    if (Cabin && Cabin->GetSelectedMirror()!=INDEX_NONE)
    {
        bRequireThrottleRelease = true;
        StopInputs();
        return;
    }
    if (!bCanDrive) return;
    if (bRequireThrottleRelease)
    {
        if (RequestedThrottle > ThrottleReleased) { StopInputs(); return; }
        bRequireThrottleRelease = false;
        if (!bWheelMode) Movement->SetBrakeInput(0.f);
    }
    StatusSeconds = FMath::Max(0.f, StatusSeconds - DeltaTime);
    if (bUrbanMode && PracticeStage==0)
    {
        if (Movement->GetHandbrakeInput()) { Movement->SetThrottleInput(0.f); return; }
        PracticeStage=1;
        if (RequestedThrottle > ThrottleReleased) { bRequireThrottleRelease=true; StopInputs(); return; }
    }
    if (Movement->GetBrakeInput() > 0.05f) Movement->SetThrottleInput(0.f);
    const float Limit = bReverseMode ? ReverseLimitKmh : ForwardLimitKmh;
    if ((bReverseMode || !bUrbanMode) && GetSpeedKmh() >= Limit)
    {
        Movement->SetThrottleInput(0.f);
    }
    const FVector Offset = Vehicle->GetActorLocation() - InitialTransform.GetLocation();
    const float Along = FVector::DotProduct(Offset, InitialTransform.GetRotation().GetForwardVector());
    const float Across = FVector::DotProduct(Offset, InitialTransform.GetRotation().GetRightVector());
    const bool Outside=bUrbanMode && UrbanScenario ? !UrbanScenario->IsInsideBounds(Vehicle->GetActorLocation()) :
        FMath::Abs(Across)>CorridorHalfWidthCm || Along< -900.f || Along>CorridorLengthCm || Offset.Z< -200.f;
    if (Outside)
    {
        ResetPractice();
        SetStatus(TEXT("Fuera del area de practica. Reinicio seguro."));
        return;
    }
    if (Parking)
    {
        Parking->UpdateParking(Vehicle->GetActorTransform(),Vehicle->GetVelocity().Size()*.036f,bReverseMode,Movement->GetHandbrakeInput(),DeltaTime);
        PracticeStage = Parking->Complete() ? 3 : Parking->CompletedBays+1;
    }
    else if (bUrbanMode && UrbanScenario)
    {
        if (PracticeStage>0 && PracticeStage<3 && GetDistanceToGoalMeters()<4.f)
        {
            if (UrbanCheckpointIndex<UrbanScenario->Checkpoints.Num()-1) { ++UrbanCheckpointIndex; PracticeStage=2; }
            else if (GetSpeedKmh()<.5f) PracticeStage=3;
        }
    }
    else
    {
        if (PracticeStage == 0 && GetDistanceToGoalMeters() < 2.5f) PracticeStage = 1;
        if (PracticeStage == 1 && GetDistanceToGoalMeters() < 3.f && GetSpeedKmh() < 0.5f) PracticeStage = 2;
    }
}

void URVDrivingAssistComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopInputs();
    WheelInput.Reset();
    Super::EndPlay(EndPlayReason);
}
