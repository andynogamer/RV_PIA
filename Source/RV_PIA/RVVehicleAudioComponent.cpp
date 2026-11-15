#include "RVVehicleAudioComponent.h"
#include "RVDrivingAssistComponent.h"
#include "RVTrafficSystem.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "WheeledVehiclePawn.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

URVVehicleAudioComponent::URVVehicleAudioComponent()
{
    PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.bTickEvenWhenPaused=true;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
    static ConstructorHelpers::FObjectFinder<USoundBase> Engine(TEXT("/Game/RutaVirtual/Audio/S_EngineLoop"));EngineSound=Engine.Object;
    static ConstructorHelpers::FObjectFinder<USoundBase> Road(TEXT("/Game/RutaVirtual/Audio/S_RoadLoop"));RoadSound=Road.Object;
    static ConstructorHelpers::FObjectFinder<USoundBase> City(TEXT("/Game/RutaVirtual/Audio/S_CityLoop"));CitySound=City.Object;
    static ConstructorHelpers::FObjectFinder<USoundBase> Horn(TEXT("/Game/RutaVirtual/Audio/S_Horn"));HornSound=Horn.Object;
    static ConstructorHelpers::FObjectFinder<USoundBase> Impact(TEXT("/Game/RutaVirtual/Audio/S_Impact"));ImpactSound=Impact.Object;
    static ConstructorHelpers::FObjectFinder<USoundBase> Reward(TEXT("/Game/RutaVirtual/Audio/S_Reward"));RewardSound=Reward.Object;
}
UAudioComponent* URVVehicleAudioComponent::CreateLoop(FName Name,USoundBase* Sound)
{
    auto* Audio=NewObject<UAudioComponent>(GetOwner(),Name);
    Audio->SetupAttachment(GetOwner()->GetRootComponent());Audio->bAutoActivate=false;Audio->bAutoDestroy=false;
    Audio->SetSound(Sound);Audio->SetVolumeMultiplier(0);GetOwner()->AddInstanceComponent(Audio);Audio->RegisterComponent();
    Audio->Play();return Audio;
}
void URVVehicleAudioComponent::BeginPlay()
{
    Super::BeginPlay();
    EngineAudio=CreateLoop(TEXT("PlayerEngineAudio"),EngineSound);
    RoadAudio=CreateLoop(TEXT("PlayerRoadAudio"),RoadSound);
    CityAudio=CreateLoop(TEXT("PlayerCityAudio"),CitySound);
    if (auto* Feedback=GetOwner()->FindComponentByClass<URVDrivingFeedbackComponent>())
    {
        Feedback->OnFeedback.AddDynamic(this,&URVVehicleAudioComponent::HandleFeedback);
        Feedback->OnFeedbackStopped.AddDynamic(this,&URVVehicleAudioComponent::SilenceTransientSounds);
    }
    for (TActorIterator<ARVUrbanScenario> S(GetWorld());S;++S) S->OnHornRequested.AddDynamic(this,&URVVehicleAudioComponent::PlayHorn);
}
void URVVehicleAudioComponent::ToggleMute() {bMuted=!bMuted;if (bMuted) SilenceTransientSounds();}
void URVVehicleAudioComponent::SilenceTransientSounds()
{
    for (const auto& Cue:Cues) if (IsValid(Cue)) Cue->Stop();
    Cues.Empty();
}
void URVVehicleAudioComponent::PlayCue(USoundBase* Sound,float Volume,AActor* SpatialSource)
{
    const auto* Assist=GetOwner()->FindComponentByClass<URVDrivingAssistComponent>();
    if (!Sound || bMuted || (Assist && Assist->bPracticePaused)) return;
    AActor* Source=SpatialSource ? SpatialSource : GetOwner();
    auto* Cue=NewObject<UAudioComponent>(Source);
    Cue->SetupAttachment(Source->GetRootComponent());Cue->bAutoActivate=false;Cue->bAutoDestroy=true;
    Cue->SetSound(Sound);Cue->SetVolumeMultiplier(Volume*FMath::Clamp(MasterVolume,0.f,1.f));
    if (SpatialSource && SpatialSource!=GetOwner())
    {
        FSoundAttenuationSettings Settings;Settings.bAttenuate=true;Settings.bSpatialize=true;
        Settings.AttenuationShapeExtents=FVector(250,0,0);Settings.FalloffDistance=2200;
        Cue->AdjustAttenuation(Settings);
    }
    Cue->RegisterComponent();Cues.Add(Cue);Cue->Play();
}
void URVVehicleAudioComponent::PlayHorn(AActor* Vehicle,FName Reason)
{
    const float Now=GetWorld()->GetTimeSeconds();
    if (!Vehicle || Now-LastAnyHorn<.4f) return;
    if (const float* Last=LastHorns.Find(Vehicle)) if (Now-*Last<1) return;
    LastHorns.Add(Vehicle,Now);LastAnyHorn=Now;
    const auto* Assist=GetOwner()->FindComponentByClass<URVDrivingAssistComponent>();
    if (bMuted || (Assist && Assist->bPracticePaused)) return;
    ++HornSoundsPlayed;PlayCue(HornSound,.3f,Vehicle);
}
void URVVehicleAudioComponent::HandleFeedback(const FRVDrivingFeedback& Feedback)
{
    const float Now=GetWorld()->GetTimeSeconds();
    if (bMuted || Now-LastFeedbackSound<(Feedback.Kind==ERVFeedbackKind::Penalty ? 4.f : 2.f)) return;
    LastFeedbackSound=Now;++FeedbackSoundsPlayed;
    if (Feedback.Kind==ERVFeedbackKind::Penalty)
    {
        if (Feedback.Code==TEXT("collision")) PlayCue(ImpactSound,.35f);
        PlayCue(HornSound,.18f);
    }
    else if (Feedback.Kind==ERVFeedbackKind::Reward) PlayCue(RewardSound,.14f);
}
void URVVehicleAudioComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Dt,Type,Tick);
    const auto* Assist=GetOwner()->FindComponentByClass<URVDrivingAssistComponent>();
    const bool Quiet=bMuted || (Assist && Assist->bPracticePaused);
    for (auto* Audio:{EngineAudio.Get(),RoadAudio.Get(),CityAudio.Get()}) if (Audio) Audio->SetPaused(Quiet);
    if (Quiet) {SilenceTransientSounds();return;}
    const auto* Vehicle=Cast<AWheeledVehiclePawn>(GetOwner());
    auto* Movement=Vehicle ? Cast<UChaosWheeledVehicleMovementComponent>(Vehicle->GetVehicleMovementComponent()) : nullptr;
    if (!Movement || !EngineAudio) return;
    const float Speed=FMath::Abs(Movement->GetForwardSpeed())*.036f;
    const float RpmRatio=FMath::Clamp(Movement->GetEngineRotationSpeed()/FMath::Max(1.f,Movement->GetEngineMaxRotationSpeed()),0.f,1.f);
    EnginePitch=FMath::FInterpTo(EnginePitch,.72f+RpmRatio*1.5f,Dt,3.f);
    const float Master=FMath::Clamp(MasterVolume,0.f,1.f);
    EngineAudio->SetPitchMultiplier(EnginePitch);EngineAudio->SetVolumeMultiplier(Master*(.18f+.16f*Movement->GetThrottleInput()));
    RoadAudio->SetPitchMultiplier(.8f+Speed/70);RoadAudio->SetVolumeMultiplier(Master*FMath::Clamp(Speed/30,0.f,1.f)*.38f);
    CityAudio->SetVolumeMultiplier(Master*.45f);
    Cues.RemoveAll([](const auto& Cue){return !IsValid(Cue) || !Cue->IsPlaying();});
}
void URVVehicleAudioComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    SilenceTransientSounds();
    for (auto* Audio:{EngineAudio.Get(),RoadAudio.Get(),CityAudio.Get()}) if (Audio) Audio->Stop();
    Super::EndPlay(Reason);
}
