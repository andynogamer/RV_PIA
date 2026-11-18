#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RVSessionMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class USizeBox;

/** Desktop start/pause menu. Built with native UMG, without an editor-only dependency. */
UCLASS()
class RV_PIA_API URVSessionMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(bool bPaused, bool bUrban);
    void FocusPrimaryButton();
    UFUNCTION(BlueprintCallable, Category="Session") void ToggleControls();
    UFUNCTION(BlueprintPure, Category="Session") bool IsShowingControls() const { return bControlsVisible; }
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    void BuildMenu();
    UButton* AddButton(UVerticalBox* Parent,const FString& Name,const FString& Label,bool bPrimary=false);
    void AddControlRow(UVerticalBox* Parent, const TCHAR* Control, const TCHAR* Action);
    UFUNCTION() void SwitchControlGuide();
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> WheelGuide;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> KeyboardGuide;
    UTextBlock* AddText(UVerticalBox* Parent,const FString& Name,const FString& Text,int32 Size,FLinearColor Color,float Bottom=12);
    UFUNCTION() void HandlePrimary();
    UFUNCTION() void HandleRestart();
    UFUNCTION() void HandleHome();
    UFUNCTION() void HandleQuit();
    UFUNCTION() void HandleSwitchRoad();
    UFUNCTION() void HandleSwitchParking();
    UPROPERTY(Transient) TObjectPtr<UButton> PrimaryButton;
    UPROPERTY(Transient) TObjectPtr<UButton> ControlsBackButton;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> MainPanel;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> ControlsPanel;
    UPROPERTY(Transient) TObjectPtr<USizeBox> CardSize;
    bool bPauseMenu=false;
    bool bUrbanMenu=true;
    bool bControlsVisible=false;
};
