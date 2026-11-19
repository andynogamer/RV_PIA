#include "RVWheelInput.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeLock.h"
#include "Misc/App.h"
#include "HAL/IConsoleManager.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <windows.gaming.input.h>
#include <windows.gaming.input.forcefeedback.h>
#include <wrl/client.h>
#include <wrl/event.h>
#include <wrl/wrappers/corewrappers.h>
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"

namespace
{
    namespace FF = ABI::Windows::Gaming::Input::ForceFeedback;
    using Microsoft::WRL::ComPtr;
    using FLoadResult = FF::ForceFeedbackLoadEffectResult;
    using FLoadOperation = ABI::Windows::Foundation::IAsyncOperation<FLoadResult>;
    static TAutoConsoleVariable<float> CollisionGain(TEXT("rv.G920.CollisionGain"), .15f,
        TEXT("Normalized collision pulse gain, 0 disables, maximum 0.25."));

    static TAutoConsoleVariable<float> CenteringGain(TEXT("rv.G920.CenteringGain"), .25f,
        TEXT("Spring strength while driving. Zero disables centering. Maximum 0.35."));

    // Each effect has independent loading, gain and lifetime. A completed load never starts a motor.
    struct FWheelEffect
    {
        ComPtr<FF::IForceFeedbackMotor> Motor;
        ComPtr<FF::IForceFeedbackEffect> Effect;
        TAtomic<bool> bLoaded{false};
        bool bStarted = false;

        static TSharedPtr<FWheelEffect, ESPMode::ThreadSafe> Load(FF::IForceFeedbackMotor* Motor,
            const ComPtr<FF::IForceFeedbackEffect>& Effect, const TCHAR* Label)
        {
            if (!Effect || FAILED(Effect->put_Gain(0.0))) return {};
            auto State = MakeShared<FWheelEffect, ESPMode::ThreadSafe>();
            State->Motor = Motor;
            State->Effect = Effect;
            ComPtr<FLoadOperation> Op;
            if (FAILED(Motor->LoadEffectAsync(Effect.Get(), &Op))) return {};
            auto Done = Microsoft::WRL::Callback<ABI::Windows::Foundation::IAsyncOperationCompletedHandler<FLoadResult>>(
                [State,Label](FLoadOperation* Operation, AsyncStatus Status) -> HRESULT
                {
                    FLoadResult Result{};
                    const bool bReady = Status == AsyncStatus::Completed && SUCCEEDED(Operation->GetResults(&Result)) &&
                        Result == FF::ForceFeedbackLoadEffectResult_Succeeded;
                    State->bLoaded.Store(bReady);
                    UE_LOG(LogTemp,Display,TEXT("G920: %s %s"),Label,bReady ? TEXT("cargado") : TEXT("no disponible"));
                    return S_OK;
                });
            if (FAILED(Op->put_Completed(Done.Get()))) return {};
            return State;
        }

        void Start(float Gain)
        {
            boolean Enabled = false, Paused = true;
            if (bLoaded.Load() && Gain > 0.f && SUCCEEDED(Motor->get_IsEnabled(&Enabled)) && Enabled &&
                SUCCEEDED(Motor->get_AreEffectsPaused(&Paused)) && !Paused && SUCCEEDED(Effect->put_Gain(Gain)))
                bStarted = SUCCEEDED(Effect->Start());
        }
        void Stop()
        {
            if (bStarted) { Effect->Stop(); bStarted = false; }
        }
        ~FWheelEffect()
        {
            Stop();
            if (bLoaded.Load())
            {
                ComPtr<__FIAsyncOperation_1_boolean> Unload;
                Motor->TryUnloadEffectAsync(Effect.Get(), &Unload);
            }
        }
    };

    struct FWheelForces
    {
        TSharedPtr<FWheelEffect, ESPMode::ThreadSafe> Impact;
        TSharedPtr<FWheelEffect, ESPMode::ThreadSafe> Spring;
        FCriticalSection SpringLock;
        PTP_TIMER Watchdog = nullptr;
        double LastDrivingSample = 0.;
        double NextStartAttempt = 0.;
        float AppliedGain = 0.f;

        explicit FWheelForces(ABI::Windows::Gaming::Input::IRacingWheel* Wheel)
        {
            ComPtr<FF::IForceFeedbackMotor> Motor;
            FF::ForceFeedbackEffectAxes Axes{};
            if (FAILED(Wheel->get_WheelMotor(&Motor)) || !Motor || FAILED(Motor->get_SupportedAxes(&Axes)) ||
                !(Axes & FF::ForceFeedbackEffectAxes_X)) return;
            using Microsoft::WRL::Wrappers::HStringReference;
            ComPtr<FF::IPeriodicForceEffectFactory> WaveFactory;
            ComPtr<FF::IForceFeedbackEffect> WaveEffect;
            ComPtr<FF::IPeriodicForceEffect> Wave;
            if (SUCCEEDED(RoGetActivationFactory(HStringReference(RuntimeClass_Windows_Gaming_Input_ForceFeedback_PeriodicForceEffect).Get(), IID_PPV_ARGS(&WaveFactory))) &&
                SUCCEEDED(WaveFactory->CreateInstance(FF::PeriodicForceEffectKind_SineWave, &WaveEffect)) &&
                SUCCEEDED(WaveEffect.As(&Wave)) && SUCCEEDED(Wave->SetParameters({1.f,0.f,0.f},20.f,0.f,0.f,{150*ETimespan::TicksPerMillisecond})))
                Impact = FWheelEffect::Load(Motor.Get(),WaveEffect,TEXT("efecto de choque"));

            Watchdog = CreateThreadpoolTimer(&CheckDrivingSample,this,nullptr);
            if (!Watchdog) return;
            ComPtr<FF::IConditionForceEffectFactory> SpringFactory;
            ComPtr<FF::IForceFeedbackEffect> SpringEffect;
            ComPtr<FF::IConditionForceEffect> Condition;
            if (SUCCEEDED(RoGetActivationFactory(HStringReference(RuntimeClass_Windows_Gaming_Input_ForceFeedback_ConditionForceEffect).Get(), IID_PPV_ARGS(&SpringFactory))) &&
                SUCCEEDED(SpringFactory->CreateInstance(FF::ConditionForceEffectKind_Spring,&SpringEffect)) &&
                SUCCEEDED(SpringEffect.As(&Condition)) && SUCCEEDED(Condition->SetParameters({1.f,0.f,0.f},1.f,1.f,1.f,1.f,.025f,0.f)))
                Spring = FWheelEffect::Load(Motor.Get(),SpringEffect,TEXT("resorte de centrado"));
        }
        ~FWheelForces()
        {
            if (Watchdog)
            {
                SetThreadpoolTimer(Watchdog,nullptr,0,0);
                WaitForThreadpoolTimerCallbacks(Watchdog,true);
                CloseThreadpoolTimer(Watchdog);
                Watchdog = nullptr;
            }
            Stop();
        }
        static void CALLBACK CheckDrivingSample(PTP_CALLBACK_INSTANCE, void* Context, PTP_TIMER)
        {
            auto& Self = *static_cast<FWheelForces*>(Context);
            FScopeLock Guard(&Self.SpringLock);
            if (Self.Spring && Self.Spring->bStarted && FPlatformTime::Seconds()-Self.LastDrivingSample >= .30)
            {
                const HRESULT Apartment = RoInitialize(RO_INIT_MULTITHREADED);
                Self.Spring->Stop();
                SetThreadpoolTimer(Self.Watchdog,nullptr,0,0);
                if (SUCCEEDED(Apartment)) RoUninitialize();
            }
        }
        void UpdateSpring(float Steering)
        {
            if (!Spring || !Spring->bLoaded.Load()) return;
            const float Gain = FMath::Clamp(CenteringGain.GetValueOnGameThread(),0.f,.35f);
            FScopeLock Guard(&SpringLock);
            LastDrivingSample = FPlatformTime::Seconds();
            if (Gain <= 0.f)
            {
                if (Spring->bStarted) { Spring->Stop(); SetThreadpoolTimer(Watchdog,nullptr,0,0); }
                return;
            }
            if (Spring->bStarted)
            {
                if (Gain != AppliedGain && SUCCEEDED(Spring->Effect->put_Gain(Gain))) AppliedGain = Gain;
                return;
            }
            // Do not pull a turned wheel when resuming or reconnecting.
            if (!FMath::IsNearlyZero(Steering,.08f) || LastDrivingSample < NextStartAttempt) return;
            NextStartAttempt = LastDrivingSample + .5;
            Spring->Start(Gain);
            if (Spring->bStarted)
            {
                AppliedGain = Gain;
                LARGE_INTEGER Delay;
                Delay.QuadPart = -100 * ETimespan::TicksPerMillisecond;
                FILETIME Due{Delay.LowPart,static_cast<DWORD>(Delay.HighPart)};
                SetThreadpoolTimer(Watchdog,&Due,50,0);
                UE_LOG(LogTemp,Display,TEXT("G920: centrado activo (%.0f%%)"),Gain*100.f);
            }
        }
        void Stop()
        {
            if (Impact) Impact->Stop();
            FScopeLock Guard(&SpringLock);
            if (Spring && Spring->bStarted)
            {
                Spring->Stop();
                if (Watchdog) SetThreadpoolTimer(Watchdog,nullptr,0,0);
                UE_LOG(LogTemp,Display,TEXT("G920: centrado detenido"));
            }
        }
        void PlayImpact()
        {
            if (!Impact) return;
            Impact->Stop();
            Impact->Start(FMath::Clamp(CollisionGain.GetValueOnGameThread(),0.f,.25f));
            if (Impact->bStarted) UE_LOG(LogTemp,Display,TEXT("G920: pulso por choque, 150 ms"));
        }
    };
    bool WheelHasInputFocus(void* Window)
    {
        if (!Window) return false;
        if (FApp::UseVRFocus()) return FApp::HasVRFocus();
        return GetAncestor(static_cast<HWND>(Window), GA_ROOT)==GetForegroundWindow();
    }
}
#endif

struct FRVWheelInput::FSystemWheel
{
#if PLATFORM_WINDOWS
    using FWheel = ABI::Windows::Gaming::Input::IRacingWheel;
    Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IRacingWheelStatics> Wheels;
    Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IRawGameControllerStatics> Controllers;
    Microsoft::WRL::ComPtr<FWheel> Matched;
    Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IUINavigationControllerStatics2> NavigationFactory;
    Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IUINavigationController> Navigation;
    bool bDrivingLayout = false;
    TUniquePtr<FWheelForces> Forces;

    FSystemWheel()
    {
        using Microsoft::WRL::Wrappers::HStringReference;
        RoGetActivationFactory(HStringReference(RuntimeClass_Windows_Gaming_Input_RacingWheel).Get(), IID_PPV_ARGS(&Wheels));
        RoGetActivationFactory(HStringReference(RuntimeClass_Windows_Gaming_Input_RawGameController).Get(), IID_PPV_ARGS(&Controllers));
        RoGetActivationFactory(HStringReference(RuntimeClass_Windows_Gaming_Input_UINavigationController).Get(), IID_PPV_ARGS(&NavigationFactory));
    }

    bool Matches(FWheel* Wheel) const
    {
        using namespace ABI::Windows::Gaming::Input;
        Microsoft::WRL::ComPtr<IGameController> Controller;
        Microsoft::WRL::ComPtr<IRawGameController> Raw;
        UINT16 Vendor = 0, ProductId = 0;
        return SUCCEEDED(Wheel->QueryInterface(IID_PPV_ARGS(&Controller))) &&
            SUCCEEDED(Controllers->FromGameController(Controller.Get(), &Raw)) && Raw &&
            SUCCEEDED(Raw->get_HardwareVendorId(&Vendor)) && Vendor == 0x046d &&
            SUCCEEDED(Raw->get_HardwareProductId(&ProductId)) && ProductId == 0xc262;
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
                    if (Candidate.Get() != Matched.Get())
                    {
                        Matched = Candidate;
                        Forces.Reset();
                        Navigation.Reset();
                        Microsoft::WRL::ComPtr<ABI::Windows::Gaming::Input::IGameController> Controller;
                        if (NavigationFactory && SUCCEEDED(Candidate.As(&Controller)))
                            NavigationFactory->FromGameController(Controller.Get(), &Navigation);
                        boolean Clutch = false, Shifter = false;
                        INT32 MaxGear = 0;
                        bDrivingLayout = SUCCEEDED(Candidate->get_HasClutch(&Clutch)) && Clutch &&
                            SUCCEEDED(Candidate->get_HasPatternShifter(&Shifter)) && Shifter &&
                            SUCCEEDED(Candidate->get_MaxPatternShifterGear(&MaxGear)) && MaxGear >= 6;
                    }
                    return Matched.Get();
                }
            }
        }
        Matched.Reset();
        Forces.Reset();
        Navigation.Reset();
        return nullptr;
    }
#endif
};

void FRVWheelInput::StopForces()
{
#if PLATFORM_WINDOWS
    if (SystemWheel && SystemWheel->Forces) SystemWheel->Forces->Stop();
#endif
}

void FRVWheelInput::UpdateCentering(void* Window,float Steering)
{
#if PLATFORM_WINDOWS
    if (!SystemWheel || !WheelHasInputFocus(Window) || !SystemWheel->Matched) { StopForces(); return; }
    if (!SystemWheel->Forces) SystemWheel->Forces = MakeUnique<FWheelForces>(SystemWheel->Matched.Get());
    SystemWheel->Forces->UpdateSpring(Steering);
#endif
}

void FRVWheelInput::PlayCollisionPulse(void* Window)
{
#if PLATFORM_WINDOWS
    if (SystemWheel && WheelHasInputFocus(Window) && SystemWheel->FindConnected() && SystemWheel->Forces)
        SystemWheel->Forces->PlayImpact();
#endif
}

TOptional<ERVWheelButton> FRVWheelInput::ReadButtons(void* Window)
{
#if PLATFORM_WINDOWS
    static const bool bIsolated = FParse::Param(FCommandLine::Get(), TEXT("RVNoWheelInput"));
    if (bIsolated || !WheelHasInputFocus(Window)) { StopForces(); return {}; }
    if (!SystemWheel) SystemWheel = MakeUnique<FSystemWheel>();
    if (!SystemWheel->FindConnected() || !SystemWheel->Navigation) return {};
    using namespace ABI::Windows::Gaming::Input;
    UINavigationReading Reading{};
    if (FAILED(SystemWheel->Navigation->GetCurrentReading(&Reading))) return {};
    ERVWheelButton Buttons = ERVWheelButton::None;
    if (Reading.RequiredButtons & RequiredUINavigationButtons_Up) Buttons |= ERVWheelButton::Up;
    if (Reading.RequiredButtons & RequiredUINavigationButtons_Down) Buttons |= ERVWheelButton::Down;
    if (Reading.RequiredButtons & RequiredUINavigationButtons_Accept) Buttons |= ERVWheelButton::Accept;
    if (Reading.RequiredButtons & RequiredUINavigationButtons_Cancel) Buttons |= ERVWheelButton::Back;
    if (Reading.RequiredButtons & RequiredUINavigationButtons_Menu) Buttons |= ERVWheelButton::Menu;
    if (Reading.OptionalButtons & OptionalUINavigationButtons_Context1) Buttons |= ERVWheelButton::Handbrake;
    if (Reading.OptionalButtons & OptionalUINavigationButtons_Context2) Buttons |= ERVWheelButton::Center;
    return Buttons;
#else
    return {};
#endif
}

bool FRVWheelInput::ReadAutomatic(void* Window, FRVWheelSample& Sample)
{
    bHasReading = false;
    bAutomaticLayout = false;
#if PLATFORM_WINDOWS
    if (!WheelHasInputFocus(Window)) return false;
    if (!SystemWheel) SystemWheel = MakeUnique<FSystemWheel>();
    auto* Wheel = SystemWheel->FindConnected();
    if (!Wheel || !SystemWheel->bDrivingLayout) return false;
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
    static const bool bOfflineTest = FParse::Param(FCommandLine::Get(), TEXT("RVNoWheelInput"));
    if (bOfflineTest) return false;
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
