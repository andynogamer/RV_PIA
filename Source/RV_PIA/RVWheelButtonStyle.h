#pragma once
#include "CoreMinimal.h"

namespace RVWheelStyle
{
    inline FLinearColor LetterColor(TCHAR Letter)
    {
        switch (Letter)
        {
            case 'A': return FLinearColor::FromSRGBColor(FColor(106, 223, 89));
            case 'B': return FLinearColor::FromSRGBColor(FColor(255, 99, 92));
            case 'X': return FLinearColor::FromSRGBColor(FColor(75, 165, 255));
            case 'Y': return FLinearColor::FromSRGBColor(FColor(255, 209, 71));
            default: return FLinearColor::White;
        }
    }
}
