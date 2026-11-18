#include "RVCabinComponent.h"
#include "RVDrivingAssistComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "RVInstrumentPainter.h"
#include "RVPracticePlayerController.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    struct FMirrorMount { const TCHAR* Key; const TCHAR* Label; FVector Position, RearDirection; float Width, Height; };
    // Local vehicle coordinates in cm. Neutral aim bisects the eye and the desired rearward ray.
    const FMirrorMount Mounts[] = {
        {TEXT("Izquierdo"),TEXT("izquierdo"),{12,-108,96},{-1,-.25,.015},19.5f,11.8f},
        {TEXT("Central"),TEXT("central"),{8,0,126},{-1,0,-.035},25.f,7.8f},
        {TEXT("Derecho"),TEXT("derecho"),{12,108,96},{-1,.25,.015},19.5f,11.8f}
    };
    const TCHAR* SettingsSection = TEXT("RutaVirtual.Retrovisores");
}

URVCabinComponent::URVCabinComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Center(TEXT("/Game/RutaVirtual/Vehicles/Interior/Detalle/SM_EspejoCentral"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Side(TEXT("/Game/RutaVirtual/Vehicles/Interior/Detalle/SM_EspejoLateral"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Trim(TEXT("/Game/RutaVirtual/Vehicles/Interior/Detalle/SM_DetalleCabina"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> GearLever(TEXT("/Game/RutaVirtual/Vehicles/Interior/Detalle/SM_PalancaCabina"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Surface(TEXT("/Game/RutaVirtual/Vehicles/Interior/Detalle/M_ReflejoCabina"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Shell(TEXT("/Game/RutaVirtual/Vehicles/Interior/Detalle/SM_CarroceriaCabina"));
    CabinShellMesh=Shell.Object;
    DisplayPlane=Plane.Object;
    CenterMesh=Center.Object; SideMesh=Side.Object; TrimMesh=Trim.Object; LeverMesh=GearLever.Object; Reflection=Surface.Object;
}

UStaticMeshComponent* URVCabinComponent::AddMesh(FName Name, UStaticMesh* Mesh, const FVector& Position)
{
    auto* Part = NewObject<UStaticMeshComponent>(GetOwner(),Name);
    Part->SetupAttachment(GetOwner()->GetRootComponent());
    Part->SetStaticMesh(Mesh);
    Part->SetRelativeLocation(Position);
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetGenerateOverlapEvents(false);
    Part->SetCanEverAffectNavigation(false);
    GetOwner()->AddInstanceComponent(Part);
    Part->RegisterComponent();
    return Part;
}

void URVCabinComponent::BeginPlay()
{
    Super::BeginPlay();
    Assist = GetOwner()->FindComponentByClass<URVDrivingAssistComponent>();
    TArray<UCameraComponent*> Cameras;
    GetOwner()->GetComponents(Cameras);
    for (auto* Camera : Cameras)
        if (Camera->GetName().StartsWith(TEXT("FrontCamera"))) CabinCamera=Camera;
    if (!Assist || !CabinCamera || !CenterMesh || !SideMesh || !TrimMesh || !LeverMesh || !Reflection) { SetComponentTickEnabled(false); return; }
    if (!UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
        CabinCamera->SetRelativeRotation(FRotator(-7,0,0));
    // FBX importers can bake the front-axis conversion into the mesh or its node.
    // The outboard brackets define the widest axis of the trim; glass thickness defines its normal.
    auto* Trim=AddMesh(TEXT("DetalleCabina"),TrimMesh,FVector::ZeroVector);
    const auto TrimExtent=TrimMesh->GetBounds().BoxExtent;
    if (TrimExtent.X>TrimExtent.Y) Trim->SetRelativeRotation(FRotator(0,90,0));
    TArray<UStaticMeshComponent*> BodyParts;
    GetOwner()->GetComponents(BodyParts);
    for (auto* Part : BodyParts)
        if (Part->GetFName()==TEXT("Chasis_Main")) ExteriorBody=Part;
    if (CabinShellMesh && ExteriorBody)
    {
        CabinShell=AddMesh(TEXT("CarroceriaCabina"),CabinShellMesh,FVector::ZeroVector);
        CabinShell->AttachToComponent(ExteriorBody,FAttachmentTransformRules::SnapToTargetIncludingScale);
        const auto Extent=CabinShellMesh->GetBounds().BoxExtent;
        if (Extent.Y>Extent.X) CabinShell->SetRelativeRotation(FRotator(0,90,0));
        for (int32 Slot=0; Slot<CabinShellMesh->GetStaticMaterials().Num(); ++Slot)
        {
            const FName Name=CabinShellMesh->GetStaticMaterials()[Slot].MaterialSlotName;
            const int32 Original=ExteriorBody->GetMaterialIndex(Name);
            if (Original!=INDEX_NONE) CabinShell->SetMaterial(Slot,ExteriorBody->GetMaterial(Original));
        }
        CabinShell->SetVisibility(false);
    }
    Lever=AddMesh(TEXT("PalancaInterior"),LeverMesh,FVector(-17,-5,59));
    const auto KnobExtent=LeverMesh->GetBounds().BoxExtent;
    if (KnobExtent.X<KnobExtent.Y) LeverMeshToMount=FRotator(0,90,0).Quaternion();
    Movement=GetOwner()->FindComponentByClass<UChaosWheeledVehicleMovementComponent>();
    if (DisplayPlane && Movement)
    {
        ClusterDisplay=InstallDisplay(TEXT("CuadroInstrumentos"),{-4.91,-32,82.65},{48.5,10.1},{864,180});
        ConsoleDisplay=InstallDisplay(TEXT("PantallaConsola"),{-2.95,25,75},{32.3,20.1},{640,400});
        ClusterDisplay->OnCanvasRenderTargetUpdate.AddDynamic(this,&URVCabinComponent::DrawCluster);
        ConsoleDisplay->OnCanvasRenderTargetUpdate.AddDynamic(this,&URVCabinComponent::DrawConsole);
    }
    for (int32 I=0; I<3; ++I)
    {
        const auto& Mount=Mounts[I];
        auto* Mirror=AddMesh(FName(*FString::Printf(TEXT("Espejo%s"),Mount.Key)), I==1 ? CenterMesh : SideMesh, Mount.Position);
        const auto Extent=Mirror->GetStaticMesh()->GetBounds().BoxExtent;
        MirrorMeshToMount[I]=Extent.Y<Extent.X ? FRotator(0,90,0).Quaternion() : FQuat::Identity;
        Mirrors.Add(Mirror);
        FVector2D Saved;
        if (GConfig->GetVector2D(SettingsSection,Mount.Key,Saved,GGameUserSettingsIni) && !Saved.ContainsNaN())
            Angles[I]={FMath::Clamp(Saved.X,-15.,15.),FMath::Clamp(Saved.Y,-10.,10.)};
        AimGlass(I);
        auto* RT=NewObject<UTextureRenderTarget2D>(this);
        RT->RenderTargetFormat=RTF_RGBA16f;
        RT->ClearColor=FLinearColor::Black;
        RT->InitAutoFormat(I==1 ? 384 : 256,I==1 ? 120 : 156);
        auto* Capture=NewObject<USceneCaptureComponent2D>(GetOwner(),FName(*FString::Printf(TEXT("Reflejo%s"),Mount.Key)));
        Capture->SetupAttachment(GetOwner()->GetRootComponent());
        Capture->TextureTarget=RT;
        Capture->bCaptureEveryFrame=false;
        Capture->bCaptureOnMovement=false;
        Capture->CaptureSource=SCS_SceneColorHDRNoAlpha;
        Capture->bUseCustomProjectionMatrix=true;
        Capture->FOVAngle=145;
        Capture->MaxViewDistanceOverride=15000;
        Capture->LODDistanceFactor=1.6f;
        Capture->ShowFlags.SetMotionBlur(false);
        Capture->ShowFlags.SetDynamicShadows(false);
        Capture->ShowFlags.SetLumenGlobalIllumination(false);
        Capture->ShowFlags.SetLumenReflections(false);
        GetOwner()->AddInstanceComponent(Capture); Capture->RegisterComponent();
        Captures.Add(Capture);
        const int32 Slot=Mirror->GetMaterialIndex(TEXT("M_RV_Cristal"));
        if (Slot!=INDEX_NONE && Reflection)
            if (auto* M=Mirror->CreateDynamicMaterialInstance(Slot,Reflection)) M->SetTextureParameterValue(TEXT("Reflejo"),RT);
    }
    for (USceneCaptureComponent2D* Capture : Captures)
        for (UStaticMeshComponent* Mirror : Mirrors) Capture->HideComponent(Mirror);
}

void URVCabinComponent::AimGlass(int32 Index)
{
    const FVector Eye=GetOwner()->GetActorTransform().InverseTransformPosition(CabinCamera->GetAttachParent()->GetComponentLocation());
    const auto& Mount=Mounts[Index];
    const FVector Normal=((Eye-Mount.Position).GetSafeNormal()+Mount.RearDirection.GetSafeNormal()).GetSafeNormal();
    FRotator Aim=Normal.Rotation();
    Aim.Yaw+=Angles[Index].X;
    Aim.Pitch+=Angles[Index].Y;
    Mirrors[Index]->SetRelativeRotation(Aim.Quaternion()*MirrorMeshToMount[Index]);
}

void URVCabinComponent::UpdateCapture(int32 Index,const FVector& Eye)
{
    const auto& Mount=Mounts[Index];
    const auto* Mirror=Mirrors[Index].Get();
    const FQuat Basis=Mirror->GetComponentQuat()*MirrorMeshToMount[Index].Inverse();
    const FVector N=Basis.GetForwardVector(), Y=Basis.GetRightVector(), Z=Basis.GetUpVector();
    const FVector Center=Mirror->GetComponentLocation()+N*.10;
    const double Depth=FVector::DotProduct(Eye-Center,N);
    if (Depth<=1.) return;
    const FVector VirtualEye=Eye-2.*Depth*N;
    auto* Capture=Captures[Index].Get();
    Capture->SetWorldLocationAndRotation(VirtualEye,Basis);
    // Map the four physical glass edges onto the full target, even for an off-axis eye.
    // The near plane at the glass clips the cabin between the mirror and the virtual eye.
    Capture->CustomProjectionMatrix=FMatrix(
        FPlane(2*Depth/Mount.Width,0,0,0), FPlane(0,2*Depth/Mount.Height,0,0),
        FPlane(-2*FVector::DotProduct(Center-Eye,Y)/Mount.Width,-2*FVector::DotProduct(Center-Eye,Z)/Mount.Height,0,1),
        FPlane(0,0,Depth,0));
    Capture->CaptureScene();
}

FString URVCabinComponent::MirrorName() const
{
    return SelectedMirror==INDEX_NONE ? FString() : FString(Mounts[SelectedMirror].Label);
}

void URVCabinComponent::FinishAdjustment()
{
    if (bPreviewCamera && CabinCamera) CabinCamera->SetRelativeRotation(OriginalCamera);
    bPreviewCamera=false;
    SelectedMirror=INDEX_NONE;
    if (bAnglesChanged)
    {
        for (int32 I=0; I<3; ++I) GConfig->SetVector2D(SettingsSection,Mounts[I].Key,Angles[I],GGameUserSettingsIni);
        GConfig->Flush(false,GGameUserSettingsIni);
        bAnglesChanged=false;
    }
}

void URVCabinComponent::CycleMirror()
{
    if (!CabinCamera || Assist->bPracticePaused || !CabinCamera->IsActive() || Assist->GetSpeedKmh()>.5f) return;
    if (SelectedMirror==2) { FinishAdjustment(); return; }
    ++SelectedMirror;
    if (!UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
    {
        if (!bPreviewCamera) OriginalCamera=CabinCamera->GetRelativeRotation();
        bPreviewCamera=true;
        CabinCamera->SetWorldRotation((Mirrors[SelectedMirror]->GetComponentLocation()-CabinCamera->GetComponentLocation()).Rotation());
    }
}

void URVCabinComponent::AdjustMirror(float HorizontalDegrees,float VerticalDegrees)
{
    if (SelectedMirror==INDEX_NONE) return;
    auto& Offset=Angles[SelectedMirror];
    Offset.X=FMath::Clamp(Offset.X+HorizontalDegrees,-15.,15.);
    Offset.Y=FMath::Clamp(Offset.Y+VerticalDegrees,-10.,10.);
    AimGlass(SelectedMirror);
    bAnglesChanged=true;
}

void URVCabinComponent::TickComponent(float DeltaTime,ELevelTick Type,FActorComponentTickFunction* Tick)
{
    Super::TickComponent(DeltaTime,Type,Tick);
    auto* PC=Cast<APlayerController>(CastChecked<APawn>(GetOwner())->GetController());
    if (ExteriorBody && CabinShell)
    {
        const bool FromSeat=PC && PC->IsLocalController() && CabinCamera->IsActive();
        if (FromSeat!=bCabinShellActive)
        {
            bCabinShellActive=FromSeat;
            ExteriorBody->SetVisibility(!FromSeat);
            CabinShell->SetVisibility(FromSeat);
        }
    }
    if (!PC || !PC->IsLocalController() || !PC->PlayerCameraManager || !CabinCamera->IsActive() || Assist->bPracticePaused)
    {
        if (SelectedMirror!=INDEX_NONE) FinishAdjustment();
        return;
    }
    const int32 Gear=Assist->bWheelMode ? Assist->GetLastWheelSample().Gear : (Assist->bReverseMode ? -1 : 0);
    const int32 Slot=Gear<0 ? 6 : Gear;
    const double Across=Slot ? ((Slot-1)/2-1)*12. : 0.;
    const double Along=Slot ? ((Slot%2) ? 13. : -13.) : 0.;
    Lever->SetRelativeRotation(FRotator(-Along,0,Across).Quaternion()*LeverMeshToMount);
    InstrumentCountdown-=DeltaTime;
    if (InstrumentCountdown<=0.f && ClusterDisplay && ConsoleDisplay)
    {
        InstrumentCountdown=.08f;
        UpdateInstruments();
    }

    const auto& Input = Assist->GetLastWheelSample();
    if (Input.bMirrorSelect && !bPaddlesDown) CycleMirror();
    bPaddlesDown = Input.bMirrorSelect;
    if (SelectedMirror!=INDEX_NONE)
    {
        if (Assist->GetSpeedKmh()>.5f) FinishAdjustment();
        else
        {
            const float Pan=float(Input.MirrorPan)+float(PC->IsInputKeyDown(EKeys::Right))-float(PC->IsInputKeyDown(EKeys::Left));
            const float Tilt=float(Input.MirrorTilt)+float(PC->IsInputKeyDown(EKeys::Up))-float(PC->IsInputKeyDown(EKeys::Down));
            if (Pan || Tilt) AdjustMirror(-Pan*12.f*DeltaTime,Tilt*12.f*DeltaTime);
        }
    }
    CaptureCountdown-=DeltaTime;
    if (CaptureCountdown>0) return;
    CaptureCountdown=1.f/45.f;
    const FVector Eye=PC->PlayerCameraManager->GetCameraLocation();
    const FVector View=PC->PlayerCameraManager->GetCameraRotation().Vector();
    for (int32 Attempt=0; Attempt<3; ++Attempt)
    {
        const int32 Index=NextMirror; NextMirror=(NextMirror+1)%3;
        if (FVector::DotProduct((Mirrors[Index]->GetComponentLocation()-Eye).GetSafeNormal(),View)>.27)
        {
            UpdateCapture(Index,Eye);
            break;
        }
    }
}

UCanvasRenderTarget2D* URVCabinComponent::InstallDisplay(FName Name, FVector Center, FVector2D Size, FIntPoint Resolution)
{
    auto* Screen=AddMesh(Name,DisplayPlane,Center);
    // The Engine plane's +X follows increasing U and +Y follows increasing V.
    Screen->SetRelativeRotation(FRotationMatrix::MakeFromXY(FVector::RightVector,-FVector::UpVector).ToQuat());
    Screen->SetRelativeScale3D(FVector(Size.X/100.,Size.Y/100.,1));
    Screen->SetCastShadow(false);
    auto* Target=UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this,UCanvasRenderTarget2D::StaticClass(),Resolution.X,Resolution.Y);
    Target->ClearColor=FLinearColor(.007f,.012f,.016f,1.f);
    Screen->CreateDynamicMaterialInstance(0,Reflection)->SetTextureParameterValue(TEXT("Reflejo"),Target);
    return Target;
}

void URVCabinComponent::UpdateInstruments()
{
    const int32 Speed=FMath::RoundToInt(Assist->GetSpeedKmh());
    const int32 Rpm=50*FMath::RoundToInt(Movement->GetEngineRotationSpeed()/50.f);
    const FString Gear=Assist->GetGearLabel();
    if (Speed!=DisplaySpeed || Rpm!=DisplayRpm || Gear!=DisplayGear)
    {
        DisplaySpeed=Speed;
        DisplayRpm=Rpm;
        DisplayGear=Gear;
        ClusterDisplay->FastUpdateResource();
    }

    const auto* Controller=Cast<ARVPracticePlayerController>(CastChecked<APawn>(GetOwner())->GetController());
    const int32 Step=Assist->GetCalibrationStep();
    int32 Pedal=INDEX_NONE, Shifter=7, Steering=0;
    FString Message;
    if (Step!=INDEX_NONE)
    {
        Message=Assist->GetInstruction();
        if (Step==1 || Step==2) Steering=Step==1 ? -1 : 1;
        else if (Step>=3 && Step<=5) Pedal=5-Step;
        else Shifter=Step>=6 && Step<=11 ? Step-5 : Step==12 ? -1 : 0;
    }
    else if (SelectedMirror!=INDEX_NONE)
    {
        Message=FString(TEXT("Retrovisor "))+MirrorName();
    }
    else if (Assist->IsWaitingForClutch())
    {
        Pedal=0;
        Message=TEXT("Pisa el embrague. Mantén la marcha.");
    }
    else if (Controller && Controller->bShowDrivingGuide)
    {
        if (Assist->IsWaitingForWheelReport()) { Shifter=0; Message=TEXT("Mueve un mando del G920."); }
        else if (Assist->NeedsWheelNeutral()) { Pedal=0; Message=TEXT("Suelta el acelerador y pisa el embrague."); }
        else if (Assist->bUrbanMode && Assist->PracticeStage==0)
        {
            Pedal=-2;
            Message=TEXT("Libera el freno de mano.");
        }
        else if (Assist->bWheelMode && !Assist->SelectedGear && Speed<1)
        {
            Shifter=1;
            Message=TEXT("Pisa el embrague y pon primera.");
        }
        else Message=Assist->GetInstruction();
    }
    if (Message!=DisplayInstruction || Pedal!=DisplayPedal || Shifter!=DisplayShifter ||
        Steering!=DisplaySteering || Step!=DisplayStep)
    {
        DisplayInstruction=Message; DisplayPedal=Pedal; DisplayShifter=Shifter;
        DisplaySteering=Steering; DisplayStep=Step;
        ConsoleDisplay->FastUpdateResource();
    }
}

void URVCabinComponent::DrawCluster(UCanvas* Canvas,int32 Width,int32 Height)
{
    using namespace RVInstruments;
    if (!Canvas) return;
    const FGuidePainter G{*Canvas,Width/864.f};
    const FLinearColor Dial(.33f,.43f,.46f), Needle(1.f,.30f,.08f);
    auto DrawDial=[&](float X,float Fraction,const TCHAR* Unit,const TCHAR* End)
    {
        constexpr float Y=133.f, Radius=105.f;
        auto Point=[&](float T,float R)
        {
            const float Angle=UE_PI*(1.f-FMath::Clamp(T,0.f,1.f));
            return FVector2D(X+R*FMath::Cos(Angle),Y-R*FMath::Sin(Angle));
        };
        for (int32 I=0; I<40; ++I)
        {
            const auto A=Point(I/40.f,Radius), B=Point((I+1)/40.f,Radius);
            G.Line(A.X,A.Y,B.X,B.Y,3,Dial);
            if (I%2==0)
            {
                const auto Tick=Point(I/40.f,Radius-(I%10==0 ? 15.f : 7.f));
                G.Line(A.X,A.Y,Tick.X,Tick.Y,3,Ink);
            }
        }
        const auto Tip=Point(Fraction,Radius-15);
        G.Line(X,Y,Tip.X,Tip.Y,5,Needle);
        G.Box(X-5,Y-5,10,10,Ink);
        G.Text(X-115,137,TEXT("0"),16,Muted);
        G.Text(X+84,137,End,16,Muted);
        G.Text(X-45,146,Unit,12,Muted);
    };
    DrawDial(134,DisplayRpm/8000.f,TEXT("RPM × 1000"),TEXT("8"));
    DrawDial(730,DisplaySpeed/120.f,TEXT("km/h"),TEXT("120"));
    G.Text(326,10,DisplayGear,62,Mint);
    G.Text(447,10,FString::FromInt(DisplaySpeed),62,Ink);
    G.Text(321,111,TEXT("MARCHA"),17,Muted);
    G.Text(450,111,TEXT("km/h"),18,Muted);
}

void URVCabinComponent::DrawConsole(UCanvas* Canvas,int32 Width,int32 Height)
{
    using namespace RVInstruments;
    if (!Canvas) return;
    const FGuidePainter G{*Canvas,Width/320.f};
    G.Text(12,6,TEXT("RUTA VIRTUAL"),15,Mint);
    G.Line(12,36,306,36,1,Muted);
    if (DisplayInstruction.IsEmpty()) return;
    const bool HasIcon=DisplayPedal!=INDEX_NONE || DisplayShifter!=7 || DisplaySteering!=0;
    if (DisplayPedal>=0) G.Pedals(13,81,DisplayPedal);
    else if (DisplayPedal==-2) G.Key(9,84,91,TEXT("Espacio"));
    else if (DisplayShifter!=7) G.Shifter(5,55,DisplayShifter);
    else if (DisplaySteering) G.Wheel(54,101,DisplaySteering);
    const float X=HasIcon ? 119.f : 13.f;
    G.Paragraph(X,54,307-X,DisplayInstruction,16);
    if (DisplayStep!=INDEX_NONE) G.Text(13,166,TEXT("Enter   Registrar"),14,Mint);
}
