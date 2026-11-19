#if WITH_DEV_AUTOMATION_TESTS

#include "../RVPracticePlayerController.h"
#include "../RVSessionMenuWidget.h"
#include "../RVWheelInput.h"
#include "Camera/CameraComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Components/StereoLayerComponent.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "ImageUtils.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Serialization/BufferArchive.h"

// The OpenXR origin takes effect on a subsequent XR frame. Keep the headset still for this measurement.
class FRVCheckSeatedView final : public IAutomationLatentCommand
{
public:
    FRVCheckSeatedView(ARVPracticePlayerController* InPC, FAutomationTestBase* InTest) : PC(InPC), Test(InTest) {}
    bool Update() override
    {
        if (++Frames < 8) return false;
        if (!PC.IsValid() || !PC->GetPawn()) { Test->AddError(TEXT("Lost the vehicle before measuring the seat.")); return true; }
        TArray<UCameraComponent*> Cameras;
        PC->GetPawn()->GetComponents(Cameras);
        const auto* Found = Cameras.FindByPredicate([](UCameraComponent* C){ return C->IsActive() && C->GetName().StartsWith(TEXT("FrontCamera")); });
        if (Test->TestTrue(TEXT("Active cabin camera"), Found && (*Found)->GetAttachParent()))
        {
            const auto* Camera = *Found;
            const auto Seat = Camera->GetAttachParent()->GetSocketTransform(Camera->GetAttachSocketName());
            const float Distance = FVector::Distance(Camera->GetComponentLocation(), Seat.GetLocation());
            const float Yaw = FMath::Abs(PC->GetPawn()->GetActorTransform().InverseTransformVectorNoScale(Camera->GetForwardVector()).Rotation().Yaw);
            Test->AddInfo(FString::Printf(TEXT("Seat error %.1f cm, vehicle heading error %.1f degrees."),Distance,Yaw));
            Test->TestTrue(TEXT("Seated head near the authored eye point"), Distance < 12.f);
            Test->TestTrue(TEXT("Facing the front of the car"), Yaw < 12.f);
        }
        PC->ShowStartMenu();
        return true;
    }
private:
    TWeakObjectPtr<ARVPracticePlayerController> PC;
    FAutomationTestBase* Test;
    int32 Frames=0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRVStereoMenuTest, "RutaVirtual.Session.StereoMenu",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FRVStereoMenuTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("RVNoWheelInput")))
    {
        AddError(TEXT("Use a dedicated test game with -RVNoWheelInput."));
        return false;
    }
    UWorld* World = GEngine->GetCurrentPlayWorld();
    auto* PC = World ? Cast<ARVPracticePlayerController>(World->GetFirstPlayerController()) : nullptr;
    if (!TestTrue(TEXT("Parked at the start menu"), PC && PC->SessionScreen==ERVSessionScreen::Start && PC->SessionMenu))
        return false;
    const auto Layer = [PC]() { return PC->FindComponentByClass<UStereoLayerComponent>(); };
    if (!TestNotNull(TEXT("Stereo layer in the game world"), Layer())) return false;
    auto* Texture = Cast<UTextureRenderTarget2D>(Layer()->GetTexture());
    if (!TestNotNull(TEXT("Visible menu texture"), Texture)) return false;
    TestTrue(TEXT("Full UVs and shared image for both eyes"), Layer()->GetUVRect()==FBox2D(FVector2D(0,0),FVector2D(1,1)) && !Layer()->GetLeftTexture());
    TestTrue(TEXT("Quad preserves menu aspect"), FMath::IsNearlyEqual(Layer()->GetQuadSize().X / Layer()->GetQuadSize().Y,
        double(Texture->SizeX) / Texture->SizeY));
    TestFalse(TEXT("Desktop overlay excluded"), PC->SessionMenu->IsInViewport());
    for (const FName Name : {TEXT("PrimaryButton"),TEXT("ControlsButton"),TEXT("RoadButton"),TEXT("ParkingButton"),TEXT("QuitButton")})
    {
        auto* Button = PC->SessionMenu->GetWidgetFromName(Name);
        if (!TestNotNull(Name.ToString(), Button)) return false;
        const auto Rect = Button->GetCachedGeometry().GetRenderBoundingRect();
        TestTrue(Name.ToString()+TEXT(" entirely inside the texture"), Rect.Right-Rect.Left>100 && Rect.Bottom-Rect.Top>25 &&
            Rect.Left>=0 && Rect.Top>=0 && Rect.Right<=Texture->SizeX && Rect.Bottom<=Texture->SizeY);
    }
    const auto Directory = FPaths::ProjectSavedDir()/TEXT("AgentReports");
    IFileManager::Get().MakeDirectory(*Directory, true);
    const auto CaptureMenu = [&](const TCHAR* File)
    {
        FBufferArchive Png;
        return FImageUtils::ExportRenderTarget2DAsPNG(Texture,Png) && FFileHelper::SaveArrayToFile(Png,*(Directory/File));
    };
    TestTrue(TEXT("Export rendered menu"), CaptureMenu(TEXT("quest_menu.png")));

    const auto Press = [PC](FKey Key)
    {
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1.f));
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0.f));
    };
    Press(EKeys::Down);
    Press(EKeys::Enter);
    if (!TestTrue(TEXT("Arrows and Enter open controls"), PC->SessionMenu && PC->SessionMenu->IsShowingControls())) return false;
    TestTrue(TEXT("Export wheel buttons and labels"), CaptureMenu(TEXT("quest_controles_g920.png")));
    Press(EKeys::P);
    Press(EKeys::Enter);
    TestTrue(TEXT("Enter begins driving and removes the layer"), PC->SessionScreen==ERVSessionScreen::Driving &&
        !UGameplayStatics::IsGamePaused(World) && !Layer() && !PC->SessionMenu);
    PC->ShowPauseMenu();
    TestTrue(TEXT("Paused game presents the stereo menu"), UGameplayStatics::IsGamePaused(World) && Layer());
    Press(EKeys::Enter);
    TestTrue(TEXT("Enter resumes and removes the pause menu"), PC->SessionScreen==ERVSessionScreen::Driving && !Layer());
    PC->ShowStartMenu();
    const auto Tap = [PC](ERVWheelButton Button)
    {
        PC->HandleWheelButtons(Button);
        PC->HandleWheelButtons(ERVWheelButton::None);
    };
    PC->HandleWheelButtons({});
    PC->HandleWheelButtons(ERVWheelButton::Accept);
    TestTrue(TEXT("Held A on reconnect keeps the car parked"), PC->SessionScreen==ERVSessionScreen::Start);
    PC->HandleWheelButtons(ERVWheelButton::None);
    PC->HandleWheelButtons(ERVWheelButton::Down);
    PC->HandleWheelButtons(ERVWheelButton::Down);
    Tap(ERVWheelButton::Accept);
    if (!TestTrue(TEXT("D-pad advances once and A opens controls"), PC->SessionMenu && PC->SessionMenu->IsShowingControls())) return false;
    Tap(ERVWheelButton::Back);
    TestFalse(TEXT("B returns to the start page"), PC->SessionMenu->IsShowingControls());
    Tap(ERVWheelButton::Accept);
    if (!TestTrue(TEXT("A enters the cabin"), PC->SessionScreen==ERVSessionScreen::Driving)) return false;
    auto* Movement = PC->GetPawn()->FindComponentByClass<UChaosWheeledVehicleMovementComponent>();
    if (!TestNotNull(TEXT("Chaos vehicle"), Movement)) return false;
    const bool InitialBrake = Movement->GetHandbrakeInput();
    Tap(ERVWheelButton::Handbrake);
    TestTrue(TEXT("X operates the parking brake at rest"), Movement->GetHandbrakeInput()!=InitialBrake);
    Tap(ERVWheelButton::Handbrake);
    Tap(ERVWheelButton::Menu);
    TestTrue(TEXT("Menu on the wheel pauses the world"), PC->SessionScreen==ERVSessionScreen::Paused && UGameplayStatics::IsGamePaused(World));
    Tap(ERVWheelButton::Center);
    Tap(ERVWheelButton::Back);
    TestTrue(TEXT("B resumes from pause"), PC->SessionScreen==ERVSessionScreen::Driving && !Layer());
    if (GEngine->XRSystem.IsValid())
    {
        TestTrue(TEXT("OpenXR uses the local seated origin"), UHeadMountedDisplayFunctionLibrary::GetTrackingOrigin()==EHMDTrackingOrigin::Local);
        ADD_LATENT_AUTOMATION_COMMAND(FRVCheckSeatedView(PC,this));
    }
    else PC->ShowStartMenu();
    return true;
}
#endif
