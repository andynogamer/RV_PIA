#pragma once
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "RVWheelButtonStyle.h"

namespace RVInstruments
{
const FLinearColor Panel(.012f, .027f, .038f, .96f);
const FLinearColor Surface(.041f, .09f, .113f);
const FLinearColor Ink(.95f, .98f, 1.f);
const FLinearColor Mint(.28f, .93f, .72f);
const FLinearColor Muted(.58f, .73f, .77f);
const FLinearColor Amber(1.f, .72f, .24f);

// All geometry uses the same logical units. Render type at the final pixel size.
class FGuidePainter
{
public:
    UCanvas& Canvas;
    float Scale;

    void Box(float X, float Y, float W, float H, FLinearColor Color) const
    {
        FCanvasTileItem Item(FVector2D(X*Scale,Y*Scale),FVector2D(W*Scale,H*Scale),Color);
        Item.BlendMode=SE_BLEND_Translucent;
        Canvas.DrawItem(Item);
    }
    void Line(float X, float Y, float U, float V, float Weight, FLinearColor Color) const
    {
        FCanvasLineItem Item(FVector2D(X*Scale,Y*Scale),FVector2D(U*Scale,V*Scale));
        Item.SetColor(Color);
        Item.LineThickness=Weight*Scale;
        Canvas.DrawItem(Item);
    }
    FSlateFontInfo Font(float Size, bool Bold) const
    {
        return FCoreStyle::GetDefaultFontStyle(Bold ? "Bold" : "Regular", FMath::Max(1, FMath::RoundToInt(Size*Scale)));
    }
    void Text(float X, float Y, const FString& Value, float Size, FLinearColor Color, bool Bold=true) const
    {
        FCanvasTextItem Item(FVector2D(X*Scale, Y*Scale), FText::FromString(Value), Font(Size, Bold), Color);
        // UE 5.6 Canvas checks the UFont even when Slate supplies the face and size.
        Item.Font = UEngine::GetLargeFont();
        Canvas.DrawItem(Item);
    }
    void Paragraph(float X, float Y, float Width, const FString& Value, float Size) const
    {
        const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        const auto Face = Font(Size, true);
        TArray<FString> Words;
        Value.ParseIntoArrayWS(Words);
        FString Row;
        for (const FString& Word : Words)
        {
            const FString Next = Row.IsEmpty() ? Word : Row + TEXT(" ") + Word;
            if (!Row.IsEmpty() && Measure->Measure(Next, Face).X > Width*Scale)
            {
                Text(X, Y, Row, Size, Ink);
                Y += Size*1.65f;
                Row = Word;
            }
            else Row = Next;
        }
        Text(X, Y, Row, Size, Ink);
    }
    void Key(float X, float Y, float W, const FString& Label) const
    {
        Box(X, Y+3, W, 33, Muted);
        Box(X, Y, W, 32, Ink);
        const float TextWidth = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label, Font(16,true)).X / Scale;
        Text(X+(W-TextWidth)*.5f, Y+2, Label, 16, Panel);
    }
    void Wheel(float X, float Y, int32 Turn) const
    {
        constexpr float Radius = 29;
        for (int32 I=0; I<32; ++I)
        {
            const float A = I*UE_TWO_PI/32, B = (I+1)*UE_TWO_PI/32;
            Line(X+Radius*FMath::Cos(A),Y+Radius*FMath::Sin(A),
                 X+Radius*FMath::Cos(B),Y+Radius*FMath::Sin(B),5,Mint);
        }
        Line(X-27,Y-8,X,Y+4,5,Ink);
        Line(X,Y+4,X+27,Y-8,5,Ink);
        Line(X,Y+4,X,Y+28,5,Ink);
        if (Turn)
        {
            const float Tip = X+Turn*40;
            Line(X,Y-41,Tip,Y-41,4,Amber);
            Line(Tip,Y-41,Tip-Turn*8,Y-47,4,Amber);
            Line(Tip,Y-41,Tip-Turn*8,Y-35,4,Amber);
        }
    }
    void WheelButton(float CenterX, float CenterY, TCHAR Letter, float Radius=21.f) const
    {
        const FVector2D Center(CenterX*Scale,CenterY*Scale);
        FCanvasNGonItem Rim(Center,FVector2D(Radius*Scale),32,GWhiteTexture,Muted);
        Canvas.DrawItem(Rim);
        FCanvasNGonItem Face(Center,FVector2D((Radius-2)*Scale),32,GWhiteTexture,Panel);
        Canvas.DrawItem(Face);
        const FString Glyph = FString::Chr(Letter);
        const float Size = Radius*.95f;
        const auto Extent = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Glyph,Font(Size,true))/Scale;
        Text(CenterX-Extent.X*.5f,CenterY-Extent.Y*.5f,Glyph,Size,RVWheelStyle::LetterColor(Letter));
    }
    void Pedals(float X, float Y, int32 Pressed) const
    {
        for (int32 I=0; I<3; ++I)
        {
            const bool Active = I==Pressed;
            const float Left = X+I*30;
            const float Top = Y+(Active ? 10 : 0);
            Line(Left+10,Y-10,Left+10,Top+11,5,Muted);
            Box(Left,Top,21,I==2 ? 40 : 33,Active ? Mint : Muted);
            for (int32 Groove=1; Groove<=3; ++Groove)
                Line(Left+4,Top+Groove*7,Left+16,Top+Groove*7,2,Panel);
            if (Active)
            {
                Line(Left+10,Top+45,Left+10,Top+61,3,Mint);
                Line(Left+10,Top+61,Left+4,Top+55,3,Mint);
                Line(Left+10,Top+61,Left+16,Top+55,3,Mint);
            }
        }
    }
    void Shifter(float X, float Y, int32 Gear) const
    {
        const float Mid = Y+42;
        Line(X+10,Mid,X+90,Mid,4,Muted);
        for (int32 Col=0; Col<3; ++Col)
        {
            const float Lane = X+10+Col*40;
            Line(Lane,Y+25,Lane,Y+60,4,Muted);
            Text(Lane-6,Y, FString::FromInt(1+2*Col),15,Ink);
            Text(Lane-6,Y+62,Col==2 ? TEXT("R") : FString::FromInt(2+2*Col),15,Ink);
        }
        const float Center = X+50;
        if (Gear==0) { Box(Center-6,Mid-6,12,12,Mint); return; }
        const int32 Slot = Gear<0 ? 6 : FMath::Clamp(Gear,1,6);
        const float Lane = X+10+((Slot-1)/2)*40;
        const float Tip = Slot%2 ? Y+24 : Y+61;
        const int32 Sign = Slot%2 ? -1 : 1;
        Line(Center,Mid,Lane,Mid,5,Mint);
        Line(Lane,Mid,Lane,Tip,5,Mint);
        Line(Lane,Tip,Lane-6,Tip-Sign*7,4,Mint);
        Line(Lane,Tip,Lane+6,Tip-Sign*7,4,Mint);
    }
};
}
