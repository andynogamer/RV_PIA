#include "RVPracticeHUD.h"
#include "RVDrivingAssistComponent.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void ARVPracticeHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner || !PlayerOwner->GetPawn()) return;
    const auto* Assist = PlayerOwner->GetPawn()->FindComponentByClass<URVDrivingAssistComponent>();
    if (!Assist) return;
    const float Width = FMath::Min(Canvas->ClipX - 32.f, 720.f);
    DrawRect(FLinearColor(0.012f, 0.022f, 0.028f, 0.88f), 16.f, 16.f, Width, 118.f);
    DrawText(TEXT("RUTA VIRTUAL  |  PRACTICA DE CONDUCCION"), FLinearColor(0.7f, 0.9f, 0.85f), 30.f, 25.f, nullptr, 1.05f);
    DrawText(TEXT("W acelerar  /  S frenar  /  A-D girar  /  R marcha"), FLinearColor::White, 30.f, 49.f, nullptr, 0.95f);
    DrawText(TEXT("Espacio freno de mano  /  P pausa  /  Home reiniciar  /  Tab camara"), FLinearColor::White, 30.f, 69.f, nullptr, 0.85f);
    DrawText(Assist->GetInstruction(), FLinearColor(1.f, 0.85f, 0.4f), 30.f, 91.f, nullptr, 0.95f);
    const FString State = FString::Printf(TEXT("%s  |  %.0f km/h  |  Objetivo: %.0f m"),
        Assist->bReverseMode ? TEXT("R") : TEXT("D"), Assist->GetSpeedKmh(), Assist->GetDistanceToGoalMeters());
    DrawText(State, FLinearColor::White, 30.f, 112.f, nullptr, 0.85f);
}
