#include "RVWheelInput.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <windows.gaming.input.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

struct FRVWheelInput::FSystemWheel
{
#if PLATFORM_WINDOWS
    using FWheel = ABI::Windows::Gaming::Input::IRacingWheel;
    Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IRacingWheelStatics> Wheels;
    Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IRawGameControllerStatics> Controllers;
    Microsoft::WRL::ComPtr<FWheel> Matched;

    FSystemWheel()
    {
        using Microsoft::WRL::Wrappers::HStringReference;
        RoGetActivationFactory(HStringReference(RuntimeClass_Windows_Gaming_Input_RacingWheel).Get(), IID_PPV_ARGS(&Wheels));
        RoGetActivationFactory(HStringReference(RuntimeClass_Windows_Gaming_Input_RawGameController).Get(), IID_PPV_ARGS(&Controllers));
    }

    bool Matches(FWheel* Wheel) const
    {
        using namespace ABI::Windows::Gaming::Input;
        Microsoft::WRL::ComPtr<IGameController> Controller;
        Microsoft::WRL::ComPtr<IRawGameController> Raw;
        UINT16 Vendor = 0, ProductId = 0;
        boolean HasClutch = false, HasShifter = false;
        INT32 MaxGear = 0;
        return SUCCEEDED(Wheel->QueryInterface(IID_PPV_ARGS(&Controller))) &&
            SUCCEEDED(Controllers->FromGameController(Controller.Get(), &Raw)) && Raw &&
            SUCCEEDED(Raw->get_HardwareVendorId(&Vendor)) && Vendor == 0x046d &&
            SUCCEEDED(Raw->get_HardwareProductId(&ProductId)) && ProductId == 0xc262 &&
            SUCCEEDED(Wheel->get_HasClutch(&HasClutch)) && HasClutch &&
            SUCCEEDED(Wheel->get_HasPatternShifter(&HasShifter)) && HasShifter &&
            SUCCEEDED(Wheel->get_MaxPatternShifterGear(&MaxGear)) && MaxGear >= 6;
    }

    FWheel* FindConnected()
    {
        Microsoft::WRL::ComPtr<__FIVectorView_1_Windows__CGaming__CInput__CRacingWheel> Connected;
        UINT32 Count = 0;
        if (Wheels && Controllers && SUCCEEDED(Wheels->get_RacingWheels(&Connected)) && SUCCEEDED(Connected->get_Size(&Count)))
        {
            for (UINT32 Index = 0; Index < Count; ++Index)
            {
                Microsoft::WRL::ComPtr<FWheel> Candidate;
                if (SUCCEEDED(Connected->GetAt(Index, &Candidate)) &&
                    (Candidate.Get() == Matched.Get() || Matches(Candidate.Get())))
                {
                    Matched = Candidate;
                    return Matched.Get();
                }
            }
        }
        Matched.Reset();
        return nullptr;
    }
#endif
};

bool FRVWheelInput::ReadAutomatic(void* Window, FRVWheelSample& Sample)
{
    bHasReading = false;
    bAutomaticLayout = false;
#if PLATFORM_WINDOWS
    if (!Window || GetAncestor(static_cast<HWND>(Window), GA_ROOT) != GetForegroundWindow()) return false;
    if (!SystemWheel) SystemWheel = MakeUnique<FSystemWheel>();
    auto* Wheel = SystemWheel->FindConnected();
    if (!Wheel) return false;
    bAutomaticLayout = true;
    ABI::Windows::Gaming::Input::RacingWheelReading Reading{};
    if (FAILED(Wheel->GetCurrentReading(&Reading)) || Reading.Timestamp == 0 ||
        Reading.PatternShifterGear < -1 || Reading.PatternShifterGear > 6) return false;
    Sample = {float(Reading.Wheel), float(Reading.Throttle), float(Reading.Brake),
              float(Reading.Clutch), Reading.PatternShifterGear};
    using namespace ABI::Windows::Gaming::Input;
    const auto Down = [&](RacingWheelButtons Button) { return (Reading.Buttons & Button) != 0; };
    Sample.bMirrorSelect = Down(RacingWheelButtons_PreviousGear) && Down(RacingWheelButtons_NextGear);
    Sample.MirrorPan = int8(Down(RacingWheelButtons_DPadRight)) - int8(Down(RacingWheelButtons_DPadLeft));
    Sample.MirrorTilt = int8(Down(RacingWheelButtons_DPadUp)) - int8(Down(RacingWheelButtons_DPadDown));
    bHasReading = true;
#endif
    return bHasReading;
}

namespace
{
    const TCHAR* ProfileSection = TEXT("RutaVirtual.G920");
    const TCHAR* Steps[] = {
        TEXT("Centra el volante, suelta los pedales y pon la palanca en N."),
        TEXT("Mantén el volante en el extremo izquierdo."),
        TEXT("Mantén el volante en el extremo derecho."),
        TEXT("Centra el volante y pisa a fondo el acelerador."),
        TEXT("Suelta el acelerador y pisa a fondo el freno."),
        TEXT("Suelta el freno y pisa a fondo el embrague."),
        TEXT("Suelta los pedales y selecciona 1ª."),
        TEXT("Selecciona 2ª."),
        TEXT("Selecciona 3ª."),
        TEXT("Selecciona 4ª."),
        TEXT("Selecciona 5ª."),
        TEXT("Selecciona la posición 6 (reversa sin hundir)."),
        TEXT("Empuja la palanca hacia abajo y selecciona R."),
        TEXT("Vuelve a neutral y suelta los pedales.")
    };

    float NormalizeAxis(int32 Value, int32 Rest, int32 End)
    {
        const float Position = float(Value - Rest) / float(End - Rest);
        return FMath::GetMappedRangeValueClamped(FVector2D(0.035, 0.985), FVector2D(0, 1), Position);
    }

#if PLATFORM_WINDOWS
    BOOL CALLBACK FindG920(const DIDEVICEINSTANCEW* Instance, void* Context)
    {
        if (LOWORD(Instance->guidProduct.Data1) == 0x046d &&
            (FCString::Stristr(Instance->tszProductName, TEXT("G920")) ||
             FCString::Stristr(Instance->tszInstanceName, TEXT("G920"))))
        {
            *static_cast<DIDEVICEINSTANCEW*>(Context) = *Instance;
            return DIENUM_STOP;
        }
        return DIENUM_CONTINUE;
    }
#endif
}

bool FRVWheelInput::FProfile::IsValid() const
{
    if (Axes.Num() != 4 || Rest.Num() != 4 || Ends.Num() != 5 || Gears.Num() != 7) return false;
    TSet<int32> SeenAxes;
    TSet<int32> SeenButtons;
    for (int32 Index = 0; Index < Axes.Num(); ++Index)
    {
        if (Axes[Index] < 0 || Axes[Index] >= AxisCount || SeenAxes.Contains(Axes[Index]) ||
            Rest[Index] < 0 || Rest[Index] > 65535 || Ends[Index] < 0 || Ends[Index] > 65535 ||
            FMath::Abs(Ends[Index] - Rest[Index]) < 1024) return false;
        SeenAxes.Add(Axes[Index]);
    }
    if (Ends[4] < 0 || Ends[4] > 65535 || FMath::Abs(Ends[4] - Rest[0]) < 1024 ||
        (Ends[0] > Rest[0]) == (Ends[4] > Rest[0])) return false;
    for (int32 Button : Gears)
    {
        if (Button < 0 || Button >= 128 || SeenButtons.Contains(Button)) return false;
        SeenButtons.Add(Button);
    }
    return true;
}

FRVWheelInput::FRVWheelInput()
{
    if (!GConfig) return;
    FString ProductString;
    if (GConfig->GetString(ProfileSection, TEXT("Product"), ProductString, GGameUserSettingsIni))
        LexTryParseString(CalibratedProduct, *ProductString);
    auto Load = [](const TCHAR* Key, TArray<int32>& Values)
    {
        TArray<FString> Strings;
        GConfig->GetArray(ProfileSection, Key, Strings, GGameUserSettingsIni);
        for (const FString& String : Strings)
        {
            int32 Value = INDEX_NONE;
            if (!LexTryParseString(Value, *String)) { Values.Reset(); return; }
            Values.Add(Value);
        }
    };
    Load(TEXT("Axes"), Profile.Axes);
    Load(TEXT("Rest"), Profile.Rest);
    Load(TEXT("Ends"), Profile.Ends);
    Load(TEXT("Gears"), Profile.Gears);
}

FRVWheelInput::~FRVWheelInput()
{
#if PLATFORM_WINDOWS
    if (Device) static_cast<IDirectInputDevice8W*>(Device)->Release();
    if (DirectInput) static_cast<IDirectInput8W*>(DirectInput)->Release();
#endif
}

bool FRVWheelInput::IsCalibrated() const
{
    return CalibratedProduct != 0 && Profile.IsValid();
}

bool FRVWheelInput::Poll(void* Window, FRawSample& Sample)
{
#if PLATFORM_WINDOWS
    bHasReading = false;
    if (!Window) return false;
    if (Device && DeviceWindow != Window)
    {
        static_cast<IDirectInputDevice8W*>(Device)->Release();
        Device = nullptr;
    }
    if (!Device)
    {
        const double Now = FPlatformTime::Seconds();
        if (Now < NextScan) return false;
        NextScan = Now + 1.0;
        if (!DirectInput)
        {
            IDirectInput8W* Api = nullptr;
            if (FAILED(DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W,
                                         reinterpret_cast<void**>(&Api), nullptr))) return false;
            DirectInput = Api;
        }
        auto* Api = static_cast<IDirectInput8W*>(DirectInput);
        DIDEVICEINSTANCEW Instance{};
        Instance.dwSize = sizeof(Instance);
        if (FAILED(Api->EnumDevices(DI8DEVCLASS_GAMECTRL, FindG920, &Instance, DIEDFL_ATTACHEDONLY)) ||
            !Instance.guidProduct.Data1) return false;

        IDirectInputDevice8W* Wheel = nullptr;
        if (FAILED(Api->CreateDevice(Instance.guidInstance, &Wheel, nullptr))) return false;
        if (FAILED(Wheel->SetDataFormat(&c_dfDIJoystick2)) ||
            FAILED(Wheel->SetCooperativeLevel(static_cast<HWND>(Window), DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)))
        {
            Wheel->Release();
            return false;
        }
        Wheel->Acquire();
        Product = Instance.guidProduct.Data1;
        DeviceWindow = Window;
        Device = Wheel;
    }

    auto* Wheel = static_cast<IDirectInputDevice8W*>(Device);
    DIJOYSTATE2 State{};
    const HRESULT PollResult = Wheel->Poll();
    if (FAILED(PollResult) || FAILED(Wheel->GetDeviceState(sizeof(State), &State)))
    {
        // Release a lost instance so a replug can be discovered by enumeration.
        Wheel->Release();
        Device = nullptr;
        return false;
    }
    Sample.Axes = {State.lX, State.lY, State.lZ, State.lRx, State.lRy, State.lRz,
                   State.rglSlider[0], State.rglSlider[1]};
    Sample.Buttons.Init(false, UE_ARRAY_COUNT(State.rgbButtons));
    for (int32 Index = 0; Index < Sample.Buttons.Num(); ++Index)
        Sample.Buttons[Index] = (State.rgbButtons[Index] & 0x80) != 0;
    bHasReading = true;
    return true;
#else
    return false;
#endif
}

bool FRVWheelInput::Read(void* Window, FRVWheelSample& Sample)
{
#if WITH_EDITOR
    static const bool bOfflineTest = FParse::Param(FCommandLine::Get(), TEXT("RVNoWheelInput"));
    if (bOfflineTest) return false;
#endif
    FRawSample Raw;
    if (!IsCalibrated())
    {
        if (ReadAutomatic(Window, Sample)) return true;
        if (bAutomaticLayout) return false; // Recognized, waiting for the first real input report.
        const double Now = FPlatformTime::Seconds();
        if (Now >= NextUncalibratedProbe)
        {
            NextUncalibratedProbe = Now + 1.0;
            Poll(Window, Raw);
        }
        return false;
    }
    if (!Poll(Window, Raw) || CalibratedProduct != Product) return false;
    for (int32 Axis : Profile.Axes)
        if (Raw.Axes[Axis] < 0 || Raw.Axes[Axis] > 65535) return false;
    for (int32 Index = 0; Index < Profile.Gears.Num(); ++Index)
    {
        if (!Raw.Buttons[Profile.Gears[Index]]) continue;
        if (Sample.Gear != 0) return false;
        Sample.Gear = Index < 6 ? Index + 1 : -1;
    }
    const auto ReadAxis = [&](int32 Index)
    {
        return NormalizeAxis(Raw.Axes[Profile.Axes[Index]], Profile.Rest[Index], Profile.Ends[Index]);
    };
    Sample.Steering = NormalizeAxis(Raw.Axes[Profile.Axes[0]], Profile.Rest[0], Profile.Ends[4]) - ReadAxis(0);
    Sample.Throttle = ReadAxis(1);
    Sample.Brake = ReadAxis(2);
    Sample.Clutch = ReadAxis(3);
    return true;
}

void FRVWheelInput::StartCalibration()
{
    Captured = {};
    CapturedProduct = 0;
    CalibrationStep = 0;
    CalibrationError.Reset();
}

void FRVWheelInput::CancelCalibration()
{
    CalibrationStep = INDEX_NONE;
    CalibrationError.Reset();
}

FString FRVWheelInput::GetPrompt() const
{
    if (!CalibrationError.IsEmpty()) return CalibrationError;
    if (!IsCalibrating()) return {};
    return Steps[CalibrationStep];
}

void FRVWheelInput::CaptureStep(void* Window)
{
    if (!IsCalibrating()) return;
    CalibrationError.Reset();
    FRawSample Raw;
    if (!Poll(Window, Raw))
    {
        CalibrationError = TEXT("G920 no disponible. Revisa G HUB y joy.cpl. Enter reintenta.");
        return;
    }
    if (CalibrationStep > 0 && CapturedProduct != Product)
    {
        StartCalibration();
        CalibrationError = TEXT("Cambió el dispositivo. Centra el volante, pon N y pulsa Enter.");
        return;
    }

    const int32 Button = Raw.Buttons.Find(true);
    if (CalibrationStep == 0)
    {
        if (Button != INDEX_NONE)
        {
            CalibrationError = TEXT("Pon la palanca en N y suelta botones. Pulsa Enter.");
            return;
        }
        Neutral = Raw;
        CapturedProduct = Product;
    }
    else if (CalibrationStep <= 5)
    {
        int32 Axis = INDEX_NONE;
        int32 Largest = 1023;
        int32 NextLargest = 0;
        for (int32 Index = 0; Index < AxisCount; ++Index)
        {
            const int32 Value = Raw.Axes[Index];
            const int32 Rest = Neutral.Axes[Index];
            if (Value < 0 || Value > 65535 || Rest < 0 || Rest > 65535) continue;
            const int32 Travel = FMath::Abs(Value - Rest);
            if (Travel > Largest) { NextLargest = Axis == INDEX_NONE ? NextLargest : Largest; Largest = Travel; Axis = Index; }
            else { NextLargest = FMath::Max(NextLargest, Travel); }
        }
        const bool bRight = CalibrationStep == 2;
        if (Axis == INDEX_NONE || NextLargest * 3 > Largest ||
            (bRight && (Axis != Captured.Axes[0] ||
             (Captured.Ends[0] > Neutral.Axes[Axis]) == (Raw.Axes[Axis] > Neutral.Axes[Axis]))) ||
            (!bRight && Captured.Axes.Contains(Axis)))
        {
            CalibrationError = FString(TEXT("Mueve solo el control indicado y repite. ")) + Steps[CalibrationStep];
            return;
        }
        if (bRight) Captured.Ends[4] = Raw.Axes[Axis];
        else
        {
            if (Captured.Ends.IsEmpty()) Captured.Ends.SetNumZeroed(5);
            Captured.Ends[Captured.Axes.Num()] = Raw.Axes[Axis];
            Captured.Axes.Add(Axis);
            Captured.Rest.Add(Neutral.Axes[Axis]);
        }
    }
    else if (CalibrationStep <= 12)
    {
        if (Button == INDEX_NONE || Raw.Buttons.CountSetBits() != 1 || Captured.Gears.Contains(Button))
        {
            CalibrationError = FString(TEXT("Se necesita una posición distinta. ")) + Steps[CalibrationStep];
            return;
        }
        Captured.Gears.Add(Button);
    }
    else
    {
        if (Button != INDEX_NONE || !Captured.IsValid() ||
            NormalizeAxis(Raw.Axes[Captured.Axes[1]], Captured.Rest[1], Captured.Ends[1]) > 0.f)
        {
            CalibrationError = TEXT("Vuelve a N, suelta el acelerador y pulsa Enter.");
            return;
        }
        CalibratedProduct = Product;
        Profile = MoveTemp(Captured);
        SaveProfile();
        CancelCalibration();
        return;
    }
    ++CalibrationStep;
}

void FRVWheelInput::SaveProfile() const
{
    GConfig->SetString(ProfileSection, TEXT("Product"), *LexToString(CalibratedProduct), GGameUserSettingsIni);
    auto Save = [](const TCHAR* Key, const TArray<int32>& Values)
    {
        TArray<FString> Strings;
        for (int32 Value : Values) Strings.Add(FString::FromInt(Value));
        GConfig->SetArray(ProfileSection, Key, Strings, GGameUserSettingsIni);
    };
    Save(TEXT("Axes"), Profile.Axes);
    Save(TEXT("Rest"), Profile.Rest);
    Save(TEXT("Ends"), Profile.Ends);
    Save(TEXT("Gears"), Profile.Gears);
    GConfig->Flush(false, GGameUserSettingsIni);
}
