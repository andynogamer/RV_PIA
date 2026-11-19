#pragma once

#include "CoreMinimal.h"

enum class ERVWheelButton : uint8 { None=0, Up=1, Down=2, Accept=4, Back=8, Menu=16, Handbrake=32, Center=64 };
ENUM_CLASS_FLAGS(ERVWheelButton);

struct FRVWheelSample
{
    float Steering = 0.f;
    float Throttle = 0.f;
    float Brake = 0.f;
    float Clutch = 0.f;
    int32 Gear = 0;
    bool bMirrorSelect = false;
    int8 MirrorPan = 0;
    int8 MirrorTilt = 0;
};

class FRVWheelInput
{
public:
    FRVWheelInput();
    ~FRVWheelInput();
    FRVWheelInput(const FRVWheelInput&) = delete;
    FRVWheelInput& operator=(const FRVWheelInput&) = delete;

    bool IsCalibrated() const;
    bool Read(void* Window, FRVWheelSample& Sample);
    TOptional<ERVWheelButton> ReadButtons(void* Window);
    void PlayCollisionPulse(void* Window);
    void StopForces();
    void UpdateCentering(void* Window,float Steering);
    void StartCalibration();
    void CancelCalibration();
    void CaptureStep(void* Window);
    bool IsCalibrating() const { return CalibrationStep != INDEX_NONE; }
    int32 GetCalibrationStep() const { return CalibrationStep; }
    bool HasReading() const { return bHasReading; }
    bool HasLayout() const { return bAutomaticLayout || IsCalibrated(); }
    bool IsAutomaticWheelPresent() const { return bAutomaticLayout; }
    FString GetPrompt() const;

private:
    static constexpr int32 AxisCount = 8;
    struct FRawSample
    {
        TArray<int32, TInlineAllocator<AxisCount>> Axes;
        TBitArray<> Buttons;
    };
    struct FProfile
    {
        TArray<int32> Axes;
        TArray<int32> Rest;
        TArray<int32> Ends;
        TArray<int32> Gears;
        bool IsValid() const;
    };
    bool Poll(void* Window, FRawSample& Sample);
    bool ReadAutomatic(void* Window, FRVWheelSample& Sample);
    struct FSystemWheel;
    TUniquePtr<FSystemWheel> SystemWheel;
    bool bAutomaticLayout = false;
    void SaveProfile() const;
    void* DirectInput = nullptr;
    void* Device = nullptr;
    void* DeviceWindow = nullptr;
    double NextScan = 0.0;
    double NextUncalibratedProbe = 0.0;
    bool bHasReading = false;
    uint32 Product = 0;
    uint32 CalibratedProduct = 0;
    uint32 CapturedProduct = 0;
    int32 CalibrationStep = INDEX_NONE;
    FRawSample Neutral;
    FProfile Profile;
    FProfile Captured;
    FString CalibrationError;
};
