#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RVDrivingFeedbackComponent.h"
#include "RVVehicleAudioComponent.generated.h"

class UAudioComponent;
class USoundBase;

/** Runtime sound mix and rate-limited feedback cues. No device-specific haptics. */
UCLASS(ClassGroup=(RutaVirtual),meta=(BlueprintSpawnableComponent))
class RV_PIA_API URVVehicleAudioComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URVVehicleAudioComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Tick) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Audio",meta=(ClampMin="0",ClampMax="1")) float MasterVolume=.7f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Audio") bool bMuted=false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Audio") float EnginePitch=1.f;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Audio") int32 HornSoundsPlayed=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Audio") int32 FeedbackSoundsPlayed=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Audio") TObjectPtr<UAudioComponent> EngineAudio;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Audio") TObjectPtr<UAudioComponent> RoadAudio;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Audio") TObjectPtr<UAudioComponent> CityAudio;
    UFUNCTION(BlueprintCallable,Category="Audio") void ToggleMute();
    UFUNCTION(BlueprintCallable,Category="Audio") void SilenceTransientSounds();
    UFUNCTION() void PlayHorn(AActor* Vehicle,FName Reason);
private:
    UAudioComponent* CreateLoop(FName Name,USoundBase* Sound);
    void PlayCue(USoundBase* Sound,float Volume,AActor* SpatialSource=nullptr);
    UFUNCTION() void HandleFeedback(const FRVDrivingFeedback& Feedback);
    UPROPERTY() TObjectPtr<USoundBase> EngineSound;
    UPROPERTY() TObjectPtr<USoundBase> RoadSound;
    UPROPERTY() TObjectPtr<USoundBase> CitySound;
    UPROPERTY() TObjectPtr<USoundBase> HornSound;
    UPROPERTY() TObjectPtr<USoundBase> ImpactSound;
    UPROPERTY() TObjectPtr<USoundBase> RewardSound;
    UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Cues;
    TMap<TWeakObjectPtr<AActor>,float> LastHorns;
    float LastAnyHorn=-100,LastFeedbackSound=-100;
};
