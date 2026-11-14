#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RVPracticeHUD.generated.h"

UCLASS()
class RV_PIA_API ARVPracticeHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
