#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "RVDrivingFeedbackComponent.generated.h"

class UPrimitiveComponent;
class URVDrivingAssistComponent;

UENUM(BlueprintType)
enum class ERVFeedbackKind : uint8 { Notice, Penalty, Reward };

USTRUCT(BlueprintType)
struct FRVDrivingFeedback
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName Code;
    UPROPERTY(BlueprintReadOnly) FString Message;
    UPROPERTY(BlueprintReadOnly) ERVFeedbackKind Kind=ERVFeedbackKind::Notice;
    UPROPERTY(BlueprintReadOnly) int32 ScoreDelta=0;
    /** Logical normalized request for a future validated backend, not motor output. */
    UPROPERTY(BlueprintReadOnly) float HapticRequest=0.f;
    UPROPERTY(BlueprintReadOnly) float HapticDurationSeconds=0.f;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRVDrivingFeedbackEvent,const FRVDrivingFeedback&,Feedback);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRVFeedbackStopped);

/** Explicit training stop line, independent of decorative signs elsewhere in the city. */
UCLASS()
class RV_PIA_API ARVStopRule : public AActor
{
    GENERATED_BODY()
public:
    ARVStopRule();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Training") float HalfWidthCm=200.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Training") float ApproachCm=350.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Training") float StopSeconds=1.f;
};

/** Educational practice assessment: no legal certification or clinical scoring. */
UCLASS(ClassGroup=(RutaVirtual),meta=(BlueprintSpawnableComponent))
class RV_PIA_API URVDrivingFeedbackComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    URVDrivingFeedbackComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Feedback") int32 SafetyScore=100;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Feedback") int32 PenaltyCount=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Feedback") int32 RewardCount=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Feedback") int32 CollisionCount=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Feedback") FRVDrivingFeedback LatestFeedback;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Feedback") float CleanDrivingRewardSeconds=20.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Feedback") float MinimumImpactKmh=2.5f;
    UPROPERTY(BlueprintAssignable,Category="Feedback") FRVDrivingFeedbackEvent OnFeedback;
    UPROPERTY(BlueprintAssignable,Category="Feedback") FRVFeedbackStopped OnFeedbackStopped;
    UFUNCTION(BlueprintCallable,Category="Feedback") void ResetSession();
    UFUNCTION(BlueprintCallable,Category="Feedback") void SuspendFeedback();
    UFUNCTION(BlueprintCallable,Category="Feedback") void RefreshCollisionNotifications();
    UFUNCTION(BlueprintCallable,Category="Feedback") void RegisterIncident(FName Code);
    UFUNCTION(BlueprintPure,Category="Feedback") FString GetSummary() const;
    UFUNCTION(BlueprintPure,Category="Feedback") bool HasVisibleFeedback() const;
private:
    void Emit(FName Code,const FString& Message,ERVFeedbackKind Kind,int32 Points,float HapticRequest);
    void Reward(FName Code,const FString& Message,int32 Points);
    UFUNCTION() void OnVehicleHit(UPrimitiveComponent* HitComponent,AActor* Other,UPrimitiveComponent* OtherComponent,FVector Impulse,const FHitResult& Hit);
    UPROPERTY() TObjectPtr<URVDrivingAssistComponent> Assist;
    TMap<FName,float> LastIncidents;
    TMap<TWeakObjectPtr<AActor>,float> LastImpactContacts;
    void RecordImpact(AActor* Other);
    TMap<TWeakObjectPtr<AActor>,float> StopTimers;
    TSet<TWeakObjectPtr<AActor>> PassedStops;
    TSet<TWeakObjectPtr<AActor>> RewardedStops;
    FVector PreviousPosition=FVector::ZeroVector;
    FVector PreviousVelocity=FVector::ZeroVector;
    bool bHasPrevious=false;
    float LatestTime=-100.f;
    float OverspeedSeconds=0.f,FollowingSeconds=0.f,WrongWaySeconds=0.f,CleanSeconds=0.f;
    int32 LastCheckpoint=0,LastStage=0;
};
