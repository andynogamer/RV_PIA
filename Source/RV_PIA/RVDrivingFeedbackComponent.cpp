#include "RVDrivingFeedbackComponent.h"
#include "RVDrivingAssistComponent.h"
#include "RVPracticePlayerController.h"
#include "RVTrafficSystem.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "WheeledVehiclePawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ARVStopRule::ARVStopRule() { SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("StopLine"))); }
URVDrivingFeedbackComponent::URVDrivingFeedbackComponent()
{
    PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.bTickEvenWhenPaused=true;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
}
void URVDrivingFeedbackComponent::BeginPlay()
{
    Super::BeginPlay();Assist=GetOwner()->FindComponentByClass<URVDrivingAssistComponent>();
    RefreshCollisionNotifications();
    if (auto* Vehicle=Cast<AWheeledVehiclePawn>(GetOwner()))
    {
        Vehicle->GetMesh()->OnComponentHit.AddDynamic(this,&URVDrivingFeedbackComponent::OnVehicleHit);
    }
}
void URVDrivingFeedbackComponent::RefreshCollisionNotifications()
{
    if (auto* Vehicle=Cast<AWheeledVehiclePawn>(GetOwner()))
    {
        Vehicle->GetMesh()->SetNotifyRigidBodyCollision(true);
        Vehicle->GetMesh()->SetAllBodiesNotifyRigidBodyCollision(true);
    }
}
void URVDrivingFeedbackComponent::SuspendFeedback()
{
    OverspeedSeconds=FollowingSeconds=WrongWaySeconds=0;
    bHasPrevious=false;PreviousVelocity=FVector::ZeroVector;
    OnFeedbackStopped.Broadcast();
}
void URVDrivingFeedbackComponent::ResetSession()
{
    SafetyScore=100;PenaltyCount=RewardCount=CollisionCount=0;CleanSeconds=0;
    LastIncidents.Empty();StopTimers.Empty();PassedStops.Empty();RewardedStops.Empty();
    LatestFeedback=FRVDrivingFeedback();LatestTime=-100;LastCheckpoint=LastStage=0;SuspendFeedback();
    RefreshCollisionNotifications();
}
FString URVDrivingFeedbackComponent::GetSummary() const
{
    return FString::Printf(TEXT("Seguridad: %d/100 | Errores: %d | Logros: %d"),SafetyScore,PenaltyCount,RewardCount);
}
bool URVDrivingFeedbackComponent::HasVisibleFeedback() const { return GetWorld()->GetTimeSeconds()-LatestTime<(LatestFeedback.Kind==ERVFeedbackKind::Penalty ? 7.f : 4.f); }
void URVDrivingFeedbackComponent::Emit(FName Code,const FString& Message,ERVFeedbackKind Kind,int32 Points,float HapticRequest)
{
    const int32 Before=SafetyScore;SafetyScore=FMath::Clamp(SafetyScore+Points,0,100);
    LatestFeedback.Code=Code;LatestFeedback.Message=Message;LatestFeedback.Kind=Kind;
    LatestFeedback.ScoreDelta=SafetyScore-Before;LatestFeedback.HapticRequest=FMath::Clamp(HapticRequest,0.f,1.f);
    LatestFeedback.HapticDurationSeconds=HapticRequest>0 ? .15f : 0.f;
    LatestTime=GetWorld()->GetTimeSeconds();OnFeedback.Broadcast(LatestFeedback);
}
void URVDrivingFeedbackComponent::RegisterIncident(FName Code)
{
    if (!Assist || Assist->bPracticePaused) return;
    const float Now=GetWorld()->GetTimeSeconds();const float Cooldown=Code==TEXT("collision") ? 4.f : 10.f;
    if (const float* Last=LastIncidents.Find(Code)) if (Now-*Last<Cooldown) return;
    FString Message;int32 Cost=0;float Request=.25f;
    if (Code==TEXT("red_light")) {Cost=12;Message=TEXT("Cruzaste en rojo. Detente antes de la linea y espera el verde.");}
    else if (Code==TEXT("stop_sign")) {Cost=10;Message=TEXT("ALTO sin detencion. Detente por completo y observa antes de salir.");}
    else if (Code==TEXT("collision")) {Cost=20;Request=.6f;Message=TEXT("Contacto detectado. Frena, recupera espacio y continua con calma.");++CollisionCount;}
    else if (Code==TEXT("overspeed")) {Cost=5;Message=TEXT("Velocidad excesiva. Suelta el acelerador y recupera el limite.");}
    else if (Code==TEXT("following")) {Cost=4;Message=TEXT("Demasiado cerca. Reduce la velocidad y deja espacio al vehiculo de delante.");}
    else if (Code==TEXT("wrong_way")) {Cost=8;Message=TEXT("Circulas en sentido contrario. Detente y recupera tu carril con seguridad.");}
    else return;
    LastIncidents.Add(Code,Now);CleanSeconds=0;++PenaltyCount;
    Emit(Code,Message,ERVFeedbackKind::Penalty,-Cost,Request);
}
void URVDrivingFeedbackComponent::Reward(FName Code,const FString& Message,int32 Points)
{
    ++RewardCount;
    // Preserve a corrective message long enough to read it; still account for the reward.
    if (LatestFeedback.Kind==ERVFeedbackKind::Penalty && GetWorld()->GetTimeSeconds()-LatestTime<7)
    { SafetyScore=FMath::Clamp(SafetyScore+Points,0,100);return; }
    Emit(Code,Message,ERVFeedbackKind::Reward,Points,0);
}
void URVDrivingFeedbackComponent::OnVehicleHit(UPrimitiveComponent* HitComponent,AActor* Other,UPrimitiveComponent* OtherComponent,FVector Impulse,const FHitResult& Hit)
{
    if (!Other || Other==GetOwner() || !Assist || Assist->bPracticePaused || FMath::Abs(Hit.ImpactNormal.Z)>.65f) return;
    const FVector Relative=PreviousVelocity-Other->GetVelocity();
    const float Impact=FMath::Max(FMath::Abs(FVector::DotProduct(Relative,Hit.ImpactNormal)),Impulse.Size()/FMath::Max(1.f,HitComponent->GetMass()))*.036f;
    if (Impact>=MinimumImpactKmh) RegisterIncident(TEXT("collision"));
}
void URVDrivingFeedbackComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Dt,Type,Tick);
    auto* PC=Cast<ARVPracticePlayerController>(GetOwner()->GetInstigatorController());
    if (!Assist || Assist->bPracticePaused || (PC && PC->SessionScreen!=ERVSessionScreen::Driving)) return;
    const FVector Position=GetOwner()->GetActorLocation(),Velocity=GetOwner()->GetVelocity();
    const float Speed=Velocity.Size2D()*.036f;
    const bool Continuous=bHasPrevious && FVector::Dist2D(Position,PreviousPosition)<500;
    // Chaos can consume skeletal hit notifications during a physics-state rebuild.
    // Confirm a sharp horizontal speed loss against nearby blocking geometry too.
    const float Drop=PreviousVelocity.Size2D()-Velocity.Size2D();
    if (Continuous && Drop*.036f>=MinimumImpactKmh && Drop/FMath::Max(.001f,Dt)>2000)
    {
        const FVector Travel=PreviousVelocity.GetSafeNormal2D();
        const FVector Center=Position+FVector(0,0,60);
        FHitResult Contact;FCollisionQueryParams ContactQuery(SCENE_QUERY_STAT(TrainingImpact),false,GetOwner());
        if (GetWorld()->SweepSingleByChannel(Contact,Center,Center+Travel*30,Travel.Rotation().Quaternion(),ECC_Vehicle,
            FCollisionShape::MakeBox(FVector(245,95,45)),ContactQuery) && FMath::Abs(Contact.ImpactNormal.Z)<.65f)
            RegisterIncident(TEXT("collision"));
    }
    Dt=FMath::Min(Dt,.1f);
    OverspeedSeconds=Speed>SpeedLimitKmh+2 ? OverspeedSeconds+Dt : 0;
    if (OverspeedSeconds>2.5f) RegisterIncident(TEXT("overspeed"));

    // Headway warning requires sustained proximity while actually moving forward.
    FHitResult Ahead;FCollisionQueryParams Query(SCENE_QUERY_STAT(TrainingHeadway),false,GetOwner());
    const FVector Forward=GetOwner()->GetActorForwardVector();
    const FVector Sensor=Position+Forward*260+FVector(0,0,50);
    const float SafeGap=FMath::Max(200.f,Speed/.036f);
    const bool HasCar=Speed>8 && FVector::DotProduct(Velocity,Forward)>100 && GetWorld()->SweepSingleByChannel(Ahead,Sensor,Sensor+Forward*SafeGap,
        Forward.Rotation().Quaternion(),ECC_Vehicle,FCollisionShape::MakeBox(FVector(15,70,30)),Query) &&
        (Cast<ARVTrafficVehicle>(Ahead.GetActor()) || Cast<APawn>(Ahead.GetActor()));
    FollowingSeconds=HasCar ? FollowingSeconds+Dt : 0;
    if (FollowingSeconds>1.5f) RegisterIncident(TEXT("following"));

    bool NearCrossing=false;
    for (TActorIterator<ARVTrafficSignal> S(GetWorld());S;++S)
    {
        NearCrossing|=FVector::Dist2D(Position,S->GetActorLocation())<1700;
        const FVector To=S->GetActorLocation()-Position;
        const bool XAxis=FMath::Abs(Forward.X)>FMath::Abs(Forward.Y);
        const FVector Axis=XAxis ? FVector(FMath::Sign(Forward.X),0,0) : FVector(0,FMath::Sign(Forward.Y),0);
        const float Gap=FVector::DotProduct(To,Axis)-S->StopLineOffsetCm-230;
        if (Gap>=0 && Gap<650 && FMath::Abs(FVector::CrossProduct(Axis,To).Z)<1000 && Speed<.5f && S->GetSignalForAxis(XAxis)==ERVSignalColor::Red)
        {
            float& Held=StopTimers.FindOrAdd(*S);Held+=Dt;
            if (Held>1 && !RewardedStops.Contains(*S)) {RewardedStops.Add(*S);Reward(TEXT("safe_stop"),TEXT("Buena detencion en rojo. Espera el verde y observa antes de avanzar."),3);}
        }
        else StopTimers.Remove(*S);
    }
    ARVTrafficRoute* Nearest=nullptr;float NearestDistance=150.f;FVector LaneDirection=FVector::ZeroVector;
    if (Speed>8 && !NearCrossing)
        for (TActorIterator<ARVTrafficRoute> R(GetWorld());R;++R)
        {
            const FVector Point=R->Spline->FindLocationClosestToWorldLocation(Position,ESplineCoordinateSpace::World);
            const float Distance=FVector::Dist2D(Point,Position);
            if (Distance<NearestDistance) {Nearest=*R;NearestDistance=Distance;LaneDirection=R->Spline->FindDirectionClosestToWorldLocation(Position,ESplineCoordinateSpace::World);}
        }
    WrongWaySeconds=Nearest && FVector::DotProduct(Velocity.GetSafeNormal2D(),LaneDirection)<-.6f ? WrongWaySeconds+Dt : 0;
    if (WrongWaySeconds>2.5f) RegisterIncident(TEXT("wrong_way"));

    for (TActorIterator<ARVStopRule> Rule(GetWorld());Rule;++Rule)
    {
        const FVector Axis=Rule->GetActorForwardVector(),Offset=Position-Rule->GetActorLocation();
        const float Along=FVector::DotProduct(Offset,Axis)+230;
        const float Lateral=FMath::Abs(FVector::CrossProduct(Axis,Offset).Z);
        if (Along< -Rule->ApproachCm-500) {PassedStops.Remove(*Rule);StopTimers.Remove(*Rule);}
        if (Lateral>Rule->HalfWidthCm || PassedStops.Contains(*Rule)) continue;
        float& Held=StopTimers.FindOrAdd(*Rule);
        if (Along<0 && Along> -Rule->ApproachCm && Speed<.5f) Held+=Dt;
        if (Continuous && FVector::DotProduct(PreviousPosition-Rule->GetActorLocation(),Axis)+230<0 && Along>=0 && FVector::DotProduct(Velocity,Axis)>20)
        {
            PassedStops.Add(*Rule);
            if (Held<Rule->StopSeconds) RegisterIncident(TEXT("stop_sign"));
            else if (!RewardedStops.Contains(*Rule)) {RewardedStops.Add(*Rule);Reward(TEXT("safe_stop"),TEXT("ALTO realizado. Buena observacion antes de incorporarte."),3);}
        }
    }
    if (Assist->UrbanCheckpointIndex>LastCheckpoint || Assist->PracticeStage>LastStage)
    {
        const bool Complete=Assist->PracticeStage>=(Assist->bUrbanMode ? 3 : 2);
        if (Complete) Reward(TEXT("route_complete"),TEXT("Recorrido completado. Revisa tus avances y vuelve a practicar a tu ritmo."),10);
        else if (Assist->UrbanCheckpointIndex>LastCheckpoint) Reward(TEXT("checkpoint"),TEXT("Objetivo alcanzado. Continua observando el entorno."),3);
    }
    LastCheckpoint=Assist->UrbanCheckpointIndex;LastStage=Assist->PracticeStage;
    if (Speed>4 && !HasCar && WrongWaySeconds==0 && OverspeedSeconds==0) CleanSeconds+=Dt;
    if (CleanSeconds>=FMath::Max(5.f,CleanDrivingRewardSeconds)) {CleanSeconds=0;Reward(TEXT("clean_driving"),TEXT("Buena conduccion mantenida. Conserva este ritmo y la distancia."),2);}
    PreviousPosition=Position;PreviousVelocity=Velocity;bHasPrevious=true;
}
void URVDrivingFeedbackComponent::EndPlay(const EEndPlayReason::Type Reason) {OnFeedbackStopped.Broadcast();Super::EndPlay(Reason);}
