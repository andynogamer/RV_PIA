#include "RVPracticePlayerController.h"
#include "RVDrivingAssistComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "RVSessionMenuWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "RVVehicleAudioComponent.h"

ARVPracticePlayerController::ARVPracticePlayerController()
{
    bShouldPerformFullTickWhenPaused = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
}

URVDrivingAssistComponent* ARVPracticePlayerController::GetAssist() const
{
    return GetPawn() ? GetPawn()->FindComponentByClass<URVDrivingAssistComponent>() : nullptr;
}

void ARVPracticePlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (!IsLocalController()) return;
    if (!bSessionInitialized && GetAssist()) { ShowStartMenu(); return; }
    if (SessionMenu && PendingMenuFocusFrames>0)
    {
        int32 Width=0,Height=0;GetViewportSize(Width,Height);
        // A newly created PIE viewport can initially report 2x2. Wait for its layout.
        if (Width>32 && Height>32 && --PendingMenuFocusFrames==0)
        {
            SessionMenu->FocusPrimaryButton();
            SetMouseLocation(Width/2,Height/2);
        }
    }
    if (auto* Assist=GetAssist())
    {
        if (SessionScreen==ERVSessionScreen::Driving && Assist->bPracticePaused) ShowPauseMenu();
        else if (SessionScreen==ERVSessionScreen::Paused && !Assist->bPracticePaused) CaptureDrivingInput();
    }
}

void ARVPracticePlayerController::OpenMenu(bool bPauseMenu)
{
    auto* Assist=GetAssist();
    if (!Assist) return;
    bSessionInitialized=true;
    if (!Assist->bPracticePaused) Assist->TogglePracticePause();
    if (SessionMenu) SessionMenu->RemoveFromParent();
    SessionMenu=CreateWidget<URVSessionMenuWidget>(this,URVSessionMenuWidget::StaticClass());
    if (!SessionMenu) return;
    SessionMenu->Configure(bPauseMenu,Assist->bUrbanMode);
    SessionMenu->AddToViewport(100);
    bShowMouseCursor=true;
    UWidgetBlueprintLibrary::SetFocusToGameViewport();
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(SessionMenu->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
    SetInputMode(Mode);
    PendingMenuFocusFrames=3;
}

void ARVPracticePlayerController::ShowStartMenu()
{
    if (!IsLocalController() || !GetAssist()) return;
    GetAssist()->ResetPractice();
    SessionScreen=ERVSessionScreen::Start;
    OpenMenu(false);
}

void ARVPracticePlayerController::CaptureDrivingInput()
{
    if (SessionMenu) { SessionMenu->RemoveFromParent(); SessionMenu=nullptr; }
    SessionScreen=ERVSessionScreen::Driving;
    PendingMenuFocusFrames=0;
    bShowMouseCursor=false;
    FInputModeGameOnly Mode;Mode.SetConsumeCaptureMouseDown(false);
    SetInputMode(Mode);
    if (auto* Viewport=GetWorld()->GetGameViewport())
    {
        Viewport->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
        Viewport->SetMouseLockMode(EMouseLockMode::LockAlways);
    }
    UWidgetBlueprintLibrary::SetFocusToGameViewport();
}

void ARVPracticePlayerController::StartSession()
{
    if (auto* Assist=GetAssist())
    {
        if (SessionScreen==ERVSessionScreen::Start) Assist->ResetPractice();
        if (Assist->bPracticePaused) Assist->TogglePracticePause();
        CaptureDrivingInput();
    }
}

void ARVPracticePlayerController::ShowPauseMenu()
{
    if (SessionScreen==ERVSessionScreen::Start) return;
    SessionScreen=ERVSessionScreen::Paused;
    OpenMenu(true);
}

void ARVPracticePlayerController::RestartSession()
{
    if (auto* Assist=GetAssist()) { Assist->ResetPractice(); CaptureDrivingInput(); }
}

void ARVPracticePlayerController::QuitSession()
{
    if (auto* Assist=GetAssist()) if (!Assist->bPracticePaused) Assist->TogglePracticePause();
    // QuitGame ends PIE when testing; it does not close the Unreal Editor.
    UKismetSystemLibrary::QuitGame(this,this,EQuitPreference::Quit,false);
}

void ARVPracticePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ARVPracticePlayerController::ToggleReverse);
    InputComponent->BindKey(EKeys::Home, IE_Pressed, this, &ARVPracticePlayerController::ResetPractice).bExecuteWhenPaused = true;
    InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ARVPracticePlayerController::TogglePracticePause).bExecuteWhenPaused = true;
    InputComponent->BindKey(EKeys::M, IE_Pressed, this, &ARVPracticePlayerController::TogglePracticePause).bExecuteWhenPaused = true;
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ARVPracticePlayerController::TogglePracticePause).bExecuteWhenPaused = true;
    InputComponent->BindKey(EKeys::H, IE_Pressed, this, &ARVPracticePlayerController::RequestHorn);
    InputComponent->BindKey(EKeys::V, IE_Pressed, this, &ARVPracticePlayerController::ToggleSound).bExecuteWhenPaused=true;
}
void ARVPracticePlayerController::ToggleSound()
{
    if (GetPawn()) if (auto* Audio=GetPawn()->FindComponentByClass<URVVehicleAudioComponent>()) Audio->ToggleMute();
}

void ARVPracticePlayerController::ToggleReverse()
{
    if (SessionScreen!=ERVSessionScreen::Driving) return;
    if (APawn* ControlledPawn = GetPawn())
        if (auto* Assist = ControlledPawn->FindComponentByClass<URVDrivingAssistComponent>()) Assist->TryToggleReverse();
}

void ARVPracticePlayerController::ResetPractice()
{
    if (SessionScreen==ERVSessionScreen::Start) return;
    if (SessionScreen==ERVSessionScreen::Paused) { RestartSession(); return; }
    if (APawn* ControlledPawn = GetPawn())
        if (auto* Assist = ControlledPawn->FindComponentByClass<URVDrivingAssistComponent>()) Assist->ResetPractice();
}

void ARVPracticePlayerController::TogglePracticePause()
{
    if (SessionScreen==ERVSessionScreen::Driving) ShowPauseMenu();
    else if (SessionScreen==ERVSessionScreen::Paused) StartSession();
}
void ARVPracticePlayerController::RequestHorn()
{
    if (SessionScreen!=ERVSessionScreen::Driving) return;
    if (APawn* ControlledPawn=GetPawn())
        if (auto* Assist=ControlledPawn->FindComponentByClass<URVDrivingAssistComponent>()) Assist->RequestHorn();
}
