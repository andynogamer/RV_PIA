#include "RVTrafficSystem.h"
#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ARVTrafficSignal::ARVTrafficSignal()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SignalRoot")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Black(TEXT("/Game/RutaVirtual/Materials/M_BaseCono"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Lamp(TEXT("/Game/RutaVirtual/Materials/M_TrafficLamp"));
    for (int32 Approach = 0; Approach < 4; ++Approach)
    {
        const bool XAxis = Approach >= 2;
        const float Sign = Approach % 2 == 0 ? 1.f : -1.f;
        const FVector Corner = XAxis ? FVector(950.f*Sign,650.f*Sign,0) : FVector(650.f*Sign,-950.f*Sign,0);
        auto* Pole = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Pole%d"),Approach));
        Pole->SetupAttachment(RootComponent); Pole->SetStaticMesh(Cube.Object);
        Pole->SetRelativeLocation(Corner+FVector(0,0,220)); Pole->SetRelativeScale3D(FVector(.10f,.10f,4.4f));
        Pole->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        auto* Head = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Head%d"),Approach));
        Head->SetupAttachment(RootComponent); Head->SetStaticMesh(Cube.Object); Head->SetMaterial(0,Black.Object);
        Head->SetRelativeLocation(Corner+FVector(0,0,375));
        Head->SetRelativeScale3D(XAxis ? FVector(.22f,.46f,1.12f) : FVector(.46f,.22f,1.12f));
        Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        for (int32 Color = 0; Color < 3; ++Color)
        {
            auto* Lens = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Lamp%d_%d"),Approach,Color));
            Lens->SetupAttachment(RootComponent); Lens->SetStaticMesh(Sphere.Object); Lens->SetMaterial(0,Lamp.Object);
            const FVector Face = XAxis ? FVector(-Sign*15,0,0) : FVector(0,Sign*15,0);
            Lens->SetRelativeLocation(Corner+Face+FVector(0,0,410-Color*34));
            Lens->SetRelativeScale3D(FVector(.24f)); Lens->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Lamps.Add(Lens);
        }
    }
}

int32 ARVTrafficSignal::GetPhase() const
{
    if (PhaseOverride >= 0 && PhaseOverride <= 5) return PhaseOverride;
    const float Green = FMath::Max(1.f,GreenSeconds), Amber = FMath::Max(1.f,AmberSeconds), Clear = FMath::Max(1.f,ClearanceSeconds);
    float T = GetWorld() && GetWorld()->IsGameWorld() ? GetWorld()->GetTimeSeconds()+CycleOffset : 0.f;
    T = FMath::Fmod(FMath::Max(0.f,T),2.f*(Green+Amber+Clear));
    const float Durations[] = {Green,Amber,Clear,Green,Amber,Clear};
    for (int32 Index=0; Index<6; ++Index) { if (T < Durations[Index]) return Index; T-=Durations[Index]; }
    return 5;
}

ERVSignalColor ARVTrafficSignal::GetSignalForAxis(bool bXAxis) const
{
    const int32 Phase = GetPhase();
    if (Phase == (bXAxis ? 3 : 0)) return ERVSignalColor::Green;
    if (Phase == (bXAxis ? 4 : 1)) return ERVSignalColor::Amber;
    return ERVSignalColor::Red;
}

void ARVTrafficSignal::RefreshLights()
{
    if (!GetWorld() || !GetWorld()->IsGameWorld()) return;
    if (LampMaterials.Num()!=Lamps.Num())
    {
        LampMaterials.Empty();
        auto* Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/RutaVirtual/Materials/M_TrafficLamp.M_TrafficLamp"));
        for (const auto& Lens : Lamps)
        {
            Lens->SetMaterial(0,Parent);
            LampMaterials.Add(Lens->CreateDynamicMaterialInstance(0));
        }
    }
    const FLinearColor Colors[] = {FLinearColor(5,0.02f,0.01f),FLinearColor(5,2.5f,0),FLinearColor(0.01f,4,0.05f)};
    for (int32 Index=0; Index<Lamps.Num(); ++Index)
    {
        const auto Signal = GetSignalForAxis(Index/3>=2);
        const int32 Active = Signal==ERVSignalColor::Red ? 0 : (Signal==ERVSignalColor::Amber ? 1 : 2);
        if (LampMaterials[Index]) LampMaterials[Index]->SetVectorParameterValue(TEXT("LampColor"), Index%3==Active ? Colors[Active] : FLinearColor(.015f,.015f,.015f));
    }
}
void ARVTrafficSignal::OnConstruction(const FTransform& Transform) { Super::OnConstruction(Transform); RefreshLights(); }
void ARVTrafficSignal::BeginPlay() { Super::BeginPlay(); RefreshLights(); }
void ARVTrafficSignal::Tick(float DeltaTime) { Super::Tick(DeltaTime); RefreshLights(); }

ARVTrafficRoute::ARVTrafficRoute()
{
    Spline=CreateDefaultSubobject<USplineComponent>(TEXT("LaneSpline")); SetRootComponent(Spline);
}

ARVTrafficVehicle::ARVTrafficVehicle()
{
    PrimaryActorTick.bCanEverTick=true;
    Collision=CreateDefaultSubobject<UBoxComponent>(TEXT("TrafficCollision")); SetRootComponent(Collision);
    Collision->SetBoxExtent(FVector(238,115,65)); Collision->SetCollisionProfileName(TEXT("Vehicle"));
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Car(TEXT("/Game/Vehicles/SportsCar/SM_SportsCar"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Glass(TEXT("/Game/Vehicles/SportsCar/SM_SportsCar_Glass"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Wheel(TEXT("/Game/Vehicles/SportsCar/SM_SportsCar_Wheel"));
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrafficBody")); Body->SetupAttachment(RootComponent);
    Body->SetStaticMesh(Car.Object); Body->SetRelativeLocation(FVector(0,0,-55)); Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* Windows=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrafficGlass")); Windows->SetupAttachment(Body);
    Windows->SetStaticMesh(Glass.Object); Windows->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    for (int32 Index=0; Index<4; ++Index)
    {
        auto* Tire=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("TrafficWheel%d"),Index));
        Tire->SetupAttachment(RootComponent); Tire->SetStaticMesh(Wheel.Object);
        Tire->SetRelativeLocation(FVector(Index<2 ? 142.f : -136.f, Index%2==0 ? -100.f : 100.f,-31.f));
        Tire->SetCollisionEnabled(ECollisionEnabled::NoCollision); Wheels.Add(Tire);
    }
}

void ARVTrafficVehicle::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Paint=Body->CreateDynamicMaterialInstance(1)) Paint->SetVectorParameterValue(TEXT("Paint Tint"),BodyColor);
    TeleportToDistance(DistanceAlongRoute);
}
void ARVTrafficVehicle::TeleportToDistance(float DistanceCm)
{
    if (!Route || !Route->Spline) return;
    DistanceAlongRoute=FMath::Clamp(DistanceCm,0.f,Route->Spline->GetSplineLength()); SpeedCmS=0;
    SetActorLocationAndRotation(Route->Spline->GetLocationAtDistanceAlongSpline(DistanceAlongRoute,ESplineCoordinateSpace::World),
        Route->Spline->GetDirectionAtDistanceAlongSpline(DistanceAlongRoute,ESplineCoordinateSpace::World).Rotation(),false,nullptr,ETeleportType::TeleportPhysics);
}

void ARVTrafficVehicle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!Route || !Route->Spline) return;
    const float Dt=FMath::Min(DeltaTime,.05f);
    const FVector Forward=Route->Spline->GetDirectionAtDistanceAlongSpline(DistanceAlongRoute,ESplineCoordinateSpace::World);
    float Desired=FMath::Clamp(Route->CruiseKmh,5.f,35.f)/.036f;
    StopReason=TEXT("cruise");
    bool SignalStop=false;
    for (TActorIterator<ARVTrafficSignal> It(GetWorld()); It; ++It)
    {
        const FVector ToSignal=It->GetActorLocation()-GetActorLocation();
        const float Ahead=FVector::DotProduct(ToSignal,Forward);
        const float Lateral=FMath::Abs(FVector::CrossProduct(Forward,ToSignal).Z);
        if (Ahead > It->StopLineOffsetCm-100 && Ahead<5000 && Lateral<1300)
        {
            const auto Signal=It->GetSignalForAxis(FMath::Abs(Forward.X)>FMath::Abs(Forward.Y));
            const float Gap=FMath::Max(0.f,Ahead-It->StopLineOffsetCm-260);
            const bool AmberCanStop=SpeedCmS*SpeedCmS/(2*BrakingCmS2)+150 < Gap;
            if (Signal==ERVSignalColor::Red || (Signal==ERVSignalColor::Amber && AmberCanStop))
            {
                Desired=FMath::Min(Desired,FMath::Sqrt(2*BrakingCmS2*FMath::Max(0.f,Gap-80)));
                if (Gap<120) Desired=0;
                SignalStop=true; StopReason=TEXT("signal");
            }
        }
    }
    const float LookAhead=FMath::Max(600.f,SpeedCmS*SpeedCmS/(2*BrakingCmS2)+500.f);
    const FVector Start=GetActorLocation()+Forward*265+FVector(0,0,40);
    FHitResult Obstacle;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(TrafficLookAhead),false,this);
    const bool Blocked=GetWorld()->SweepSingleByChannel(Obstacle,Start,Start+Forward*LookAhead,Forward.Rotation().Quaternion(),ECC_Vehicle,
        FCollisionShape::MakeBox(FVector(40,95,35)),Params);
    if (Blocked)
    {
        const float Gap=FMath::Max(0.f,Obstacle.Distance-240);
        Desired=FMath::Min(Desired,FMath::Sqrt(2*BrakingCmS2*Gap));
        if (Gap<140) Desired=0;
        if (!SignalStop) StopReason=Cast<APawn>(Obstacle.GetActor()) || Cast<ARVTrafficVehicle>(Obstacle.GetActor()) ? TEXT("vehicle") : TEXT("obstacle");
    }
    const float Remaining=Route->Spline->GetSplineLength()-DistanceAlongRoute;
    if (Remaining<500) { Desired=0; StopReason=TEXT("end"); }
    const float Rate=Desired<SpeedCmS ? BrakingCmS2 : AccelerationCmS2;
    SpeedCmS=FMath::FInterpConstantTo(SpeedCmS,Desired,Dt,Rate);
    const float Next=FMath::Min(Route->Spline->GetSplineLength(),DistanceAlongRoute+SpeedCmS*Dt);
    FHitResult Contact;
    const FVector Target=Route->Spline->GetLocationAtDistanceAlongSpline(Next,ESplineCoordinateSpace::World);
    SetActorRotation(Forward.Rotation());
    const bool Moved=SetActorLocation(Target,true,&Contact);
    if (Contact.bBlockingHit) { SpeedCmS=0; if (!SignalStop) StopReason=TEXT("obstacle"); }
    if (Moved && !Contact.bBlockingHit) DistanceAlongRoute=Next;
    for (const auto& Tire : Wheels) Tire->AddLocalRotation(FRotator(FMath::RadiansToDegrees(SpeedCmS*Dt/39.3f),0,0));
    const bool PlayerBlocking=Blocked && Obstacle.GetActor()==UGameplayStatics::GetPlayerPawn(GetWorld(),0);
    BlockedSeconds=PlayerBlocking && !SignalStop && SpeedCmS<10 ? BlockedSeconds+Dt : 0.f;
    if (BlockedSeconds>HornWaitSeconds && GetWorld()->GetTimeSeconds()-LastHornTime>HornCooldownSeconds)
    {
        LastHornTime=GetWorld()->GetTimeSeconds(); ++HornEventCount;
        OnHornRequested.Broadcast(this,TEXT("player_obstruction"));
        for (TActorIterator<ARVUrbanScenario> It(GetWorld()); It; ++It) { ++It->HornEvents; It->OnHornRequested.Broadcast(this,TEXT("player_obstruction")); }
    }
    if (Remaining<500 && SpeedCmS<1)
    {
        const APawn* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
        const FVector Beginning=Route->Spline->GetLocationAtDistanceAlongSpline(0,ESplineCoordinateSpace::World);
        const FVector StartDirection=Route->Spline->GetDirectionAtDistanceAlongSpline(0,ESplineCoordinateSpace::World);
        bool ClearStart=true;
        for (TActorIterator<ARVTrafficVehicle> It(GetWorld()); It; ++It)
        {
            const FVector Offset=It->GetActorLocation()-Beginning;
            // The opposite lane can end beside this lane's start. It must not deadlock recycling.
            if (*It!=this && FMath::Abs(FVector::DotProduct(Offset,StartDirection))<1000.f &&
                FMath::Abs(FVector::CrossProduct(StartDirection,Offset).Z)<280.f && FMath::Abs(Offset.Z)<150.f) ClearStart=false;
        }
        if (ClearStart && (!Player || (FVector::Dist(Player->GetActorLocation(),Beginning)>7000 && FVector::Dist(Player->GetActorLocation(),GetActorLocation())>7000))) TeleportToDistance(0);
    }
}

ARVUrbanScenario::ARVUrbanScenario()
{
    PrimaryActorTick.bCanEverTick=true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("UrbanRoot")));
}
void ARVUrbanScenario::BeginPlay()
{
    Super::BeginPlay();
    if (!bSpawnTraffic) return;
    const FLinearColor Colors[]={FLinearColor(.08f,.15f,.4f),FLinearColor(.5f,.03f,.02f),FLinearColor(.6f,.62f,.65f),FLinearColor(.03f,.1f,.04f)};
    int32 Count=0;
    for (TActorIterator<ARVTrafficRoute> It(GetWorld()); It; ++It)
    {
        for (int32 Index=0; Index<FMath::Clamp(It->VehicleCount,0,12); ++Index)
        {
            auto* Agent=GetWorld()->SpawnActorDeferred<ARVTrafficVehicle>(ARVTrafficVehicle::StaticClass(),FTransform::Identity,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if (!Agent) continue;
            Agent->Route=*It; Agent->BodyColor=Colors[Count++%4];
            Agent->DistanceAlongRoute=It->Spline->GetSplineLength()*(Index+.18f)/FMath::Max(1,It->VehicleCount);
            Agent->FinishSpawning(FTransform::Identity);
        }
    }
}
bool ARVUrbanScenario::IsInsideBounds(FVector Position) const
{
    return Position.X>=BoundsMin.X && Position.X<=BoundsMax.X && Position.Y>=BoundsMin.Y && Position.Y<=BoundsMax.Y && Position.Z>=BoundsMin.Z;
}
void ARVUrbanScenario::RequestPlayerHorn(AActor* Vehicle)
{
    if (GetWorld()->GetTimeSeconds()-LastPlayerHornTime<1.f) return;
    LastPlayerHornTime=GetWorld()->GetTimeSeconds(); ++HornEvents; OnHornRequested.Broadcast(Vehicle,TEXT("manual"));
}
void ARVUrbanScenario::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    const APawn* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
    if (!Player || Player->GetVelocity().Size()<60) return;
    const FVector Forward=Player->GetActorForwardVector();
    for (TActorIterator<ARVTrafficSignal> It(GetWorld()); It; ++It)
    {
        const FVector Offset=It->GetActorLocation()-Player->GetActorLocation();
        const float Ahead=FVector::DotProduct(Offset,Forward);
        const float Lateral=FMath::Abs(FVector::CrossProduct(Forward,Offset).Z);
        const float* Previous=PreviousApproachDistance.Find(*It);
        if (Previous && *Previous>It->StopLineOffsetCm && Ahead<=It->StopLineOffsetCm && Ahead>0 && Lateral<1200 &&
            It->GetSignalForAxis(FMath::Abs(Forward.X)>FMath::Abs(Forward.Y))==ERVSignalColor::Red) ++RedLightWarnings;
        PreviousApproachDistance.Add(*It,Ahead);
    }
}
