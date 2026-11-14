#include "RVDrivingAssistComponent.h"
#include "WheeledVehiclePawn.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

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
    // The stock template turns the brake into reverse. Training uses a separate selector.
    Movement->bReverseAsBrake = false;
    Movement->bThrottleAsBrake = false;
    Movement->SetUseAutomaticGears(true);
    Movement->SetTargetGear(1, true);
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
    if (!Movement || bPracticePaused) return false;
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
    Movement->SetHandbrakeInput(false);
    Movement->SetTargetGear(1, true);
    Movement->SetBrakeInput(0.f);
    bRequireThrottleRelease = bThrottleWasHeld || bWasPaused;
    bReverseMode = false;
    PracticeStage = 0;
    SetStatus(TEXT("Practica reiniciada. Avanza con suavidad."));
}

void URVDrivingAssistComponent::TogglePracticePause()
{
    if (!Vehicle || !Movement) return;
    StopInputs();
    Vehicle->GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Vehicle->GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    bPracticePaused = !bPracticePaused;
    UGameplayStatics::SetGamePaused(GetWorld(), bPracticePaused);
    if (!bPracticePaused)
    {
        Movement->SetBrakeInput(0.f);
        bRequireThrottleRelease = true;
    }
}

float URVDrivingAssistComponent::GetDistanceToGoalMeters() const
{
    if (!Vehicle || PracticeStage >= 2) return 0.f;
    const float ForwardCm = PracticeStage == 0 ? 1200.f : 4000.f;
    const FVector Target = InitialTransform.GetLocation() + InitialTransform.GetRotation().GetForwardVector() * ForwardCm;
    return FVector::Dist2D(Vehicle->GetActorLocation(), Target) * 0.01f;
}

FString URVDrivingAssistComponent::GetInstruction() const
{
    if (bPracticePaused) return TEXT("PAUSA - P para continuar. Home para reiniciar.");
    if (StatusSeconds > 0.f) return Status;
    if (PracticeStage == 0) return TEXT("Avanza despacio entre los conos. Limite: 30 km/h.");
    if (PracticeStage == 1) return TEXT("Frena y detente dentro de la zona final.");
    return TEXT("Practica completada. Home para repetir.");
}

void URVDrivingAssistComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!Movement || !Vehicle) return;
    APlayerController* PC = Cast<APlayerController>(Vehicle->GetController());
    if (!PC || PC->GetPawn() != Vehicle)
    {
        StopInputs(); bHadController = false; bRequireThrottleRelease = true; return;
    }
    if (!bHadController) { Movement->SetBrakeInput(0.f); bHadController = true; }
    // Input first, training rules second, movement third. Movement already ticks before its Pawn.
    if (!bInputTickLinked) { AddTickPrerequisiteActor(PC); bInputTickLinked = true; }
    if (bPracticePaused) { StopInputs(); return; }
    if (bRequireThrottleRelease)
    {
        if (Movement->GetThrottleInput() > 0.05f) { StopInputs(); return; }
        bRequireThrottleRelease = false;
        Movement->SetBrakeInput(0.f);
    }
    StatusSeconds = FMath::Max(0.f, StatusSeconds - DeltaTime);
    if (Movement->GetBrakeInput() > 0.05f) Movement->SetThrottleInput(0.f);
    const float Limit = bReverseMode ? ReverseLimitKmh : ForwardLimitKmh;
    if (GetSpeedKmh() >= Limit)
    {
        Movement->SetThrottleInput(0.f);
    }
    const FVector Offset = Vehicle->GetActorLocation() - InitialTransform.GetLocation();
    const float Along = FVector::DotProduct(Offset, InitialTransform.GetRotation().GetForwardVector());
    const float Across = FVector::DotProduct(Offset, InitialTransform.GetRotation().GetRightVector());
    if (FMath::Abs(Across) > CorridorHalfWidthCm || Along < -900.f || Along > CorridorLengthCm || Offset.Z < -200.f)
    {
        ResetPractice();
        SetStatus(TEXT("Fuera del area de practica. Reinicio seguro."));
        return;
    }
    if (PracticeStage == 0 && GetDistanceToGoalMeters() < 2.5f) PracticeStage = 1;
    if (PracticeStage == 1 && GetDistanceToGoalMeters() < 3.f && GetSpeedKmh() < 0.5f) PracticeStage = 2;
    if (SteeringWheel)
    {
        SteeringWheel->SetRelativeRotation(InitialWheelRotation * FQuat(FVector::RightVector,
            FMath::DegreesToRadians(-Movement->GetSteeringInput() * 450.f)));
    }
}

void URVDrivingAssistComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopInputs();
    Super::EndPlay(EndPlayReason);
}
