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
#include "Components/StereoLayerComponent.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "InputKeyEventArgs.h"
#include "Slate/WidgetRenderer.h"

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
    // VR Preview can enable stereo after the first player tick.
    if (SessionMenu && UsesStereoViewport() != (MenuLayer != nullptr))
        OpenMenu(SessionScreen==ERVSessionScreen::Paused);
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
        HandleWheelButtons(Assist->ReadWheelButtons());
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
    CloseMenu();
    SessionMenu=CreateWidget<URVSessionMenuWidget>(this,URVSessionMenuWidget::StaticClass());
    if (!SessionMenu) return;
    const bool bStereo = UsesStereoViewport();
    SessionMenu->Configure(bPauseMenu,Assist->bUrbanMode,bStereo);
    if (bStereo)
    {
        MenuLayer = NewObject<UStereoLayerComponent>(this);
        // Selection changes reuse the same texture. OpenXR's static image can only be acquired once.
        MenuLayer->bLiveTexture = true;
        MenuLayer->SetTickableWhenPaused(true);
        // The component defaults to a face-locked quad. The full square texture is shared by both eyes.
        MenuLayer->SetRelativeLocation(FVector(130,0,0));
        MenuLayer->SetQuadSize(FVector2D(100,100));
        MenuLayer->SetPriority(100);
        MenuLayer->RegisterComponent();
        StereoMenuContent = SessionMenu->TakeWidget();
        DrawStereoMenu();
        bShowMouseCursor = false;
        SetInputMode(FInputModeGameOnly());
        UWidgetBlueprintLibrary::SetFocusToGameViewport();
        return;
    }
    SessionMenu->AddToViewport(100);
    bShowMouseCursor=true;
    UWidgetBlueprintLibrary::SetFocusToGameViewport();
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(SessionMenu->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
    SetInputMode(Mode);
    PendingMenuFocusFrames=3;
}

bool ARVPracticePlayerController::UsesStereoViewport() const
{
    const auto* Client = GetWorld()->GetGameViewport();
    return Client && Client->Viewport && GEngine && GEngine->IsStereoscopic3D(Client->Viewport);
}

void ARVPracticePlayerController::HandleWheelButtons(TOptional<ERVWheelButton> Reading)
{
    // Baseline on connection and focus changes. A held button cannot carry through a menu transition.
    const ERVWheelButton Pressed = Reading.IsSet() && PreviousWheelButtons.IsSet() ?
        Reading.GetValue() & ~PreviousWheelButtons.GetValue() : ERVWheelButton::None;
    PreviousWheelButtons = Reading;
    if (Pressed==ERVWheelButton::None) return;
    if (EnumHasAnyFlags(Pressed, ERVWheelButton::Center)) { CenterSeat(); return; }
    if (SessionMenu)
    {
        if (EnumHasAnyFlags(Pressed, ERVWheelButton::Back | ERVWheelButton::Menu))
        {
            if (SessionMenu->IsShowingControls()) SessionMenu->ToggleControls();
            else if (SessionScreen==ERVSessionScreen::Paused) StartSession();
        }
        else if (EnumHasAnyFlags(Pressed, ERVWheelButton::Up)) SessionMenu->SelectMenuButton(-1);
        else if (EnumHasAnyFlags(Pressed, ERVWheelButton::Down)) SessionMenu->SelectMenuButton(1);
        else if (EnumHasAnyFlags(Pressed, ERVWheelButton::Accept)) SessionMenu->ActivateMenuButton();
        if (MenuLayer) DrawStereoMenu();
    }
    else if (SessionScreen==ERVSessionScreen::Driving)
    {
        if (EnumHasAnyFlags(Pressed, ERVWheelButton::Menu)) ShowPauseMenu();
        else if (EnumHasAnyFlags(Pressed, ERVWheelButton::Handbrake)) GetAssist()->ToggleParkingBrake();
        else if (EnumHasAnyFlags(Pressed, ERVWheelButton::Accept)) CaptureWheelCalibrationStep();
    }
}

bool ARVPracticePlayerController::TryCenterSeat()
{
    auto* Cabin = GetPawn() ? GetPawn()->FindComponentByClass<URVCabinComponent>() : nullptr;
    const bool bCentered = Cabin && Cabin->CenterHeadsetOnSeat();
    if (MenuLayer && SessionMenu)
    {
        SessionMenu->ShowSeatStatus(bCentered);
        DrawStereoMenu();
    }
    return bCentered;
}

void ARVPracticePlayerController::CenterSeat()
{
    const auto* Assist = GetAssist();
    if (UsesStereoViewport() && Assist && (Assist->bPracticePaused || Assist->GetSpeedKmh() <= Assist->GearChangeLimitKmh))
        TryCenterSeat();
}

void ARVPracticePlayerController::DrawStereoMenu()
{
    const FVector2D Size(1024,1024);
    auto* Target = Cast<UTextureRenderTarget2D>(MenuLayer->GetTexture());
    if (!Target)
    {
        Target = FWidgetRenderer::CreateTargetFor(Size, TF_Bilinear, false);
        if (!Target) return;
        MenuLayer->SetTexture(Target);
    }
    // Submit only when the menu or its selection changes, including while the world is paused.
    auto* Renderer = new FWidgetRenderer(false);
    Renderer->DrawWidget(Target, StereoMenuContent.ToSharedRef(), Size, 0.f);
    BeginCleanup(Renderer);
    MenuLayer->MarkTextureForUpdate();
}

bool ARVPracticePlayerController::InputKey(const FInputKeyEventArgs& Event)
{
    if (MenuLayer && SessionMenu && Event.Event==IE_Pressed)
    {
        const FKey Key = Event.Key;
        if (Key==EKeys::Up || Key==EKeys::Down) SessionMenu->SelectMenuButton(Key==EKeys::Up ? -1 : 1);
        else if (Key==EKeys::Enter) SessionMenu->ActivateMenuButton();
        else if ((Key==EKeys::P || Key==EKeys::M || Key==EKeys::Escape) && SessionMenu->IsShowingControls())
            SessionMenu->ToggleControls();
        else return Super::InputKey(Event);
        if (MenuLayer) DrawStereoMenu();
        return true;
    }
    return Super::InputKey(Event);
}

void ARVPracticePlayerController::CloseMenu()
{
    PendingMenuFocusFrames = 0;
    if (MenuLayer)
    {
        MenuLayer->DestroyComponent();
        MenuLayer = nullptr;
        StereoMenuContent.Reset();
    }
    if (SessionMenu && SessionMenu->IsInViewport()) SessionMenu->RemoveFromParent();
    SessionMenu = nullptr;
}

void ARVPracticePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CloseMenu();
    Super::EndPlay(EndPlayReason);
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
    CloseMenu();
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
        if (SessionScreen==ERVSessionScreen::Start)
        {
            if (UsesStereoViewport() && GEngine->XRSystem.IsValid() && !TryCenterSeat()) return;
            Assist->ResetPractice();
        }
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
    InputComponent->BindKey(EKeys::C, IE_Pressed, this, &ARVPracticePlayerController::CenterSeat).bExecuteWhenPaused=true;
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
