#if WITH_DEV_AUTOMATION_TESTS

#include "../RVPracticePlayerController.h"
#include "../RVDrivingFeedbackComponent.h"
#include "../RVTrafficSystem.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"

// Exercise the parked city start and the north avenue with normal traffic and real Chaos hits.
class FRVDriveUrbanImpacts final : public IAutomationLatentCommand
{
public:
    FRVDriveUrbanImpacts(ARVPracticePlayerController* Controller, UBoxComponent* InWall, FTransform InAvenue,
        FAutomationTestBase* InTest)
        : PC(Controller), Wall(InWall), Avenue(InAvenue), Test(InTest), Started(FPlatformTime::Seconds()) {}

    bool Update() override
    {
        APawn* Car = PC.IsValid() ? PC->GetPawn() : nullptr;
        auto* Movement = Car ? Car->FindComponentByClass<UChaosWheeledVehicleMovementComponent>() : nullptr;
        auto* Feedback = Car ? Car->FindComponentByClass<URVDrivingFeedbackComponent>() : nullptr;
        if (!Movement || !Feedback || !Wall.IsValid()) return Finish(TEXT("Lost the urban test vehicle or obstacle."));
        const double Now = FPlatformTime::Seconds();

        if (PC->SessionScreen == ERVSessionScreen::Paused && Now < NextDrive) return false;
        if (PC->SessionScreen != ERVSessionScreen::Driving)
        {
            if (Now - Started >= 180. && Impacts >= 5)
            {
                Test->TestTrue(TEXT("Drove at least 70 m in one urban run"), LongestRunCm >= 7000.);
                return Finish();
            }
            PC->RestartSession();
            const bool bNorth = (Impacts % 2) != 0;
            if (bNorth)
            {
                Car->SetActorTransform(Avenue, false, nullptr, ETeleportType::TeleportPhysics);
                Movement->ResetVehicle();
            }
            StartPosition = Car->GetActorLocation();
            const FVector Direction = Car->GetActorForwardVector().GetSafeNormal2D();
            Target = StartPosition + Direction * (bNorth ? 9500. : 1450.);
            Wall->SetWorldLocationAndRotation(Target + FVector(0,0,60), Direction.Rotation());
            DriveStarted = Now;
        }

        const double Elapsed = Now - DriveStarted;
        const FVector Position = Car->GetActorLocation();
        const double Travel = FVector::Dist2D(StartPosition,Position);
        LongestRunCm = FMath::Max(LongestRunCm,Travel);
        if (Feedback->CollisionCount > 0 && Travel > 180.)
        {
            ++Impacts;
            UE_LOG(LogTemp,Display,TEXT("UrbanStability: impact %d after %.1f m, %.1f s total"),Impacts,Travel*.01,Now-Started);
            PC->ShowPauseMenu();
            if (PC->SessionScreen != ERVSessionScreen::Paused || !PC->SessionMenu)
                return Finish(TEXT("Failed to show the pause menu after impact."));
            NextDrive = Now + 2.;
            return false;
        }
        if (Elapsed > 55. || Now - Started > 245.)
            return Finish(*FString::Printf(TEXT("Urban drive stalled at %s after %.1f m and %d impacts"),*Position.ToString(),Travel*.01,Impacts));
        const float YawError = FMath::FindDeltaAngleDegrees(Car->GetActorRotation().Yaw,(Target-Position).Rotation().Yaw);
        Movement->SetHandbrakeInput(Elapsed < .5);
        Movement->SetBrakeInput(0.f);
        Movement->SetThrottleInput(Elapsed > 1.5 ? .65f : 0.f);
        Movement->SetSteeringInput(FMath::Clamp(YawError / 32.f,-.75f,.75f));
        return false;
    }

private:
    bool Finish(const TCHAR* Error = nullptr)
    {
        if (Error) Test->AddError(Error);
        if (Wall.IsValid()) Wall->GetOwner()->Destroy();
        if (PC.IsValid()) PC->ShowStartMenu();
        return true;
    }
    TWeakObjectPtr<ARVPracticePlayerController> PC;
    TWeakObjectPtr<UBoxComponent> Wall;
    FTransform Avenue;
    FAutomationTestBase* Test;
    double Started, DriveStarted=0., NextDrive=0., LongestRunCm=0.;
    int32 Impacts=0;
    FVector StartPosition, Target;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRVUrbanStabilityTest, "RutaVirtual.City.DrivingStability",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FRVUrbanStabilityTest::RunTest(const FString& Parameters)
{
    UWorld* World = GEngine->GetCurrentPlayWorld();
    auto* PC = World ? Cast<ARVPracticePlayerController>(World->GetFirstPlayerController()) : nullptr;
    if (!TestTrue(TEXT("Isolated city game at the start menu"),FParse::Param(FCommandLine::Get(),TEXT("RVNoWheelInput")) &&
        PC && PC->SessionScreen == ERVSessionScreen::Start && PC->SessionMenu && World->GetMapName().Contains(TEXT("L_CiudadUrbana"))))
        return false;

    const bool bStereo = PC->UsesStereoViewport();
    AddInfo(bStereo ? TEXT("Urban test is running in a stereo viewport.") : TEXT("Urban test is running on the desktop."));
    if (FParse::Param(FCommandLine::Get(),TEXT("RVRequireStereo")) && !TestTrue(TEXT("Active stereoscopic viewport"),bStereo))
        return false;

    const FVector NorthEntry(-9100,25700,70);
    FTransform Avenue;
    double NearestCm = 1200.;
    bool bFoundAvenue = false;
    for (TActorIterator<ARVTrafficRoute> It(World); It; ++It)
    {
        auto* Lane = It->Spline.Get();
        const float Key = Lane->FindInputKeyClosestToWorldLocation(NorthEntry);
        const FVector Point = Lane->GetLocationAtSplineInputKey(Key,ESplineCoordinateSpace::World);
        const double Distance = FVector::Dist2D(Point,NorthEntry);
        if (Distance >= NearestCm) continue;
        FVector Direction = Lane->GetDirectionAtSplineInputKey(Key,ESplineCoordinateSpace::World);
        if (FMath::Abs(Direction.Y) < .9f) continue;
        if (Direction.Y < 0.) Direction *= -1.;
        Avenue = FTransform(Direction.Rotation(),Point+FVector(0,0,30));
        bFoundAvenue = true;
        NearestCm = Distance;
    }
    if (!TestTrue(TEXT("Found a lane on the north avenue"),bFoundAvenue)) return false;
    AActor* Obstacle = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Created temporary obstacle"),Obstacle)) return false;
    auto* Wall = NewObject<UBoxComponent>(Obstacle);
    Obstacle->SetRootComponent(Wall);
    Wall->SetBoxExtent(FVector(35,420,150));
    Wall->SetCollisionProfileName(TEXT("BlockAll"));
    Wall->RegisterComponent();
    ADD_LATENT_AUTOMATION_COMMAND(FRVDriveUrbanImpacts(PC,Wall,Avenue,this));
    return true;
}
#endif
