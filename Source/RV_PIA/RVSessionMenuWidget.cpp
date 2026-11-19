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
#include "Brushes/SlateRoundedBoxBrush.h"
#include "RVWheelButtonStyle.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
#include "RVDrivingFeedbackComponent.h"

namespace
{
    UTextBlock* LegendText(UWidgetTree* Tree, const FString& Value, int32 Size, bool bBold=false)
    {
        auto* Text = Tree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(Value));
        Text->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
        return Text;
    }

    USizeBox* WheelButton(UWidgetTree* Tree, TCHAR Letter)
    {
        auto* Size = Tree->ConstructWidget<USizeBox>();
        Size->SetWidthOverride(30); Size->SetHeightOverride(30);
        auto* Face = Tree->ConstructWidget<UBorder>();
        Face->SetBrush(FSlateRoundedBoxBrush(FLinearColor(.012f,.016f,.02f), 15.f, FLinearColor(.22f,.25f,.27f), 1.5f));
        Face->SetPadding(FMargin(0)); Face->SetHorizontalAlignment(HAlign_Center); Face->SetVerticalAlignment(VAlign_Center);
        auto* Glyph = LegendText(Tree, FString::Chr(Letter), 16, true);
        Glyph->SetColorAndOpacity(RVWheelStyle::LetterColor(Letter));
        Face->AddChild(Glyph); Size->AddChild(Face);
        return Size;
    }

    void AddLegend(UHorizontalBox* Row, UWidgetTree* Tree, TCHAR Letter, const TCHAR* Action)
    {
        if (Letter) Row->AddChildToHorizontalBox(WheelButton(Tree,Letter))->SetVerticalAlignment(VAlign_Center);
        auto* Slot = Row->AddChildToHorizontalBox(LegendText(Tree,Action,16));
        Slot->SetVerticalAlignment(VAlign_Center); Slot->SetPadding(FMargin(5,0,16,0));
    }
}

void URVSessionMenuWidget::Configure(bool bPaused,bool bUrban,bool bStereo)
{
    bPauseMenu=bPaused;bUrbanMenu=bUrban;bStereoMenu=bStereo;SetIsFocusable(true);
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
    if (bStereoMenu) Label->SetWrapTextAt(588.f);
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
    if (bStereoMenu) SelectMenuButton(0);
    else ControlsBackButton->SetKeyboardFocus();
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
    if (!bStereoMenu)
        AddText(MainPanel,TEXT("Heading"),bPauseMenu ? TEXT("Tómate un respiro.") : TEXT("Tu recorrido empieza aquí."),26,FLinearColor::White,12);
    AddText(MainPanel,TEXT("Description"),bStereoMenu ?
        (bPauseMenu ? TEXT("Mira hacia el volante físico y pulsa Y si necesitas centrar el asiento.") :
        TEXT("Siéntate frente al volante físico. Mira al frente y pulsa A en Comenzar práctica para centrar el asiento.")) :
        bPauseMenu ? TEXT("La práctica y el tráfico están en pausa. Continúa cuando estés listo.") :
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
    auto* FaceRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    WheelGuide->AddChildToVerticalBox(FaceRow)->SetPadding(FMargin(0,0,0,8));
    auto* Diamond = WidgetTree->ConstructWidget<UGridPanel>();
    auto* DiamondSlot = FaceRow->AddChildToHorizontalBox(Diamond);
    DiamondSlot->SetPadding(FMargin(32,0,32,0));
    Diamond->AddChildToGrid(WheelButton(WidgetTree,'Y'),0,1);
    Diamond->AddChildToGrid(WheelButton(WidgetTree,'X'),1,0);
    Diamond->AddChildToGrid(WheelButton(WidgetTree,'B'),1,2);
    Diamond->AddChildToGrid(WheelButton(WidgetTree,'A'),2,1);
    auto* Actions = WidgetTree->ConstructWidget<UVerticalBox>();
    FaceRow->AddChildToHorizontalBox(Actions)->SetVerticalAlignment(VAlign_Center);
    auto* Top = WidgetTree->ConstructWidget<UHorizontalBox>(); Actions->AddChild(Top);
    auto* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>(); Actions->AddChild(Bottom);
    AddLegend(Top,WidgetTree,'A',TEXT("Confirmar")); AddLegend(Top,WidgetTree,'B',TEXT("Volver"));
    AddLegend(Bottom,WidgetTree,'X',TEXT("Freno de mano")); AddLegend(Bottom,WidgetTree,'Y',TEXT("Centrar"));
    AddControlRow(WheelGuide,TEXT("↑ ↓ / Menú"),TEXT("Elegir / pausar la práctica"));
    AddControlRow(WheelGuide,TEXT("Embrague"),TEXT("Pisa a fondo para cambiar"));
    AddControlRow(WheelGuide,TEXT("1–5 · N · R"),TEXT("Mueve la palanca por la H"));
    AddControlRow(WheelGuide,TEXT("Reversa"),TEXT("Al parar, pisa el embrague y pon 6"));
    AddControlRow(WheelGuide,TEXT("Levas juntas"),TEXT("Elige espejo. Ajusta con la cruceta."));
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
    auto* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("Footer"));
    Stack->AddChild(Footer);
    AddLegend(Footer,WidgetTree,0,TEXT("↑ ↓   Elegir"));
    AddLegend(Footer,WidgetTree,'A',TEXT("Confirmar")); AddLegend(Footer,WidgetTree,'B',TEXT("Volver"));
    if (bStereoMenu && bPauseMenu) AddLegend(Footer,WidgetTree,'Y',TEXT("Centrar"));
    if (bStereoMenu) SelectMenuButton(0);
}

void URVSessionMenuWidget::SelectMenuButton(int32 Offset)
{
    TArray<UButton*> Buttons;
    UVerticalBox* Panel = MainPanel;
    if (bControlsVisible)
        Panel = WheelGuide->GetVisibility()==ESlateVisibility::Collapsed ? KeyboardGuide : WheelGuide;
    for (auto* Child : Panel->GetAllChildren())
        if (auto* Button = Cast<UButton>(Child)) Buttons.Add(Button);
    if (bControlsVisible) Buttons.Add(ControlsBackButton);
    if (Buttons.IsEmpty()) return;

    int32 Current = Buttons.IndexOfByKey(SelectedButton.Get());
    if (!bStereoMenu)
    {
        const int32 Focused = Buttons.IndexOfByPredicate([](UButton* Button){ return Button->HasKeyboardFocus(); });
        if (Focused!=INDEX_NONE) Current = Focused;
    }
    const int32 Next = (FMath::Max(0,Current)+Offset+Buttons.Num()) % Buttons.Num();
    SelectedButton = Buttons[Next];
    if (!bStereoMenu) SelectedButton->SetKeyboardFocus();
    for (auto* Button : Buttons)
    {
        const bool bSelected = Button==SelectedButton;
        FButtonStyle Style = Button->GetStyle();
        Style.SetNormal(FSlateColorBrush(bSelected ? FLinearColor(.17f,.76f,.58f) : FLinearColor(.065f,.11f,.15f)));
        Button->SetStyle(Style);
        CastChecked<UTextBlock>(Button->GetContent())->SetColorAndOpacity(
            FSlateColor(bSelected ? FLinearColor(.01f,.06f,.045f) : FLinearColor(.88f,.94f,.96f)));
    }
}

void URVSessionMenuWidget::ActivateMenuButton()
{
    if (!bStereoMenu) SelectMenuButton(0);
    if (SelectedButton) SelectedButton->OnClicked.Broadcast();
}

void URVSessionMenuWidget::ShowSeatStatus(bool bCentered)
{
    if (auto* Label = Cast<UTextBlock>(GetWidgetFromName(TEXT("Description"))))
        Label->SetText(FText::FromString(bCentered ? TEXT("Asiento centrado.") :
            TEXT("Ponte el visor y comprueba el límite de Meta. Cuando vuelva el seguimiento, mira al frente y pulsa A para empezar.")));
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
    if (bStereoMenu)
    {
        WidgetTree->FindWidget(TEXT("Footer"))->SetVisibility(ESlateVisibility::Visible);
        SelectedButton = nullptr;
        SelectMenuButton(0);
    }
    else if (bControlsVisible) ControlsBackButton->SetKeyboardFocus();else FocusPrimaryButton();
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
