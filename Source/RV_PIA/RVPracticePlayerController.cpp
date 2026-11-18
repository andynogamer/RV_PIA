#include "RVPracticePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "RVDrivingAssistComponent.h"
#include "RVCabinComponent.h"
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
        if (SessionScreen==ERVSessionScreen::Driving && bAutoHideGuide && Assist->GetSpeedKmh()>=4.f)
        {
            bAutoHideGuide=false;
            bShowDrivingGuide=false;
        }
        if (SessionScreen==ERVSessionScreen::Driving && Assist->bPracticePaused) ShowPauseMenu();
        else if (SessionScreen==ERVSessionScreen::Paused && !Assist->bPracticePaused) CaptureDrivingInput();
    }
}

void ARVPracticePlayerController::OpenMenu(bool bPauseMenu)
{
    auto* Assist=GetAssist();
    if (!Assist) return;
    if (auto* Cabin=GetPawn()->FindComponentByClass<URVCabinComponent>()) Cabin->FinishAdjustment();
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

bool ARVPracticePlayerController::IsHighway() const
{
    return UGameplayStatics::GetCurrentLevelName(this,true) == TEXT("L_Carretera");
}

bool ARVPracticePlayerController::IsParking() const
{
    return UGameplayStatics::GetCurrentLevelName(this,true) == TEXT("L_Estacionamiento");
}

void ARVPracticePlayerController::SwitchParking()
{
    if (SessionScreen != ERVSessionScreen::Start || !IsLocalController()) return;
    UGameplayStatics::OpenLevel(this, IsParking() ? TEXT("/Game/RutaVirtual/Maps/L_CiudadUrbana") : TEXT("/Game/RutaVirtual/Maps/L_Estacionamiento"));
}

void ARVPracticePlayerController::SwitchRoad()
{
    if (SessionScreen != ERVSessionScreen::Start || !IsLocalController()) return;
    const FName Map(IsHighway() ? TEXT("/Game/RutaVirtual/Maps/L_CiudadUrbana") : TEXT("/Game/RutaVirtual/Maps/L_Carretera"));
    UGameplayStatics::OpenLevel(this,Map);
}

void ARVPracticePlayerController::ShowStartMenu()
{
    if (!IsLocalController() || !GetAssist()) return;
    bShowDrivingGuide=bAutoHideGuide=true;
    GetAssist()->ResetPractice();
    SessionScreen=ERVSessionScreen::Start;
    OpenMenu(false);
}

void ARVPracticePlayerController::CaptureDrivingInput()
{
    if (SessionMenu) { SessionMenu->RemoveFromParent(); SessionMenu=nullptr; }
    // The template overlay displays the temporary Chaos clutch neutral. The cabin instruments show the gear engaged by the driver.
    if (UClass* TemplateDisplay=LoadClass<UUserWidget>(nullptr,TEXT("/Game/VehicleTemplate/UI/UI_Vehicle.UI_Vehicle_C")))
    {
        TArray<UUserWidget*> Displays;
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this,Displays,TemplateDisplay);
        for (auto* Display : Displays)
            if (Display->GetOwningPlayer()==this) Display->SetVisibility(ESlateVisibility::Collapsed);
    }
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
    InputComponent->BindKey(EKeys::B, IE_Pressed, this, &ARVPracticePlayerController::CycleMirror);
    InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ARVPracticePlayerController::ToggleGuide);
    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ARVPracticePlayerController::ToggleReverse);
    InputComponent->BindKey(EKeys::K, IE_Pressed, this, &ARVPracticePlayerController::ToggleWheelMode);
    InputComponent->BindKey(EKeys::J, IE_Pressed, this, &ARVPracticePlayerController::ToggleWheelCalibration);
    InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ARVPracticePlayerController::CaptureWheelCalibrationStep);
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

void ARVPracticePlayerController::ToggleWheelCalibration()
{
    if (SessionScreen != ERVSessionScreen::Driving) return;
    if (auto* Assist = GetAssist()) Assist->ToggleWheelCalibration();
}

void ARVPracticePlayerController::CaptureWheelCalibrationStep()
{
    if (SessionScreen != ERVSessionScreen::Driving) return;
    if (auto* Cabin=GetPawn()->FindComponentByClass<URVCabinComponent>(); Cabin && Cabin->GetSelectedMirror()!=INDEX_NONE)
        Cabin->FinishAdjustment();
    else if (auto* Assist = GetAssist()) Assist->CaptureWheelCalibrationStep();
}

void ARVPracticePlayerController::ToggleWheelMode()
{
    if (SessionScreen != ERVSessionScreen::Driving) return;
    if (auto* Assist = GetAssist()) Assist->ToggleWheelMode();
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

void ARVPracticePlayerController::CycleMirror()
{
    if (SessionScreen==ERVSessionScreen::Driving && GetPawn())
        if (auto* Cabin=GetPawn()->FindComponentByClass<URVCabinComponent>()) Cabin->CycleMirror();
}

void ARVPracticePlayerController::ToggleGuide()
{
    if (SessionScreen==ERVSessionScreen::Driving) { bAutoHideGuide=false; bShowDrivingGuide=!bShowDrivingGuide; }
}
