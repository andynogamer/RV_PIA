#include "RVPracticePlayerController.h"
#include "RVDrivingAssistComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"

ARVPracticePlayerController::ARVPracticePlayerController()
{
    bShouldPerformFullTickWhenPaused = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
}

void ARVPracticePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ARVPracticePlayerController::ToggleReverse);
    InputComponent->BindKey(EKeys::Home, IE_Pressed, this, &ARVPracticePlayerController::ResetPractice).bExecuteWhenPaused = true;
    InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ARVPracticePlayerController::TogglePracticePause).bExecuteWhenPaused = true;
}

void ARVPracticePlayerController::ToggleReverse()
{
    if (APawn* ControlledPawn = GetPawn())
        if (auto* Assist = ControlledPawn->FindComponentByClass<URVDrivingAssistComponent>()) Assist->TryToggleReverse();
}

void ARVPracticePlayerController::ResetPractice()
{
    if (APawn* ControlledPawn = GetPawn())
        if (auto* Assist = ControlledPawn->FindComponentByClass<URVDrivingAssistComponent>()) Assist->ResetPractice();
}

void ARVPracticePlayerController::TogglePracticePause()
{
    if (APawn* ControlledPawn = GetPawn())
        if (auto* Assist = ControlledPawn->FindComponentByClass<URVDrivingAssistComponent>()) Assist->TogglePracticePause();
}
