#include "RVPracticeHUD.h"
#include "RVCabinComponent.h"
#include "RVDrivingAssistComponent.h"
#include "RVDrivingFeedbackComponent.h"
#include "RVPracticePlayerController.h"
#include "RVInstrumentPainter.h"

using namespace RVInstruments;

void ARVPracticeHUD::DrawHUD()
{
    const auto* PC = Cast<ARVPracticePlayerController>(PlayerOwner);
    if (PC && PC->UsesStereoViewport()) return;
    Super::DrawHUD();
    if (!Canvas || !PC || !PC->GetPawn() || PC->SessionScreen != ERVSessionScreen::Driving ||
        Canvas->ClipX < 100 || Canvas->ClipY < 100) return;
    const auto* Assist = PC->GetPawn()->FindComponentByClass<URVDrivingAssistComponent>();
    if (!Assist) return;
    const float Scale = FMath::Min(Canvas->ClipX/960.f,Canvas->ClipY/520.f);
    const float Width = Canvas->ClipX/Scale, Height = Canvas->ClipY/Scale;
    const FGuidePainter G{*Canvas,Scale};
    const int32 Step = Assist->GetCalibrationStep();
    const bool Calibrating = Step != INDEX_NONE;
    const bool Wheel = Assist->bWheelMode;
    const FString PhysicalGear = Assist->GetPhysicalShifterLabel();
    const auto* Feedback = PC->GetPawn()->FindComponentByClass<URVDrivingFeedbackComponent>();

    if (const auto* Cabin=PC->GetPawn()->FindComponentByClass<URVCabinComponent>(); Cabin && Cabin->GetSelectedMirror()!=INDEX_NONE)
    {
        G.Box(16,Height-86,510,71,Panel);
        G.Text(28,Height-83,FString(TEXT("Ajustar retrovisor "))+Cabin->MirrorName(),20,Mint);
        G.Text(28,Height-48,TEXT("Flechas  Ajustar     ·     B  Siguiente     ·     Enter  Listo"),15,Ink);
        return;
    }
    if (Assist->bUrbanMode)
    {
        G.Box(Width-150,70,134,32,Panel);
        G.Text(Width-141,72,FString::Printf(TEXT("Límite  %.0f"),Assist->GetPostedSpeedKmh()),18,Ink);
    }
    if (Assist->IsParkingPractice() && !PC->bShowDrivingGuide && !Calibrating)
    {
        G.Box(16,Height-66,725,54,Panel);
        G.Text(28,Height-59,Assist->GetInstruction(),20,Mint);
    }
    if (!PC->bShowDrivingGuide && !Calibrating && !(Feedback && Feedback->HasVisibleFeedback()))
    {
        if (Assist->IsParkingPractice()) return;
        G.Box(16,Height-42,250,32,Panel);
        G.Text(27,Height-39,TEXT("G  Guía      B  Espejos      P  Pausa"),13,Ink);
        return;
    }
    G.Box(16,14,320,65,Panel);
    G.Box(16,14,5,65,Mint);
    G.Text(32,21,Assist->GetGearLabel(),29,Mint);
    G.Text(82,21,FString::Printf(TEXT("%.0f km/h"),Assist->GetSpeedKmh()),22,Ink);
    G.Text(201,27,FString::Printf(TEXT("%.0f m"),Assist->GetDistanceToGoalMeters()),18,Muted);
    G.Box(Width-157,14,141,49,Panel);
    G.Key(Width-147,20,38,TEXT("P"));
    G.Text(Width-98,23,TEXT("Pausa"),17,Ink);

    if (Assist->bUrbanMode && !Calibrating)
    {
        G.Box(16,82,475,32,Panel);
        G.Text(26,85,Assist->GetUrbanSummary(),14,Ink);
    }
    if (!PhysicalGear.IsEmpty())
    {
        G.Box(695,14,99,65,Panel);
        G.Text(703,15,TEXT("Palanca"),14,Ink);
        G.Text(703,36,PhysicalGear,21,Mint);
        const auto& Input = Assist->GetLastWheelSample();
        const TCHAR* Names[] = {TEXT("Embrague"), TEXT("Freno"), TEXT("Acelerador")};
        const float Values[] = {Input.Clutch, Input.Brake, Input.Throttle};
        G.Box(342,14,348,65,Panel);
        for (int32 I=0; I<3; ++I)
        {
            const float X = 354+I*112;
            G.Text(X,23,Names[I],14,Ink);
            G.Box(X,54,98,8,Surface);
            G.Box(X,54,98*FMath::Clamp(Values[I],0.f,1.f),8,Mint);
        }
    }
    if (Feedback && !Calibrating)
    {
        const float X = Assist->bUrbanMode ? 498.f : 16.f;
        G.Box(X,82,195,32,Panel);
        G.Text(X+10,85,FString::Printf(TEXT("Seguridad  %d / 100"),Feedback->SafetyScore),14,Mint);
    }
    if (Feedback && Feedback->HasVisibleFeedback() && !Calibrating)
    {
        const bool Penalty = Feedback->LatestFeedback.Kind==ERVFeedbackKind::Penalty;
        G.Box(16,120,550,94,Panel);
        const FString Title = FString::Printf(TEXT("%s   %+d"), Penalty ? TEXT("Atención") : TEXT("Bien hecho"),
                                               Feedback->LatestFeedback.ScoreDelta);
        G.Text(28,123,Title,16,Penalty ? Amber : Mint);
        G.Paragraph(28,149,526,Feedback->LatestFeedback.Message,16);
    }

    if (Assist->bUrbanMode)
    {
        G.Box(Width-150,70,134,32,Panel);
        G.Text(Width-141,72,FString::Printf(TEXT("Límite  %.0f"),Assist->GetPostedSpeedKmh()),18,Ink);
    }
    if (!PC->bShowDrivingGuide && !Calibrating) return;
    const float CardWidth = FMath::Min(780.f, Width-160.f);
    const float Y = Height-164;
    const float ArtX = 26, CopyX = 172;
    G.Box(16,Y,CardWidth,150,Panel);
    G.Box(ArtX,Y+10,124,130,Surface);
    FString Title = Assist->GetInstruction();
    FString Caption = Wheel ? TEXT("G920   /   CONDUCCIÓN") : TEXT("TECLADO   /   CONDUCCIÓN");
    FString Key, Hint;
    if (Calibrating)
    {
        Caption = FString::Printf(TEXT("CALIBRAR G920     %d / 14"),Step+1);
        Key = TEXT("Enter");
        Hint = TEXT("Registrar posición     ·     J  Cancelar");
        if (Step==1 || Step==2) G.Wheel(ArtX+62,Y+82,Step==1 ? -1 : 1);
        else if (Step>=3 && Step<=5)
        {
            const int32 PedalOrder[] = {2,1,0};
            G.Pedals(ArtX+21,Y+48,PedalOrder[Step-3]);
        }
        else G.Shifter(ArtX+8,Y+30,Step>=6 && Step<=11 ? Step-5 : Step==12 ? -1 : 0);
    }
    else if (Assist->NeedsWheelProfile() && Assist->HasWheelReading())
    {
        G.Wheel(ArtX+62,Y+78,0);
        Title = TEXT("Configura tu volante y tus pedales.");
        Key = TEXT("J"); Hint = TEXT("Calibrar G920");
    }
    else if (Assist->IsWaitingForWheelReport())
    {
        G.Shifter(ArtX+8,Y+30,0);
        Title = TEXT("G920 detectado. Mueve un mando para empezar.");
        Hint = TEXT("Esperando datos del volante");
    }
    else if (Assist->NeedsWheelNeutral())
    {
        G.Shifter(ArtX+8,Y+30,0);
        Key = TEXT("K"); Hint = TEXT("Usar teclado estando detenido");
    }
    else if (Assist->IsWaitingForClutch())
    {
        G.Pedals(ArtX+21,Y+48,0);
        Hint = TEXT("Mantén la palanca en la marcha elegida.");
    }
    else if (Assist->bUrbanMode && Assist->PracticeStage==0 && !Assist->NeedsWheelNeutral() && (!Wheel || Assist->SelectedGear==0))
    {
        if (Wheel) G.WheelButton(ArtX+62,Y+74,'X');
        else G.Key(ArtX+9,Y+56,106,TEXT("Espacio"));
        Title = Wheel ? TEXT("Pulsa X en el G920 para soltar el freno de mano.") : TEXT("Pulsa y suelta el freno de mano.");
        Hint = Wheel ? TEXT("Después, pisa el embrague y pon primera.") : TEXT("Después, acelera con W.");
    }
    else if (Assist->IsParkingPractice())
    {
        const bool ReverseBay = Assist->PracticeStage==2;
        Caption = Assist->GetUrbanSummary();
        if (Assist->PracticeStage==3)
        {
            G.Line(ArtX+25,Y+80,ArtX+50,Y+102,7,Mint);
            G.Line(ArtX+50,Y+102,ArtX+94,Y+46,7,Mint);
            Key = TEXT("Home"); Hint = TEXT("Repetir las dos maniobras");
        }
        else
        {
            // The open end of the painted bay is at the bottom of the diagram.
            G.Line(ArtX+30,Y+78,ArtX+30,Y+25,4,Ink);
            G.Line(ArtX+30,Y+25,ArtX+92,Y+25,4,Ink);
            G.Line(ArtX+92,Y+25,ArtX+92,Y+78,4,Ink);
            G.Line(ArtX+61,Y+64,ArtX+61,Y+40,4,Amber);
            G.Line(ArtX+61,Y+40,ArtX+52,Y+50,4,Amber);
            G.Line(ArtX+61,Y+40,ArtX+70,Y+50,4,Amber);
            G.Box(ArtX+45,Y+77,32,51,Mint);
            G.Box(ArtX+48,Y+(ReverseBay ? 105 : 86),26,9,Panel);
            Hint = ReverseBay ? (Wheel ? TEXT("Posición 6 para reversa") : TEXT("R para seleccionar reversa")) : TEXT("Entra entre las marcas amarillas");
        }
    }
    else if (Wheel)
    {
        const bool Launch = Assist->SelectedGear==0 && Assist->GetSpeedKmh()<.5f && !Assist->HasDrivingNotice();
        G.Shifter(ArtX+8,Y+30,Launch ? 1 : Assist->SelectedGear);
        if (Launch) Title = TEXT("Pisa el embrague a fondo y pon primera.");
        Hint = TEXT("G  Ocultar guía     ·     B  Ajustar espejos");
    }
    else
    {
        G.Key(ArtX+43,Y+38,38,TEXT("W"));
        G.Key(ArtX+3,Y+76,37,TEXT("A"));
        G.Key(ArtX+43,Y+76,38,TEXT("S"));
        G.Key(ArtX+84,Y+76,37,TEXT("D"));
        Key = TEXT("J"); Hint = TEXT("Calibrar G920");
    }
    G.Text(CopyX,Y+8,Caption,14,Mint);
    G.Paragraph(CopyX,Y+34,CardWidth-CopyX,Title,21);
    if (!Key.IsEmpty()) G.Key(CopyX,Y+104,72,Key);
    G.Text(CopyX+(Key.IsEmpty() ? 0 : 85),Y+106,Hint,15,Muted,false);
}
