#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RVPracticePlayerController.generated.h"

UCLASS()
class RV_PIA_API ARVPracticePlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ARVPracticePlayerController();
    virtual void SetupInputComponent() override;
private:
    void ToggleReverse();
    void ResetPractice();
    void TogglePracticePause();
};
