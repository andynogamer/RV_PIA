#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RVPracticePlayerController.generated.h"

class URVSessionMenuWidget;
class URVDrivingAssistComponent;

UENUM(BlueprintType)
enum class ERVSessionScreen : uint8 { Start, Driving, Paused };

UCLASS()
class RV_PIA_API ARVPracticePlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ARVPracticePlayerController();
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Session") ERVSessionScreen SessionScreen = ERVSessionScreen::Start;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Session") TObjectPtr<URVSessionMenuWidget> SessionMenu;
    UFUNCTION(BlueprintCallable, Category="Session") void ShowStartMenu();
    UFUNCTION(BlueprintCallable, Category="Session") void StartSession();
    UFUNCTION(BlueprintCallable, Category="Session") void ShowPauseMenu();
    UFUNCTION(BlueprintCallable, Category="Session") void RestartSession();
    UFUNCTION(BlueprintCallable, Category="Session") void QuitSession();
    UFUNCTION(BlueprintCallable, Category="Session") void SwitchRoad();
    bool IsHighway() const;
    bool IsParking() const;
    UFUNCTION(BlueprintCallable, Category="Session") void SwitchParking();
    UFUNCTION(BlueprintCallable, Category="Session") void ToggleSound();
private:
    URVDrivingAssistComponent* GetAssist() const;
    void OpenMenu(bool bPauseMenu);
    void CaptureDrivingInput();
    bool bAutoHideGuide = true;
    bool bSessionInitialized = false;
    int32 PendingMenuFocusFrames = 0;
    void ToggleReverse();
    void ToggleWheelCalibration();
    void CaptureWheelCalibrationStep();
    void ToggleWheelMode();
    void ResetPractice();
    void TogglePracticePause();
    void RequestHorn();
    void CycleMirror();
    void ToggleGuide();
public:
    bool bShowDrivingGuide = true;
};
