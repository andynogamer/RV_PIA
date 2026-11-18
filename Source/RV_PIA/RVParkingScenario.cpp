#include "RVParkingScenario.h"
#include "Components/TextRenderComponent.h"

ARVParkingScenario::ARVParkingScenario()
{
    bSpawnTraffic = false;
    PostedSpeedKmh = 10.f;
    Marker = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ActiveBay"));
    Marker->SetupAttachment(RootComponent);
    Marker->SetHorizontalAlignment(EHTA_Center);
    Marker->SetVerticalAlignment(EVRTA_TextCenter);
    Marker->SetWorldSize(55.f);
    Marker->SetTextRenderColor(FColor(117,255,177));
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARVParkingScenario::ResetParking()
{
    CompletedBays = 0;
    StoppedSeconds = 0.f;
    bCorrectEntry = false;
    Previous.Reset();
    Hint = EHint::Approach;
    UpdateMarker();
}

void ARVParkingScenario::UpdateMarker()
{
    Marker->SetVisibility(!Complete());
    if (Complete()) return;
    const FTransform& Bay = Bays[CompletedBays];
    Marker->SetWorldLocation(Bay.TransformPosition(FVector(BayHalfSize.X+45,0,230)));
    Marker->SetWorldRotation(Bay.Rotator()+FRotator(0,180,0));
    Marker->SetText(FText::FromString(CompletedBays==0 ? TEXT("A1  ▼") : TEXT("B2  ▼")));
}

float ARVParkingScenario::DistanceMeters(const FVector& Position) const
{
    return Complete() ? 0.f : .01f * FVector::Dist2D(Bays[CompletedBays].GetLocation(),Position);
}

void ARVParkingScenario::UpdateParking(const FTransform& Car, float SpeedKmh, bool bReverse, bool bHandbrake, float DeltaTime)
{
    if (Complete()) return;
    const FTransform& Bay = Bays[CompletedBays];
    const FVector Local = Bay.InverseTransformPosition(Car.GetLocation());
    const bool bNeedsReverse = CompletedBays == 1;
    const float Heading = FVector::DotProduct(Bay.GetUnitAxis(EAxis::X),Car.GetUnitAxis(EAxis::X)) * (bNeedsReverse ? -1.f : 1.f);
    const double Mouth = -BayHalfSize.X;
    if (Previous.IsSet() && Previous->X < Mouth && Local.X >= Mouth)
    {
        const double Fraction = (Mouth-Previous->X)/(Local.X-Previous->X);
        bCorrectEntry = FMath::Abs(FMath::Lerp(Previous->Y,Local.Y,Fraction)) < BayHalfSize.Y &&
            Heading > .45f && bReverse == bNeedsReverse && FVector::Dist2D(*Previous,Local) < 250.f;
    }
    Previous = Local;
    if (Local.X < Mouth || Local.X > BayHalfSize.X || FMath::Abs(Local.Y) > BayHalfSize.Y)
    {
        Hint = EHint::Approach;
        bCorrectEntry = false;
    }
    else if (!bCorrectEntry) Hint = EHint::WrongEntry;
    else if (Heading < FMath::Cos(FMath::DegreesToRadians(10.f))) Hint = EHint::Straighten;
    else
    {
        bool bInside = true;
        for (double X : {BodyMin.X,BodyMax.X})
            for (double Y : {BodyMin.Y,BodyMax.Y})
            {
                const FVector Corner = Bay.InverseTransformPosition(Car.TransformPosition(FVector(X,Y,0)));
                bInside &= FMath::Abs(Corner.X) <= BayHalfSize.X && FMath::Abs(Corner.Y) <= BayHalfSize.Y;
            }
        Hint = !bInside ? EHint::Center : (SpeedKmh >= .5f || !bHandbrake) ? EHint::Brake : EHint::Hold;
    }
    StoppedSeconds = Hint == EHint::Hold ? StoppedSeconds+DeltaTime : 0.f;
    if (StoppedSeconds < 1.5f) return;
    ++CompletedBays;
    Previous.Reset();
    bCorrectEntry = false;
    StoppedSeconds = 0.f;
    Hint = EHint::Approach;
    UpdateMarker();
}

FString ARVParkingScenario::Instruction() const
{
    if (Complete()) return TEXT("Ambas maniobras completadas. Pulsa Home para repetir.");
    switch (Hint)
    {
        case EHint::WrongEntry: return TEXT("Sal al pasillo y vuelve a entrar por la boca del cajón.");
        case EHint::Straighten: return TEXT("Endereza el coche siguiendo las líneas del cajón.");
        case EHint::Center: return TEXT("Centra el coche dentro de las líneas.");
        case EHint::Brake: return TEXT("Detente y mantén Espacio para aplicar el freno de mano.");
        case EHint::Hold: return TEXT("Mantén el freno de mano hasta confirmar.");
        default: return CompletedBays==0 ? TEXT("Entra de frente al cajón A1.") : TEXT("Estaciona de reversa en B2. Usa los espejos.");
    }
}
