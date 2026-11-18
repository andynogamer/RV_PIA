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
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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

void URVSessionMenuWidget::AddControlRow(UVerticalBox* Parent, const TCHAR* Control, const TCHAR* Action)
{
    auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    Parent->AddChildToVerticalBox(Row)->SetPadding(FMargin(0,0,0,7));
    auto* Size = WidgetTree->ConstructWidget<USizeBox>();
    Size->SetWidthOverride(157);
    Row->AddChildToHorizontalBox(Size);
    auto* Chip = WidgetTree->ConstructWidget<UBorder>();
    Chip->SetBrushColor(FLinearColor(.09f,.25f,.24f));
    Chip->SetPadding(FMargin(9,3));
    Size->AddChild(Chip);
    auto* Key = WidgetTree->ConstructWidget<UTextBlock>();
    Key->SetText(FText::FromString(Control));
    Key->SetFont(FCoreStyle::GetDefaultFontStyle("Bold",19));
    Key->SetJustification(ETextJustify::Center);
    Chip->AddChild(Key);
    auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
    Label->SetText(FText::FromString(Action));
    Label->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",19));
    auto* LabelSlot = Row->AddChildToHorizontalBox(Label);
    LabelSlot->SetPadding(FMargin(13,0,0,0));
    LabelSlot->SetVerticalAlignment(VAlign_Center);
}

void URVSessionMenuWidget::SwitchControlGuide()
{
    const bool ShowWheel = WheelGuide->GetVisibility()==ESlateVisibility::Collapsed;
    WheelGuide->SetVisibility(ShowWheel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    KeyboardGuide->SetVisibility(ShowWheel ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    ControlsBackButton->SetKeyboardFocus();
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
    CardSize->SetWidthOverride(660);CardSize->SetHeightOverride(bPauseMenu ? 590 : 625);
    auto* ContentSlot=Cast<UScaleBoxSlot>(Scale->AddChild(CardSize));
    ContentSlot->SetHorizontalAlignment(HAlign_Center);ContentSlot->SetVerticalAlignment(VAlign_Center);
    auto* Card=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Card->SetBrushColor(FLinearColor(.018f,.042f,.061f,.98f));Card->SetPadding(FMargin(36,28));CardSize->AddChild(Card);
    auto* Stack=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());Card->AddChild(Stack);
    const auto* RoadController = Cast<ARVPracticePlayerController>(GetOwningPlayer());
    const bool Highway = RoadController && RoadController->IsHighway();
    const bool Parking = RoadController && RoadController->IsParking();
    AddText(Stack,TEXT("Eyebrow"),Parking ? TEXT("ESTACIONAMIENTO / DOS MANIOBRAS") : Highway ? TEXT("CARRETERA / SIERRA DEL PINO") : bUrbanMenu ? TEXT("CIUDAD URBANA / PRÁCTICA GUIADA") : TEXT("PRIMEROS PASOS / CIRCUITO CORTO"),12,FLinearColor(.3f,.85f,.69f),6);
    AddText(Stack,TEXT("Brand"),TEXT("RUTA VIRTUAL"),38,FLinearColor(.95f,.98f,1),10);
    MainPanel=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("MainPanel"));Stack->AddChild(MainPanel);
    AddText(MainPanel,TEXT("Heading"),bPauseMenu ? TEXT("Tómate un respiro.") : TEXT("Tu recorrido empieza aquí."),26,FLinearColor::White,12);
    AddText(MainPanel,TEXT("Description"),bPauseMenu ? TEXT("La práctica y el tráfico están en pausa. Continúa cuando estés listo.") :
        Parking ? TEXT("Practica dos entradas entre líneas. Primero estaciona de frente en A1 y después de reversa en B2.") : TEXT("Observa el entorno, familiarízate con los controles y practica a tu ritmo. Comenzarás con el vehículo detenido."),17,FLinearColor(.65f,.76f,.81f),26);
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
    if (!bPauseMenu)
        AddButton(MainPanel,TEXT("RoadButton"),Highway ? TEXT("Cambiar a ciudad") : TEXT("Cambiar a carretera"))->OnClicked.AddDynamic(this,&URVSessionMenuWidget::HandleSwitchRoad);
    if (!bPauseMenu) AddButton(MainPanel,TEXT("ParkingButton"),Parking ? TEXT("Cambiar a ciudad") : TEXT("Practicar estacionamiento"))->OnClicked.AddDynamic(this,&URVSessionMenuWidget::HandleSwitchParking);
    if (!bPauseMenu) AddButton(MainPanel,TEXT("QuitButton"),TEXT("Salir del simulador"))->OnClicked.AddDynamic(this,&URVSessionMenuWidget::HandleQuit);
    ControlsPanel=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("ControlsPanel"));Stack->AddChild(ControlsPanel);
    WheelGuide=WidgetTree->ConstructWidget<UVerticalBox>(); ControlsPanel->AddChild(WheelGuide);
    AddText(WheelGuide,TEXT("WheelHeading"),TEXT("Volante y palanca"),27,FLinearColor::White,12);
    AddControlRow(WheelGuide,TEXT("Espacio"),TEXT("Pulsa y suelta el freno de mano"));
    AddControlRow(WheelGuide,TEXT("Embrague"),TEXT("Pisa a fondo para cambiar"));
    AddControlRow(WheelGuide,TEXT("1–5 · N · R"),TEXT("Mueve la palanca por la H"));
    AddControlRow(WheelGuide,TEXT("Reversa"),TEXT("Detente, pisa el embrague y mueve a 6"));
    AddControlRow(WheelGuide,TEXT("J / Enter"),TEXT("Calibra y registra cada posición"));
    AddControlRow(WheelGuide,TEXT("K"),TEXT("Alterna volante y teclado al parar"));
    AddControlRow(WheelGuide,TEXT("B / flechas"),TEXT("Elige y ajusta cada espejo al parar"));
    AddButton(WheelGuide,TEXT("ShowKeys"),TEXT("Ver teclas de conducción"))->OnClicked.AddDynamic(this,&URVSessionMenuWidget::SwitchControlGuide);
    KeyboardGuide=WidgetTree->ConstructWidget<UVerticalBox>(); ControlsPanel->AddChild(KeyboardGuide);
    AddText(KeyboardGuide,TEXT("KeysHeading"),TEXT("Teclado"),27,FLinearColor::White,12);
    AddControlRow(KeyboardGuide,TEXT("W / S"),TEXT("Acelerar / frenar"));
    AddControlRow(KeyboardGuide,TEXT("A / D"),TEXT("Girar a izquierda / derecha"));
    AddControlRow(KeyboardGuide,TEXT("Espacio / R"),TEXT("Freno de mano / avance o reversa"));
    AddControlRow(KeyboardGuide,TEXT("P · M / Inicio"),TEXT("Pausa / reiniciar"));
    AddControlRow(KeyboardGuide,TEXT("Tab / L"),TEXT("Cambiar cámara / luces"));
    AddControlRow(KeyboardGuide,TEXT("H / V"),TEXT("Claxon / silenciar sonido"));
    AddControlRow(KeyboardGuide,TEXT("G / B"),TEXT("Mostrar guía / ajustar espejos"));
    AddButton(KeyboardGuide,TEXT("ShowWheel"),TEXT("Ver volante y palanca"))->OnClicked.AddDynamic(this,&URVSessionMenuWidget::SwitchControlGuide);
    KeyboardGuide->SetVisibility(ESlateVisibility::Collapsed);
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
    CardSize->SetHeightOverride(bControlsVisible ? 585 : bPauseMenu ? 590 : 625);
    for (const FName Name : {FName(TEXT("Brand")),FName(TEXT("Eyebrow")),FName(TEXT("Footer"))})
        WidgetTree->FindWidget(Name)->SetVisibility(bControlsVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    MainPanel->SetVisibility(bControlsVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    ControlsPanel->SetVisibility(bControlsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (bControlsVisible) ControlsBackButton->SetKeyboardFocus();else FocusPrimaryButton();
}
void URVSessionMenuWidget::HandlePrimary() { if (auto* PC=Cast<ARVPracticePlayerController>(GetOwningPlayer())) PC->StartSession(); }
void URVSessionMenuWidget::HandleRestart() { if (auto* PC=Cast<ARVPracticePlayerController>(GetOwningPlayer())) PC->RestartSession(); }
void URVSessionMenuWidget::HandleHome() { if (auto* PC=Cast<ARVPracticePlayerController>(GetOwningPlayer())) PC->ShowStartMenu(); }
void URVSessionMenuWidget::HandleSwitchParking() { if (auto* PC=Cast<ARVPracticePlayerController>(GetOwningPlayer())) PC->SwitchParking(); }
void URVSessionMenuWidget::HandleSwitchRoad() { if (auto* PC=Cast<ARVPracticePlayerController>(GetOwningPlayer())) PC->SwitchRoad(); }
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
