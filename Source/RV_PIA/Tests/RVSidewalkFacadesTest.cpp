#if WITH_DEV_AUTOMATION_TESTS

#include "../RVDrivingFeedbackComponent.h"
#include "../RVPracticePlayerController.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"

// Drive north on the sidewalk into the projecting front of SM_element_084.
class FRVDriveIntoFacade final : public IAutomationLatentCommand
{
public:
    FRVDriveIntoFacade(ARVPracticePlayerController* Controller, FAutomationTestBase* InTest, double ExpectedY)
        : PC(Controller), Test(InTest), ContactY(ExpectedY), Started(FPlatformTime::Seconds()) {}

    bool Update() override
    {
        APawn* Car = PC.IsValid() ? PC->GetPawn() : nullptr;
        auto* Movement = Car ? Car->FindComponentByClass<UChaosWheeledVehicleMovementComponent>() : nullptr;
        auto* Feedback = Car ? Car->FindComponentByClass<URVDrivingFeedbackComponent>() : nullptr;
        if (!Movement || !Feedback)
        {
            Test->AddError(TEXT("Lost the sidewalk test vehicle."));
            return true;
        }
        const double Elapsed = FPlatformTime::Seconds() - Started;
        const double Y = Car->GetActorLocation().Y;
        Movement->SetHandbrakeInput(Elapsed < .5);
        Movement->SetBrakeInput(0);
        Movement->SetSteeringInput(0);
        Movement->SetThrottleInput(Elapsed < 1.5 ? 0.f : .55f);
        // The sweep estimates the stop position using the body dimensions from the parking scenario.
        if (ContactTime < 0 && Feedback->CollisionCount > 0 && FMath::Abs(Y - ContactY) < 85.)
            ContactTime = Elapsed;
        const bool bPassedWall = Y > ContactY + 35.;
        if (bPassedWall || Elapsed > 30. || (ContactTime >= 0 && Elapsed - ContactTime >= 1.5))
        {
            Test->TestTrue(TEXT("Chaos reported a moving contact at the projecting facade"), ContactTime >= 0);
            Test->TestFalse(TEXT("The car stayed outside under continued throttle"), bPassedWall);
            Test->AddInfo(FString::Printf(TEXT("Facade contact at %.1f s, car Y %.0f cm, predicted contact Y %.0f cm."),
                ContactTime, Y, ContactY));
            PC->ShowStartMenu();
            return true;
        }
        return false;
    }
private:
    TWeakObjectPtr<ARVPracticePlayerController> PC;
    FAutomationTestBase* Test;
    double ContactY, Started;
    double ContactTime = -1.;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRVSidewalkFacadesTest, "RutaVirtual.City.SidewalkFacades",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FRVSidewalkFacadesTest::RunTest(const FString& Parameters)
{
    UWorld* World = GEngine->GetCurrentPlayWorld();
    auto* PC = World ? Cast<ARVPracticePlayerController>(World->GetFirstPlayerController()) : nullptr;
    APawn* Car = PC ? PC->GetPawn() : nullptr;
    if (!TestTrue(TEXT("Isolated urban game at the start menu"), FParse::Param(FCommandLine::Get(),TEXT("RVNoWheelInput")) &&
        World && World->GetMapName().Contains(TEXT("L_CiudadUrbana")) && Car && PC->SessionScreen == ERVSessionScreen::Start &&
        Car->FindComponentByClass<UChaosWheeledVehicleMovementComponent>())) return false;
    if (FParse::Param(FCommandLine::Get(),TEXT("RVRequireStereo")) && !TestTrue(TEXT("Active stereo"),PC->UsesStereoViewport()))
        return false;

    bool bDaylight = false;
    for (TActorIterator<ASkyLight> It(World); It; ++It)
    {
        const auto* Light = It->GetLightComponent();
        if (!Light || !Light->IsVisible() || Light->Intensity <= 0.f) continue;
        bDaylight = true;
        TestFalse(TEXT("The city captures its fixed daylight outside the stereo eye pass"), Light->bRealTimeCapture);
    }
    TestTrue(TEXT("Active city skylight"),bDaylight);

    const FCollisionQueryParams Query(SCENE_QUERY_STAT(RVSidewalkFacades),false,Car);
    const auto HitMesh = [](const FHitResult& Hit)
    {
        const auto* Part = Cast<UStaticMeshComponent>(Hit.GetComponent());
        return Part && Part->GetStaticMesh() ? Part->GetStaticMesh()->GetFName() : NAME_None;
    };
    struct FSample { FName Mesh; FVector Point; double IntoWall; };
    const FSample Facades[] = {
        {TEXT("SM_element_026"),{-10426.5,-18086.2,85},-1.},
        {TEXT("SM_element_042"),{-8287.9,-16004.7,85},1.},
        {TEXT("SM_slc_bottom"),{-8160.4,-9604.,85},1.},
        {TEXT("SM_element_084"),{-7787.5,20509.,85},1.}
    };
    for (const auto& Sample : Facades)
    {
        FHitResult Hit;
        const FVector Normal(Sample.IntoWall,0,0);
        const FVector Outside = Sample.Point - Normal * 135.;
        const FCollisionShape Body = FCollisionShape::MakeBox(FVector(50,85,35));
        TestTrue(*FString::Printf(TEXT("%s blocks the vehicle channel at chassis height"),*Sample.Mesh.ToString()),
            World->SweepSingleByChannel(Hit,Outside,Sample.Point+Normal*25.,FQuat::Identity,ECC_Vehicle,Body,Query) &&
            HitMesh(Hit) == Sample.Mesh);
        TestFalse(*FString::Printf(TEXT("Sidewalk clear in front of %s"),*Sample.Mesh.ToString()),
            World->SweepSingleByChannel(Hit,Outside-FVector(0,50,0),Outside+FVector(0,50,0),FQuat::Identity,ECC_Vehicle,Body,Query));
    }

    const FVector Start(-7810.,18800.,85.);
    FHitResult Wall, Ground;
    if (!TestTrue(TEXT("The first obstruction when driving north is the protruding wall"),
        World->SweepSingleByChannel(Wall,Start,Start+FVector(0,2250,0),FQuat::Identity,ECC_Vehicle,
            FCollisionShape::MakeBox(FVector(115,235,40)),Query) && HitMesh(Wall) == FName(TEXT("SM_element_084"))) ||
        !TestTrue(TEXT("Sidewalk supports the approach"),World->LineTraceSingleByChannel(Ground,Start+FVector(0,0,180),
            Start-FVector(0,0,220),ECC_Vehicle,Query))) return false;
    PC->RestartSession();
    Car->SetActorLocationAndRotation(Ground.ImpactPoint+FVector(0,0,75),FRotator(0,90,0),false,nullptr,ETeleportType::TeleportPhysics);
    Car->FindComponentByClass<UChaosWheeledVehicleMovementComponent>()->ResetVehicle();
    ADD_LATENT_AUTOMATION_COMMAND(FRVDriveIntoFacade(PC,this,Wall.Location.Y));
    return true;
}
#endif
