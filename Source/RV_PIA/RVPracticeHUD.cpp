#include "RVPracticeHUD.h"
#include "RVDrivingAssistComponent.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "RVPracticePlayerController.h"
#include "RVDrivingFeedbackComponent.h"
#include "RVVehicleAudioComponent.h"

void ARVPracticeHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner || !PlayerOwner->GetPawn()) return;
    if (const auto* PC=Cast<ARVPracticePlayerController>(PlayerOwner))
        if (PC->SessionScreen!=ERVSessionScreen::Driving) return;
    const auto* Assist = PlayerOwner->GetPawn()->FindComponentByClass<URVDrivingAssistComponent>();
    if (!Assist) return;
    const float Width = FMath::Min(Canvas->ClipX - 32.f, 720.f);
    DrawRect(FLinearColor(0.012f, 0.022f, 0.028f, 0.88f), 16.f, 16.f, Width, 143.f);
    DrawText(Assist->bUrbanMode ? TEXT("RUTA VIRTUAL  |  CONDUCCION URBANA") : TEXT("RUTA VIRTUAL  |  PRACTICA DE CONDUCCION"), FLinearColor(0.7f, 0.9f, 0.85f), 30.f, 25.f, nullptr, 1.05f);
    DrawText(TEXT("W acelerar  /  S frenar  /  A-D girar  /  R marcha"), FLinearColor::White, 30.f, 49.f, nullptr, 0.95f);
    DrawText(TEXT("Espacio freno de mano  /  P-M menu  /  Home reiniciar  /  Tab camara"), FLinearColor::White, 30.f, 69.f, nullptr, 0.85f);
    DrawText(Assist->GetInstruction(), FLinearColor(1.f, 0.85f, 0.4f), 30.f, 91.f, nullptr, 0.95f);
    FString State = FString::Printf(TEXT("%s  |  %.0f km/h  |  Objetivo: %.0f m"),
        Assist->bReverseMode ? TEXT("R") : TEXT("D"), Assist->GetSpeedKmh(), Assist->GetDistanceToGoalMeters());
    if (Assist->bUrbanMode) State+=TEXT(" | ")+Assist->GetUrbanSummary();
    DrawText(State, FLinearColor::White, 30.f, 112.f, nullptr, 0.85f);
    if (const auto* Feedback=PlayerOwner->GetPawn()->FindComponentByClass<URVDrivingFeedbackComponent>())
    {
        const auto* Audio=PlayerOwner->GetPawn()->FindComponentByClass<URVVehicleAudioComponent>();
        DrawText(Feedback->GetSummary()+(Audio && Audio->bMuted ? TEXT(" | V: sonido OFF") : TEXT(" | H: claxon | V: silenciar")),
            FLinearColor(.55f,.9f,.8f),30,134,nullptr,.85f);
        if (Feedback->HasVisibleFeedback())
        {
            const bool Error=Feedback->LatestFeedback.Kind==ERVFeedbackKind::Penalty;
            const float Y=FMath::Max(170.f,Canvas->ClipY-92);
            const FLinearColor Color=Error ? FLinearColor(1,.64f,.35f) : FLinearColor(.3f,.95f,.7f);
            DrawRect(FLinearColor(.01f,.025f,.035f,.95f),16,Y,Width,72);
            FString Title=Error ? TEXT("ATENCION") : TEXT("BIEN HECHO");
            if (Feedback->LatestFeedback.ScoreDelta) Title+=FString::Printf(TEXT(" | %+d puntos"),Feedback->LatestFeedback.ScoreDelta);
            DrawText(Title,Color,30,Y+10,nullptr,1);
            DrawText(Feedback->LatestFeedback.Message,FLinearColor::White,30,Y+36,nullptr,.9f);
        }
    }
}
