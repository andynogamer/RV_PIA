#pragma once
#include "RVTrafficSystem.h"
#include "RVParkingScenario.generated.h"

class UTextRenderComponent;

/** The first bay is entered forwards and the second in reverse. Each bay's local +X points inwards. */
UCLASS()
class RV_PIA_API ARVParkingScenario : public ARVUrbanScenario
{
    GENERATED_BODY()
public:
    ARVParkingScenario();
    UPROPERTY(EditAnywhere, Category="Parking") TArray<FTransform> Bays;
    UPROPERTY(EditAnywhere, Category="Parking") FVector2D BayHalfSize = FVector2D(310,165);
    UPROPERTY(EditAnywhere, Category="Parking") FVector2D BodyMin = FVector2D(-235,-115);
    UPROPERTY(EditAnywhere, Category="Parking") FVector2D BodyMax = FVector2D(235,115);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Parking") int32 CompletedBays = 0;
    void ResetParking();
    void UpdateParking(const FTransform& Car, float SpeedKmh, bool bReverse, bool bHandbrake, float DeltaTime);
    FString Instruction() const;
    float DistanceMeters(const FVector& Position) const;
    bool Complete() const { return !Bays.IsValidIndex(CompletedBays); }
private:
    enum class EHint { Approach, WrongEntry, Straighten, Center, Brake, Hold };
    EHint Hint = EHint::Approach;
    TOptional<FVector> Previous;
    bool bCorrectEntry = false;
    float StoppedSeconds = 0.f;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Marker;
    void UpdateMarker();
};
