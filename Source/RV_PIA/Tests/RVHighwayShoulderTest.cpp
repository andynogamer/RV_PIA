#if WITH_DEV_AUTOMATION_TESTS

#include "../RVPracticePlayerController.h"
#include "../RVDrivingAssistComponent.h"
#include "../RVDrivingFeedbackComponent.h"
#include "../RVTrafficSystem.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

// Waypoints on the first bend of the cooked Sierra del Pino map, in vehicle-world cm.
// The middle waypoint passes on the road side of the pine at (4748, 2190).
class FRVDriveHighwayShoulder final : public IAutomationLatentCommand
{
public:
    FRVDriveHighwayShoulder(ARVPracticePlayerController* InPC, FAutomationTestBase* InTest)
        : PC(InPC), Test(InTest), Started(FPlatformTime::Seconds()) {}

    bool Update() override
    {
        APawn* Car = PC.IsValid() ? PC->GetPawn() : nullptr;
        if (!Car) { Test->AddError(TEXT("Lost the vehicle in Sierra del Pino.")); return true; }
        auto* Movement = Car->FindComponentByClass<UChaosWheeledVehicleMovementComponent>();
        const FVector Location = Car->GetActorLocation();
        const double Elapsed = FPlatformTime::Seconds()-Started;
        const FVector2D Targets[] = {{2350,200}, {4630,1830}, {7540,365}, {11040,1840}};
        if (Leg < UE_ARRAY_COUNT(Targets))
        {
            if (Elapsed > 85.)
            {
                Test->AddError(FString::Printf(TEXT("Did not reach waypoint %d, vehicle at %s"),Leg,*Location.ToString()));
                PC->ShowStartMenu();
                return true;
            }
            if (Elapsed < 2.)
            {
                Movement->SetThrottleInput(0.f);
                Movement->SetBrakeInput(0.f);
                Movement->SetHandbrakeInput(Elapsed < .5);
                return false;
            }
            const FVector2D Delta = Targets[Leg]-FVector2D(Location);
            const bool bImpactLeg = Leg == UE_ARRAY_COUNT(Targets)-1;
            if (Leg==2) CollisionsAtReentry = Car->FindComponentByClass<URVDrivingFeedbackComponent>()->CollisionCount;
            const bool bReached = bImpactLeg ? Delta.Size()<400. &&
                Car->FindComponentByClass<URVDrivingFeedbackComponent>()->CollisionCount>CollisionsAtReentry : Delta.Size()<235.;
            if (bReached)
            {
                const FString Report = FString::Printf(TEXT("Waypoint %d at %s after %.1fs"), Leg+1, *Location.ToString(), Elapsed);
                Test->AddInfo(Report);
                UE_LOG(LogTemp, Display, TEXT("Sierra: %s"), *Report);
                ++Leg;
                return false;
            }
            const float YawError = FMath::FindDeltaAngleDegrees(Car->GetActorRotation().Yaw, FMath::RadiansToDegrees(FMath::Atan2(Delta.Y,Delta.X)));
            const float Speed = Car->GetVelocity().Size2D()*.036f;
            Movement->SetHandbrakeInput(false);
            Movement->SetSteeringInput(FMath::Clamp(YawError/32.f,-.85f,.85f));
            Movement->SetThrottleInput(FMath::Clamp((15.f-Speed)*.13f,0.f,.65f));
            Movement->SetBrakeInput(Speed>18.f ? .3f : 0.f);
            return false;
        }
        Movement->SetThrottleInput(0.f);
        Movement->SetSteeringInput(0.f);
        Movement->SetBrakeInput(1.f);
        Movement->SetHandbrakeInput(true);
        // The reported failure happened at 124 s, during the second 60 s PSO eviction.
        if (Elapsed < 135.) return false;
        Test->TestTrue(TEXT("Still driving after two cache evictions"),PC->SessionScreen==ERVSessionScreen::Driving);
        PC->ShowStartMenu();
        return true;
    }
private:
    TWeakObjectPtr<ARVPracticePlayerController> PC;
    FAutomationTestBase* Test;
    double Started;
    int32 Leg=0;
    int32 CollisionsAtReentry=0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRVHighwayShoulderTest, "RutaVirtual.Road.HighwayShoulder",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FRVHighwayShoulderTest::RunTest(const FString& Parameters)
{
    UWorld* World = GEngine->GetCurrentPlayWorld();
    auto* PC = World ? Cast<ARVPracticePlayerController>(World->GetFirstPlayerController()) : nullptr;
    if (!TestTrue(TEXT("Isolated Sierra del Pino game at the menu"), FParse::Param(FCommandLine::Get(),TEXT("RVNoWheelInput")) &&
        PC && PC->IsHighway() && PC->SessionMenu && PC->GetPawn() && PC->GetPawn()->FindComponentByClass<UChaosWheeledVehicleMovementComponent>()))
        return false;
    FHitResult Hit;
    const FVector Trunk(11040,1840,345);
    if (!TestTrue(TEXT("A vehicle can hit the pine trunk"), World->LineTraceSingleByChannel(Hit,
        Trunk-FVector(150,0,0),Trunk+FVector(150,0,0),ECC_Vehicle) && Cast<ARVRoadsidePines>(Hit.GetActor()))) return false;
    PC->RestartSession(); // Select the authored seat even when the headset is resting on the desk.
    ADD_LATENT_AUTOMATION_COMMAND(FRVDriveHighwayShoulder(PC,this));
    return true;
}
#endif
