#include "RVSessionMenuWidget.h"
#include "RVPracticePlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
#include "RVDrivingFeedbackComponent.h"

void URVSessionMenuWidget::Configure(bool bPaused,bool bUrban)
{
    bPauseMenu=bPaused;bUrbanMenu=bUrban;SetIsFocusable(true);
}

TSharedRef<SWidget> URVSessionMenuWidget::RebuildWidget()
{
    if (!WidgetTree) WidgetTree=NewObject<UWidgetTree>(this,TEXT("WidgetTree"));
    if (!WidgetTree->RootWidget) BuildMenu();
    return Super::RebuildWidget();
}

UTextBlock* URVSessionMenuWidget::AddText(UVerticalBox* Parent,const FString& Name,const FString& Text,int32 Size,FLinearColor Color,float Bottom)
{
    auto* Label=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),FName(*Name));
    Label->SetText(FText::FromString(Text));
    Label->SetFont(FCoreStyle::GetDefaultFontStyle(Size>=26 ? "Bold" : "Regular",Size));
    Label->SetColorAndOpacity(FSlateColor(Color));Label->SetAutoWrapText(true);
    Parent->AddChildToVerticalBox(Label)->SetPadding(FMargin(0,0,0,Bottom));
    return Label;
}

UButton* URVSessionMenuWidget::AddButton(UVerticalBox* Parent,const FString& Name,const FString& Label,bool bPrimary)
{
    auto* Button=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),FName(*Name));
    const FLinearColor Base=bPrimary ? FLinearColor(.17f,.76f,.58f) : FLinearColor(.065f,.11f,.15f);
    FButtonStyle Style;
    Style.SetNormal(FSlateColorBrush(Base)).SetHovered(FSlateColorBrush(bPrimary ? FLinearColor(.3f,.9f,.7f) : FLinearColor(.12f,.21f,.27f)))
        .SetPressed(FSlateColorBrush(FLinearColor(.1f,.4f,.33f))).SetNormalPadding(FMargin(20,13)).SetPressedPadding(FMargin(20,13));
    Button->SetStyle(Style); // UButton is keyboard-focusable by default in UE 5.6.
    auto* Text=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(FText::FromString(Label));Text->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",17));
    Text->SetColorAndOpacity(FSlateColor(bPrimary ? FLinearColor(.01f,.06f,.045f) : FLinearColor(.88f,.94f,.96f)));
    Text->SetJustification(ETextJustify::Center);Button->AddChild(Text);
    Parent->AddChildToVerticalBox(Button)->SetPadding(FMargin(0,0,0,9));
    return Button;
}

void URVSessionMenuWidget::BuildMenu()
{
    auto* Root=WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),TEXT("MenuRoot"));WidgetTree->RootWidget=Root;
    auto* Shade=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Shade->SetBrushColor(FLinearColor(.003f,.009f,.018f,.88f));
    auto* ShadeSlot=Root->AddChildToOverlay(Shade);
    ShadeSlot->SetHorizontalAlignment(HAlign_Fill);ShadeSlot->SetVerticalAlignment(VAlign_Fill);
    auto* Scale=WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
    Scale->SetStretch(EStretch::ScaleToFit);Scale->SetStretchDirection(EStretchDirection::Both);
    auto* ScaleSlot=Root->AddChildToOverlay(Scale);ScaleSlot->SetPadding(FMargin(20));
    ScaleSlot->SetHorizontalAlignment(HAlign_Fill);ScaleSlot->SetVerticalAlignment(VAlign_Fill);
    CardSize=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    CardSize->SetWidthOverride(660);CardSize->SetHeightOverride(bPauseMenu ? 590 : 530);
    auto* ContentSlot=Cast<UScaleBoxSlot>(Scale->AddChild(CardSize));
    ContentSlot->SetHorizontalAlignment(HAlign_Center);ContentSlot->SetVerticalAlignment(VAlign_Center);
    auto* Card=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Card->SetBrushColor(FLinearColor(.018f,.042f,.061f,.98f));Card->SetPadding(FMargin(36,28));CardSize->AddChild(Card);
    auto* Stack=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());Card->AddChild(Stack);
    AddText(Stack,TEXT("Eyebrow"),bUrbanMenu ? TEXT("CIUDAD URBANA / PRÁCTICA GUIADA") : TEXT("PRIMEROS PASOS / CIRCUITO CORTO"),12,FLinearColor(.3f,.85f,.69f),6);
    AddText(Stack,TEXT("Brand"),TEXT("RUTA VIRTUAL"),38,FLinearColor(.95f,.98f,1),10);
    MainPanel=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("MainPanel"));Stack->AddChild(MainPanel);
    AddText(MainPanel,TEXT("Heading"),bPauseMenu ? TEXT("Tómate un respiro.") : TEXT("Tu recorrido empieza aquí."),26,FLinearColor::White,12);
    AddText(MainPanel,TEXT("Description"),bPauseMenu ? TEXT("La práctica y el tráfico están en pausa. Continúa cuando estés listo.") :
        TEXT("Observa el entorno, familiarízate con los controles y practica a tu ritmo. Comenzarás con el vehículo detenido."),17,FLinearColor(.65f,.76f,.81f),26);
    if (bPauseMenu)
        if (const auto* PC=Cast<ARVPracticePlayerController>(GetOwningPlayer()))
            if (PC->GetPawn()) if (const auto* Feedback=PC->GetPawn()->FindComponentByClass<URVDrivingFeedbackComponent>())
                AddText(MainPanel,TEXT("SessionSummary"),Feedback->GetSummary(),15,FLinearColor(.3f,.85f,.69f),14);
    PrimaryButton=AddButton(MainPanel,TEXT("PrimaryButton"),bPauseMenu ? TEXT("Continuar práctica") : TEXT("Comenzar práctica"),true);
    PrimaryButton->OnClicked.AddDynamic(this,&URVSessionMenuWidget::HandlePrimary);
    if (bPauseMenu)
    {
        AddButton(MainPanel,TEXT("RestartButton"),TEXT("Reiniciar práctica"))->OnClicked.AddDynamic(this,&URVSessionMenuWidget::HandleRestart);
        AddButton(MainPanel,TEXT("HomeButton"),TEXT("Volver al inicio"))->OnClicked.AddDynamic(this,&URVSessionMenuWidget::HandleHome);
    }
    AddButton(MainPanel,TEXT("ControlsButton"),TEXT("Ver controles"))->OnClicked.AddDynamic(this,&URVSessionMenuWidget::ToggleControls);
    if (!bPauseMenu) AddButton(MainPanel,TEXT("QuitButton"),TEXT("Salir del simulador"))->OnClicked.AddDynamic(this,&URVSessionMenuWidget::HandleQuit);
    ControlsPanel=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("ControlsPanel"));Stack->AddChild(ControlsPanel);
    AddText(ControlsPanel,TEXT("ControlsHeading"),TEXT("Conoce los controles"),26,FLinearColor::White,18);
    AddText(ControlsPanel,TEXT("ControlsText"),TEXT("W / ↑  Acelerar     ·     S / ↓  Frenar\nA / D  Girar a izquierda / derecha\nR  Cambiar avance / reversa, estando detenido\nEspacio  Freno de mano (pulsa y suelta para salir)\nP / M  Abrir la pausa     ·     Inicio  Reiniciar\nTab  Cambiar cámara     ·     L  Luces\nH  Claxon     ·     V  Silenciar / restaurar sonido"),17,FLinearColor(.8f,.87f,.9f),20);
    AddText(ControlsPanel,TEXT("ControlsTip"),TEXT("Suelta el acelerador al reanudar. Observa antes de incorporarte, respeta las señales y deja distancia."),16,FLinearColor(.3f,.85f,.69f),20);
    ControlsBackButton=AddButton(ControlsPanel,TEXT("ControlsBackButton"),TEXT("Volver"),true);
    ControlsBackButton->OnClicked.AddDynamic(this,&URVSessionMenuWidget::ToggleControls);
    ControlsPanel->SetVisibility(ESlateVisibility::Collapsed);
    AddText(Stack,TEXT("Footer"),TEXT("Sin prisa. Observa, practica y vuelve a intentarlo."),13,FLinearColor(.42f,.57f,.64f),0);
}

void URVSessionMenuWidget::FocusPrimaryButton()
{
    if (PrimaryButton) { PrimaryButton->SetUserFocus(GetOwningPlayer());PrimaryButton->SetKeyboardFocus(); }
}
void URVSessionMenuWidget::ToggleControls()
{
    bControlsVisible=!bControlsVisible;
    CardSize->SetHeightOverride(bControlsVisible ? 650 : bPauseMenu ? 590 : 530);
    MainPanel->SetVisibility(bControlsVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    ControlsPanel->SetVisibility(bControlsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (bControlsVisible) ControlsBackButton->SetKeyboardFocus();else FocusPrimaryButton();
}
void URVSessionMenuWidget::HandlePrimary() { if (auto* PC=Cast<ARVPracticePlayerController>(GetOwningPlayer())) PC->StartSession(); }
void URVSessionMenuWidget::HandleRestart() { if (auto* PC=Cast<ARVPracticePlayerController>(GetOwningPlayer())) PC->RestartSession(); }
void URVSessionMenuWidget::HandleHome() { if (auto* PC=Cast<ARVPracticePlayerController>(GetOwningPlayer())) PC->ShowStartMenu(); }
void URVSessionMenuWidget::HandleQuit() { if (auto* PC=Cast<ARVPracticePlayerController>(GetOwningPlayer())) PC->QuitSession(); }

FReply URVSessionMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    const FKey Key=Event.GetKey();
    if (Key==EKeys::V && !Event.IsRepeat())
    {
        if (auto* PC=Cast<ARVPracticePlayerController>(GetOwningPlayer())) PC->ToggleSound();
        return FReply::Handled();
    }
    if (Key==EKeys::Enter && HasKeyboardFocus()) { HandlePrimary();return FReply::Handled(); }
    if (Key==EKeys::Escape || Key==EKeys::P || Key==EKeys::M)
    {
        if (bControlsVisible) ToggleControls();
        else if (bPauseMenu) HandlePrimary();
        return FReply::Handled();
    }
    if (Key==EKeys::Home && bPauseMenu) { HandleRestart();return FReply::Handled(); }
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
