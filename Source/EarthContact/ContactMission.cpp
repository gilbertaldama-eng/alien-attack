#include "ContactMission.h"
#include "ContactImportedAssets.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Canvas.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWaveProcedural.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "ProceduralMeshComponent.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "TimerManager.h"

namespace
{
    // Original, generated sound cues; no external recordings or licensing needed.
    // The bark is stylized and can later be replaced by a real dog recording.
    void PlayIntroSound(UObject* Context,const FVector& Where,bool bBark)
    {
        constexpr int32 Rate=22050;
        const float Duration=bBark ? .36f : 1.25f;
        const int32 Count=FMath::CeilToInt(Rate*Duration);
        USoundWaveProcedural* Sound=NewObject<USoundWaveProcedural>(Context);
        if (!Sound) return;
        Sound->NumChannels=1;
        Sound->SetSampleRate(Rate);
        Sound->SampleByteSize=sizeof(int16);
        Sound->Duration=Duration;
        Sound->bLooping=false;
        TArray<int16> PCM;
        PCM.SetNumUninitialized(Count);
        FRandomStream Random(bBark ? 1822 : 1947);
        float Filtered=0.f;
        for (int32 I=0;I<Count;++I)
        {
            const float T=float(I)/Rate;
            const float Noise=Random.FRandRange(-1.f,1.f);
            Filtered=.96f*Filtered+.04f*Noise;
            float Wave=0.f;
            if (bBark)
            {
                // Two rough exhalation pulses with a downward-pitched throat tone.
                const float P1=FMath::Exp(-FMath::Abs(T-.075f)*19.f);
                const float P2=.64f*FMath::Exp(-FMath::Abs(T-.205f)*24.f);
                const float Envelope=(P1+P2)*FMath::Clamp(T/.022f,0.f,1.f);
                const float Throat=FMath::Sin(2.f*PI*(185.f*T-90.f*T*T));
                Wave=Envelope*(.52f*Throat+.40f*Noise+.10f*Filtered);
            }
            else
            {
                const float Envelope=FMath::Clamp(T/.025f,0.f,1.f)*FMath::Exp(-T*2.7f);
                const float Low=FMath::Sin(2.f*PI*(66.f*T-21.f*T*T));
                Wave=Envelope*(.62f*Low+.60f*Filtered+.24f*Noise);
            }
            PCM[I]=int16(FMath::Clamp(Wave,-.96f,.96f)*28000.f);
        }
        Sound->QueueAudio(reinterpret_cast<const uint8*>(PCM.GetData()),
                          PCM.Num()*sizeof(int16));
        UGameplayStatics::PlaySoundAtLocation(Context,Sound,Where);
    }


    // Fast, native Unreal geometry for genuinely non-human silhouettes.
    // Each primitive is decorative only; the character capsule supplies collision.
    void BuildAlienVisual(AContactResponder* Unit, bool Nordic)
    {
        // Prefer the imported CC0 rigged Grey in the game content folder.
        // The procedural fallback remains only when the real asset is absent.
        if (!Nordic)
        {
            USkeletalMesh* Grey=ContactAssets::Grey();
            if (Grey)
            {
                Unit->GetMesh()->SetSkeletalMesh(Grey);
                Unit->GetMesh()->SetRelativeLocation(FVector(0,0,-88));
                Unit->GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
                Unit->GetMesh()->SetRelativeScale3D(FVector(1.3f)); // 140cm source to 91cm actor
                Unit->GetMesh()->SetVisibility(true,true);
                UE_LOG(LogTemp,Display,TEXT("GREY ASSET real skeletal mesh loaded"));
                return;
            }
            UE_LOG(LogTemp,Warning,TEXT("GREY ASSET not imported; showing procedural fallback"));
        }
        if (Nordic)
        {
            // New monster leader replaces the Nordic visual, not combat mechanics.
            if (USkeletalMesh* Leader=ContactAssets::Boss())
            {
                Unit->GetMesh()->SetSkeletalMesh(Leader);
                Unit->GetMesh()->SetRelativeLocation(FVector(0,0,-88));
                Unit->GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
                Unit->GetMesh()->SetVisibility(true,true);
                UE_LOG(LogTemp,Display,TEXT("CONTACT ASSET monster leader uses imported skeletal mesh"));
                return;
            }
        }
        const UStaticMesh* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere"));
        const UStaticMesh* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder"));
        const auto* Basic=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
        auto Part=[&](const TCHAR* Name,const UStaticMesh* Shape,FVector Location,FVector Scale,
                      FLinearColor Color,FRotator Rotation=FRotator::ZeroRotator)
        {
            auto* Mesh=NewObject<UStaticMeshComponent>(Unit,Name);
            Mesh->SetupAttachment(Unit->GetRootComponent());
            Mesh->SetStaticMesh(const_cast<UStaticMesh*>(Shape));
            Mesh->SetRelativeLocationAndRotation(Location,Rotation);
            Mesh->SetRelativeScale3D(Scale);
            Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Mesh->SetCastShadow(true);
            Mesh->RegisterComponent();
            auto* Mat=UMaterialInstanceDynamic::Create(const_cast<UMaterialInterface*>(Basic),Mesh);
            if(Mat) { Mat->SetVectorParameterValue(TEXT("Color"),Color); Mesh->SetMaterial(0,Mat); }
            return Mesh;
        };
        if(!Sphere || !Cylinder || !Basic) return;
        const FLinearColor Skin=Nordic ? FLinearColor(.82f,.87f,.93f) : FLinearColor(.40f,.45f,.45f);
        const FLinearColor Eye(.004f,.008f,.015f);
        const FLinearColor Shade=Nordic ? FLinearColor(.91f,.95f,1.f) : FLinearColor(.22f,.26f,.26f);
        // Grey: large bulbous head and dark almond eyes. Nordic: elongated pale humanoid.
        Part(TEXT("EntityTorso"),Sphere,{0,0,91},Nordic ? FVector(.36f,.31f,.59f) : FVector(.31f,.25f,.51f),Skin);
        Part(TEXT("EntityHips"),Sphere,{0,0,54},{.29f,.28f,.25f},Skin);
        Part(TEXT("EntityHead"),Sphere,{0,0,145},Nordic ? FVector(.36f,.38f,.46f) : FVector(.57f,.50f,.55f),Skin);
        for (int S : {-1,1})
        {
            Part(S<0 ? TEXT("EntityEyeLeft"):TEXT("EntityEyeRight"),Sphere,
                 Nordic ? FVector(28,S*16,148) : FVector(40,S*20,147),
                 Nordic ? FVector(.10f,.11f,.16f) : FVector(.115f,.15f,.25f),Eye,
                 FRotator(0,S*10,S*12));
            Part(S<0 ? TEXT("EntityArmLeft"):TEXT("EntityArmRight"),Cylinder,
                 FVector(0,S*36,90),FVector(.12f,.12f,.64f),Skin,FRotator(S*12,0,0));
            Part(S<0 ? TEXT("EntityHandLeft"):TEXT("EntityHandRight"),Sphere,
                 FVector(5,S*42,49),FVector(.15f,.12f,.16f),Skin);
            Part(S<0 ? TEXT("EntityLegLeft"):TEXT("EntityLegRight"),Cylinder,
                 FVector(0,S*17,28),FVector(.15f,.15f,.56f),Shade,FRotator(S*4,0,0));
            Part(S<0 ? TEXT("EntityFootLeft"):TEXT("EntityFootRight"),Sphere,
                 FVector(12,S*18,5),FVector(.27f,.15f,.105f),Skin);
        }
        if (Nordic)
        {
            Part(TEXT("NordicSilverCrown"),Sphere,{-7,0,174},FVector(.34f,.36f,.09f),Shade);
            Part(TEXT("NordicNose"),Sphere,{35,0,140},FVector(.10f,.085f,.20f),Skin);
        }
        // Hide the mannequin; gameplay collision, targeting and AI remain intact.
        Unit->GetMesh()->SetVisibility(false,false); // Keep attached weapon visibility independent.
    }

    FVector Barricade(bool Alien, int32 Slot)
    {
        return FVector((Alien ? 1 : -1) * (600 + (Slot / 6) * 670), (Slot % 6 - 2.5) * 490, 0);
    }
    bool Active(UWorld* World)
    {
        const auto* State = World->GetGameState<AContactState>();
        return State && State->Result == EContactResult::Active;
    }

    // Original carapace built along the facing axis. Narrow rings form the snout,
    // flared brow and swept crest. Separate curved mandibles frame the visor.
    void FitRaiderCarapace(AContactResponder* Unit)
    {
        auto* Shell=NewObject<UProceduralMeshComponent>(Unit);
        Shell->SetupAttachment(Unit->GetRootComponent());
        Shell->SetRelativeLocation(FVector(0,0,66));
        Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Shell->RegisterComponent();
        int Section=0;
        auto Loft=[&](const TArray<FVector>& Spine,const TArray<FVector2D>& Radii,FLinearColor Color)
        {
            TArray<FVector> Vertices,Normals;
            TArray<int32> Faces;
            constexpr int Sides=10;
            for (int Ring=0; Ring<Spine.Num(); ++Ring)
                for (int Edge=0; Edge<Sides; ++Edge)
                {
                    const double A=2*PI*Edge/Sides;
                    const FVector N(0,FMath::Cos(A),FMath::Sin(A));
                    Vertices.Add(Spine[Ring]+FVector(0,N.Y*Radii[Ring].X,N.Z*Radii[Ring].Y));
                    Normals.Add(N);
                    if (Ring==Spine.Num()-1) continue;
                    const int K=Ring*Sides+Edge, J=Ring*Sides+(Edge+1)%Sides;
                    Faces.Append({K,J+Sides,J,K,K+Sides,J+Sides});
                }
            Shell->CreateMeshSection(Section,Vertices,Faces,Normals,{},{},{},false);
            auto* Mat=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial")),Unit);
            Mat->SetVectorParameterValue(TEXT("Color"),Color);
            Shell->SetMaterial(Section++,Mat);
        };
        Loft({{26,0,-3},{17,0,3},{1,0,8},{-20,0,16},{-43,0,24},{-59,0,27}},
             {{1,1},{14,8},{20,14},{17,10},{7,5},{.2f,.2f}},{.22f,.042f,.045f});
        for (int Side : {-1,1})
        {
            Loft({{29.,Side*8.,-16},{20.,Side*16.,-9},{4.,Side*17.,-1},{-12.,Side*13.,3}},
                 {{.3f,.3f},{3,4},{4,5},{1,1}},{.55f,.36f,.20f});
            Loft({{24.,Side*8.,3},{18.,Side*14.,5},{9.,Side*18.,7}},
                 {{.3f,.3f},{3,2},{.3f,.3f}},{1.f,.18f,.025f});
        }
    }
}

void AContactState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AContactState, Result);
    DOREPLIFETIME(AContactState, Allies);
    DOREPLIFETIME(AContactState, Raiders);
    DOREPLIFETIME(AContactState, Uplink);
}

AContactResponder::AContactResponder()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCharacterMovement()->MaxWalkSpeed = 440;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    bUseControllerRotationYaw = true;
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetRelativeLocationAndRotation({0,0,-88},FRotator(0,-90,0));
    GetMesh()->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")));
    IdlePose = LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS"));
    JogPose = LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Fwd"));
    Report = LoadObject<USoundBase>(nullptr,TEXT("/Game/XRFramework/Audio/Fire01"));
    Gun = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rifle"));
    Gun->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle")));
    Gun->SetupAttachment(GetMesh(), TEXT("hand_r"));
    Gun->SetAbsolute(false,true,false);
    Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Gun->SetRelativeScale3D(FVector(1.18f)); // make the existing rifle more readable
    Sidearm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SidearmProxy"));
    // Use the free First Person template pistol mesh if it exists in the project.
    // Otherwise construct a detailed pistol silhouette from built-in primitives:
    // long squared slide, angled grip, trigger guard and aligned iron sights.
    UStaticMesh* PistolModel=LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/Weapons/Pistol/Meshes/SM_Pistol.SM_Pistol"));
    UStaticMesh* Shape=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube"));
    Sidearm->SetStaticMesh(PistolModel ? PistolModel : Shape);
    Sidearm->SetupAttachment(GetMesh(),TEXT("hand_r"));
    Sidearm->SetRelativeLocation(FVector(8,18,-5));
    Sidearm->SetRelativeScale3D(PistolModel ? FVector(1.f) : FVector(.085f,.24f,.067f));
    if (!PistolModel)
    {
        auto AddPistolDetail=[&](const TCHAR* Name,FVector Offset,FVector Size,FRotator Rotate=FRotator::ZeroRotator)
        {
            auto* Detail=CreateDefaultSubobject<UStaticMeshComponent>(Name);
            Detail->SetupAttachment(Sidearm);
            Detail->SetStaticMesh(Shape);
            Detail->SetRelativeLocationAndRotation(Offset,Rotate);
            Detail->SetRelativeScale3D(Size);
            Detail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        };
        AddPistolDetail(TEXT("PistolGrip"),FVector(-1,-12,-10),
            FVector(.078f,.075f,.19f),FRotator(-12,0,0));
        AddPistolDetail(TEXT("PistolTriggerGuard"),FVector(0,1,-5),
            FVector(.035f,.085f,.025f));
        AddPistolDetail(TEXT("PistolFrontSight"),FVector(0,19,4),
            FVector(.04f,.025f,.025f));
        AddPistolDetail(TEXT("PistolRearSight"),FVector(0,-17,4),
            FVector(.06f,.02f,.027f));
    }
    Sidearm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Sidearm->SetVisibility(false,true);
    auto* Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    Boom->SetupAttachment(RootComponent);
    Boom->SetRelativeLocation({0,0,55});
    Boom->TargetArmLength = 250;
    Boom->SocketOffset = {0,58,12};
    Boom->bUsePawnControlRotation = true;
    auto* Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
}

void AContactResponder::BeginPlay()
{
    Super::BeginPlay();
    Animate(false);
}

void AContactResponder::BeginSoloHuntWithPistol()
{
    if (!HasAuthority()) return;
    // At home the responder owns a 17-round sidearm, not a battle rifle.
    // Switching to the existing rifle remains possible after the prologue.
    bPistol=true;
    Ammo=PistolAmmo;
    bTrigger=false;
    Gun->SetVisibility(false,true);
    Sidearm->SetVisibility(true,true);
    UE_LOG(LogTemp,Display,TEXT("INTRO starter pistol equipped, rounds=%d"),Ammo);
}

void AContactResponder::BeginRescueMode()
{
    if (!HasAuthority()) return;
    // The dog is alive but missing. Equip the existing rifle; preserve ammo.
    // This is an emotional combat transition, not an invulnerability cheat.
    if (bPistol) PistolAmmo=Ammo;
    bPistol=false;
    Ammo=RifleAmmo;
    bReloading=false;
    bTrigger=false;
    Gun->SetVisibility(true,true);
    Sidearm->SetVisibility(false,true);
    UE_LOG(LogTemp,Warning,TEXT("RESCUE MODE armed: dog taken, rifle ready"));
}

void AContactResponder::AssignPost(bool Alien, int32 Index)
{
    Slot = Index;
    bAlien = Alien;
    Health = Alien ? 110 : 100;
    if (Alien)
    {
        GetMesh()->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")));
        GetMesh()->SetRelativeScale3D({1.18,.8f,1.08});
        if (GetWorld()->GetGameState<AContactState>() && !GetWorld()->GetGameState<AContactState>()->bHunt)
            FitRaiderCarapace(this); // Keep the original relay-battle creatures unchanged.
    }
    for (int32 i=0; i<GetMesh()->GetNumMaterials(); ++i)
        if (auto* Mat = GetMesh()->CreateDynamicMaterialInstance(i))
            Mat->SetVectorParameterValue(TEXT("Tint"), Alien ? FLinearColor(.85f,.09f,.025f) : FLinearColor(.12f,.53f,.62f));
    GetCharacterMovement()->MaxWalkSpeed = 240;
    NextShot = GetWorld()->GetTimeSeconds() + FMath::FRandRange(.5f,2);
    Destination = GetActorLocation();
    Animate(false);
}

void AContactResponder::Animate(bool Moving)
{
    bJog = Moving;
    if (UAnimSequence* Clip=(Moving && JogPose) ? JogPose.Get() : IdlePose.Get())
        GetMesh()->PlayAnimation(Clip,true);
}

void AContactResponder::AssignHunter(int32 Index)
{
    AssignPost(true,Index);
    Health=HunterHP;
    const float HeightCm=91.44f; // three feet
    const float Scale=HeightCm/(2*GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
    SetActorScale3D(FVector(Scale));
    GetMesh()->SetRelativeScale3D(FVector(.96f));
    BuildAlienVisual(this,false);
    // Only play animation clips authored for the imported Grey skeleton.
    if (GetMesh()->GetSkeletalMeshAsset() &&
        GetMesh()->GetSkeletalMeshAsset()->GetPathName().Contains(TEXT("/Characters/Greys/")))
    {
        IdlePose=nullptr;
        JogPose=nullptr;
        IdlePose=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Greys/AS_GreyIdle.AS_GreyIdle"));
        JogPose=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Greys/AS_GreyWalk.AS_GreyWalk"));
        if (!IdlePose) JogPose=nullptr; // don't play mannequin clips against Grey skeleton
    }
    Gun->SetVisibility(false);
    Sidearm->SetVisibility(false);
    GetCharacterMovement()->MaxWalkSpeed=195;
    GetCharacterMovement()->MaxStepHeight=12;
    NextGreyBeat=GetWorld()->GetTimeSeconds()+2.0+Index*.65;
    GreyPauseUntil=0;
    bGreyReachPending=false;
    bGreyFlankLeft=(Index%2)==0;
    UE_LOG(LogTemp,Display,TEXT("HUNT hunter=%d hp=%d capsule_cm=%.2f"),Index,Health,2*GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}

void AContactResponder::AssignNordicBoss()
{
    AssignPost(true,5);
    bBoss=true;
    Health=BossHP;
    const float HeightCm=243.84f; // eight feet, Unreal units are centimeters
    const float Scale=HeightCm/(2*GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
    SetActorScale3D(FVector(Scale));
    GetMesh()->SetRelativeScale3D(FVector(.94f,.83f,1.02f));
    BuildAlienVisual(this,true);
    if (ContactAssets::Boss() && GetMesh()->GetSkeletalMeshAsset()==ContactAssets::Boss())
    {
        IdlePose=nullptr;
        JogPose=nullptr; // no mannequin animation against a different rig
    }
    for (int32 I=0; I<GetMesh()->GetNumMaterials(); ++I)
        if (auto* Mat=GetMesh()->CreateDynamicMaterialInstance(I))
            Mat->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.93f,.96f,1.f));
    Gun->SetVisibility(false);
    Sidearm->SetVisibility(false);
    GetCharacterMovement()->MaxWalkSpeed=300;
    NextMindAttack=GetWorld()->GetTimeSeconds()+3.0;
    TeleportFlash(true);
    UE_LOG(LogTemp,Display,TEXT("NORDIC BOSS spawned hp=%d height_cm=%.2f"),Health,
        2*GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}

// Spawn a temporary world-space light independently of the actor so it remains
// visible while the actor itself is hidden. No Niagara assets are required.
void AContactResponder::TeleportFlash(bool bArrival)
{
    if (!HasAuthority() || !GetWorld()) return;
    const FVector Origin=GetActorLocation()+FVector(0,0,bBoss ? 120.f : 30.f);
    if (auto* Flash=GetWorld()->SpawnActor<APointLight>(Origin,FRotator::ZeroRotator))
    {
        if (auto* Light=Flash->PointLightComponent.Get())
        {
            Light->SetLightColor(FLinearColor(.78f,.90f,1.0f));
            Light->SetIntensity(bBoss ? 150000.f : 65000.f);
            Light->SetAttenuationRadius(bBoss ? 1700.f : 750.f);
        }
        Flash->SetLifeSpan(.5f);
    }
    SetActorHiddenInGame(true);
    if (!bArrival)
    {
        SetActorEnableCollision(false);
        return; // The defeated entity remains hidden until the mission resets.
    }
    SetActorEnableCollision(false);
    FTimerHandle RevealTimer;
    GetWorld()->GetTimerManager().SetTimer(RevealTimer,
        FTimerDelegate::CreateWeakLambda(this,[this]()
        {
            SetActorHiddenInGame(false);
            SetActorEnableCollision(true);
            UE_LOG(LogTemp,Display,TEXT("NORDIC BOSS reveal complete"));
        }),.23f,false);
}

void AContactResponder::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AContactResponder,Health);
    DOREPLIFETIME(AContactResponder,Ammo);
    DOREPLIFETIME(AContactResponder,bReloading);
    DOREPLIFETIME(AContactResponder,bPistol);
}

void AContactResponder::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis("Forward",this,&AContactResponder::Forward);
    Input->BindAxis("Right",this,&AContactResponder::Right);
    Input->BindAxis("Turn",this,&APawn::AddControllerYawInput);
    Input->BindAxis("Look",this,&APawn::AddControllerPitchInput);
    Input->BindAction("Sprint",IE_Pressed,this,&AContactResponder::SprintOn);
    Input->BindAction("Sprint",IE_Released,this,&AContactResponder::SprintOff);
    Input->BindAction("Fire",IE_Pressed,this,&AContactResponder::FireOn);
    Input->BindAction("Fire",IE_Released,this,&AContactResponder::FireOff);
    Input->BindAction("Reload",IE_Pressed,this,&AContactResponder::Reload);
    Input->BindAction("EquipRifle",IE_Pressed,this,&AContactResponder::EquipRifle);
    Input->BindAction("EquipPistol",IE_Pressed,this,&AContactResponder::EquipPistol);
    Input->BindAction("Retry",IE_Pressed,this,&AContactResponder::Retry);
}
void AContactResponder::Forward(float V)
{
    const auto* State=GetWorld() ? GetWorld()->GetGameState<AContactState>() : nullptr;
    if (State && State->bHunt && State->IntroStage<2) return;
    AddMovementInput(GetActorForwardVector(),V);
}
void AContactResponder::Right(float V)
{
    const auto* State=GetWorld() ? GetWorld()->GetGameState<AContactState>() : nullptr;
    if (State && State->bHunt && State->IntroStage<2) return;
    AddMovementInput(GetActorRightVector(),V);
}
void AContactResponder::SprintOn() { ServerSprint(true); }
void AContactResponder::SprintOff() { ServerSprint(false); }
void AContactResponder::ServerSprint_Implementation(bool On) { GetCharacterMovement()->MaxWalkSpeed = On ? 700 : 440; }
void AContactResponder::FireOn() { SetTrigger(true); }
void AContactResponder::FireOff() { SetTrigger(false); }
void AContactResponder::SetTrigger_Implementation(bool Down) { bTrigger = Down; }
void AContactResponder::Reload() { ServerReload(); }
void AContactResponder::EquipRifle() { ServerEquip(false); }
void AContactResponder::EquipPistol() { ServerEquip(true); }
void AContactResponder::ServerEquip_Implementation(bool Pistol)
{
    if (IsTrooper() || Health<=0 || bPistol==Pistol) return;
    if (bPistol) PistolAmmo=Ammo; else RifleAmmo=Ammo;
    bPistol=Pistol;
    Ammo=bPistol ? PistolAmmo : RifleAmmo;
    bReloading=false;
    bTrigger=false;
    NextShot=GetWorld()->GetTimeSeconds()+.18;
    Gun->SetVisibility(!bPistol,true);
    Sidearm->SetVisibility(bPistol,true);
}
void AContactResponder::ServerReload_Implementation()
{
    if (Health && Ammo < (bPistol ? 17 : 30) && !bReloading && Active(GetWorld()))
    {
        bReloading = true;
        ReloadAt = GetWorld()->GetTimeSeconds() + (bPistol ? 1.25 : 1.6);
    }
}
void AContactResponder::Retry()
{
    if (!Active(GetWorld()) && GetNetMode()==NM_Standalone)
        UGameplayStatics::OpenLevel(this,TEXT("/Engine/Maps/Entry"),false);
}

void AContactResponder::Think(double Now)
{
    const FVector Here = GetActorLocation();
    if (GetWorld()->GetGameState<AContactState>()->bHunt)
    {
        Target=Cast<AContactResponder>(UGameplayStatics::GetPlayerPawn(this,0));
        if (!Target.IsValid()) return;
        const FVector Toward=(Target->GetActorLocation()-Here).GetSafeNormal2D();
        const double Distance=FVector::Dist2D(Here,Target->GetActorLocation());
        // Only small Greys do this. Boss retains its existing telepathic AI.
        // The pause lets the player *notice* the creature before it acts.
        if (!bBoss && Distance < 1150 && Now >= NextGreyBeat && Now >= GreyPauseUntil)
        {
            const bool Close=Distance < 170;
            GreyPauseUntil=Now+(Close ? 1.15 : 1.75);
            bGreyReachPending=Close;
            NextGreyBeat=GreyPauseUntil+(Close ? 5.25 : 3.75);
            bGreyFlankLeft=!bGreyFlankLeft;
            UE_LOG(LogTemp,Display,TEXT("GREY ENCOUNTER %d %s distance=%.0f"),
                Slot,Close?TEXT("reach warning"):TEXT("motionless stare"),Distance);
        }
        if (!bBoss && Now < GreyPauseUntil)
        {
            Destination=Here; // unnatural full stop, facing the player
            return;
        }
        Destination=Target->GetActorLocation();
        // Flank around the player instead of following a straight-line zombie chase.
        if (!bBoss && Distance > 180 && Distance < 1150)
        {
            const FVector Side(-Toward.Y,Toward.X,0);
            Destination=Target->GetActorLocation()-Toward*210.f+
                Side*(bGreyFlankLeft ? 290.f : -290.f);
        }
        FCollisionQueryParams Query(NAME_None,false,this);
        Query.AddIgnoredActor(Target.Get());
        for (float Angle : {0.f,45.f,-45.f,85.f,-85.f,120.f,-120.f})
        {
            const FVector Step=Toward.RotateAngleAxis(Angle,FVector::UpVector)*90;
            if (!GetWorld()->SweepTestByChannel(Here,Here+Step,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(6),Query))
            {
                // Preserve the intentional flank target when unobstructed;
                // short-step steering is only a fallback around obstacles.
                if (GetWorld()->SweepTestByChannel(Here,Destination,FQuat::Identity,
                    ECC_Visibility,FCollisionShape::MakeSphere(6),Query))
                    Destination=Here+Step;
                break;
            }
        }
        return;
    }
    FVector Post = Barricade(bAlien,Slot);
    const double Side = bAlien ? 1 : -1;
    Destination = Post + FVector(Side*125, Now < HideUntil ? 0 : 170, 0);
    double Best = TNumericLimits<double>::Max();
    Target.Reset();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SquadSight),false,this);
    for (TActorIterator<AContactResponder> It(GetWorld()); It; ++It)
    {
        if (It->bAlien == bAlien || It->Health <= 0) continue;
        const double Range = FVector::DistSquared(Here,It->GetActorLocation());
        if (Range >= Best) continue;
        FHitResult Hit;
        GetWorld()->LineTraceSingleByChannel(Hit, Here+FVector(0,0,45), It->GetActorLocation()+FVector(0,0,30), ECC_Visibility,Query);
        if (Hit.GetActor() == *It) { Target = *It; Best = Range; }
    }
    // Step around our own barrier before advancing to a new firing angle.
    if (Now >= HideUntil && !Target.IsValid() && FVector::Dist2D(Here,Destination) < 240)
        Destination = Post + FVector(-Side*230,195,0);
}

void AContactResponder::Tick(float Delta)
{
    Super::Tick(Delta);
    if (!Health || !Active(GetWorld())) return;
    const bool Moving = GetVelocity().Size2D()>35;
    if (!IsTrooper() && HasAuthority() && !bBoss)
        GetCharacterMovement()->MaxWalkSpeed = (GetWorld()->GetTimeSeconds()<BrainControlledUntil) ? 150.f : 440.f;
    if (Moving != bJog) Animate(Moving);
    // The template rifle's barrel runs along local +Y. Keep its grip on the
    // animated palm while the muzzle follows the view or squad target.
    FVector AimDirection=GetBaseAimRotation().Vector();
    if (IsTrooper())
        AimDirection=Target.IsValid() ? (Target->GetActorLocation()+FVector(0,0,25)-Gun->GetComponentLocation()).GetSafeNormal() : GetActorForwardVector();
    Gun->SetWorldRotation(FRotationMatrix::MakeFromYZ(AimDirection,FVector::UpVector).Rotator());
    Sidearm->SetWorldRotation(Gun->GetComponentRotation());
    if (!HasAuthority()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (bReloading && Now >= ReloadAt)
    {
        Ammo=bPistol ? 17 : 30;
        if (bPistol) PistolAmmo=Ammo; else RifleAmmo=Ammo;
        bReloading=false;
        if (!IsTrooper()) UE_LOG(LogTemp,Display,TEXT("BATTLE player reloaded ammo=%d"),Ammo);
    }
    if (IsTrooper())
    {
        if (GetWorld()->GetGameState<AContactState>()->bHunt &&
            GetWorld()->GetGameState<AContactState>()->IntroStage < 7)
        {
            if (GetWorld()->GetGameState<AContactState>()->IntroStage==7)
            {
                // Uncanny pursuit tableau: greys converge on the same crash corridor as the dog.
                // A properly animated skeletal chase is a later asset/AI milestone.
                const FVector Pursuit(3250,-2550,100);
                const FVector Delta=Pursuit-GetActorLocation();
                if (Delta.Size2D()>180)
                {
                    AddMovementInput(Delta.GetSafeNormal2D(),1.f,true);
                    SetActorRotation(FRotator(0,Delta.Rotation().Yaw,0));
                }
            }
            return;
        }
        if (Now >= NextThink) { Think(Now); NextThink=Now+FMath::FRandRange(.18f,.28f); }
        const bool bGreyPaused=GetWorld()->GetGameState<AContactState>()->bHunt &&
            !bBoss && Now < GreyPauseUntil;
        if (bGreyPaused)
        {
            GetCharacterMovement()->StopMovementImmediately();
        }
        // A reach is deliberately telegraphed. Contact is possible only after
        // the stare, only if the player has remained close, and only with LOS.
        if (!bBoss && bGreyReachPending && Now >= GreyPauseUntil)
        {
            bGreyReachPending=false;
            if (Target.IsValid() && Target->Health>0 &&
                FVector::Dist2D(GetActorLocation(),Target->GetActorLocation())<160.f)
            {
                FHitResult TouchSight;
                FCollisionQueryParams Params(SCENE_QUERY_STAT(GreyTouch),false,this);
                GetWorld()->LineTraceSingleByChannel(TouchSight,
                    GetActorLocation()+FVector(0,0,50),
                    Target->GetActorLocation()+FVector(0,0,55),
                    ECC_Visibility,Params);
                if (TouchSight.GetActor()==Target.Get())
                {
                    Target->BrainControlledUntil=FMath::Max(Target->BrainControlledUntil,Now+.75);
                    Target->ReceiveHit(2,this);
                    UE_LOG(LogTemp,Display,TEXT("GREY ENCOUNTER %d touch completed"),Slot);
                }
            }
        }
        FVector Direction = Destination - GetActorLocation();
        if (!bGreyPaused && Direction.Size2D()>35)
            AddMovementInput(Direction.GetSafeNormal2D(),1,true);
        auto* Enemy = Target.Get();
        FVector Facing = Enemy ? Enemy->GetActorLocation()-GetActorLocation() : Direction;
        if (!Facing.IsNearlyZero()) SetActorRotation(FRotator(0,Facing.Rotation().Yaw,0));
        if (GetWorld()->GetGameState<AContactState>()->bHunt)
        {
            // Boss projects a telepathic suppression field with visible line of sight.
            // Any damage briefly disrupts the channel; the player can use this to escape.
            if (bBoss && Enemy && Enemy->Health && Now >= NextMindAttack && Now >= HideUntil &&
                FVector::Dist2D(GetActorLocation(),Enemy->GetActorLocation()) < 1650)
            {
                FHitResult MentalSight;
                FCollisionQueryParams SightParams(SCENE_QUERY_STAT(NordicMindSight),false,this);
                GetWorld()->LineTraceSingleByChannel(MentalSight,
                    GetActorLocation()+FVector(0,0,125),
                    Enemy->GetActorLocation()+FVector(0,0,55),ECC_Visibility,SightParams);
                if (MentalSight.GetActor()==Enemy)
                {
                    Enemy->BrainControlledUntil=Now+1.65;
                    NextMindAttack=Now+5.25;
                    UE_LOG(LogTemp,Display,TEXT("NORDIC mind-control applied duration=1.65"));
                }
                else NextMindAttack=Now+1.0;
            }
            if (!bGreyPaused && Enemy && Enemy->Health && Now>=NextShot && Now>=HideUntil && FVector::Dist2D(GetActorLocation(),Enemy->GetActorLocation())<(bBoss ? 170 : 80)
                && FMath::Abs(GetActorLocation().Z-Enemy->GetActorLocation().Z)<115)
            {
                FHitResult Contact;
                GetWorld()->LineTraceSingleByChannel(Contact,GetActorLocation(),Enemy->GetActorLocation(),ECC_Visibility,FCollisionQueryParams(NAME_None,false,this));
                if (Contact.GetActor()==Enemy)
                {
                    Enemy->ReceiveHit(bBoss ? 9 : 2,this);
                    NextShot=Now+(bBoss ? 1.7 : 2.5);
                }
            }
            return;
        }
        if (Enemy && Enemy->Health && Now >= NextShot && Now>=HideUntil)
            Shoot(Enemy->GetActorLocation()+FVector(0,0,25),true);
        return;
    }
    if (auto* State=GetWorld()->GetGameState<AContactState>())
        if (State->bHunt && State->IntroStage<2) return; // gaming / barking prologue
    if (bTrigger && !bReloading && Now>=NextShot && Controller)
    {
        FVector Eye; FRotator View;
        Controller->GetPlayerViewPoint(Eye,View);
        FHitResult Sight;
        FVector End=Eye+View.Vector()*14000;
        GetWorld()->LineTraceSingleByChannel(Sight,Eye,End,ECC_Visibility,FCollisionQueryParams(NAME_None,false,this));
        Shoot(Sight.bBlockingHit ? Sight.ImpactPoint : End,false);
        if (bPistol) bTrigger=false; // Semi-auto: release and press for the next shot.
    }
}

void AContactResponder::Shoot(const FVector& Aim, bool Scatter)
{
    if (bReloading) return;
    if (Ammo<=0) { ServerReload(); return; }
    --Ammo;
    if (!IsTrooper()) UE_LOG(LogTemp,Display,TEXT("BATTLE player fired ammo=%d position=%s"),Ammo,*GetActorLocation().ToCompactString());
    const double Now=GetWorld()->GetTimeSeconds();
    NextShot=Now+(Scatter ? FMath::FRandRange(.7f,1.45) : (bPistol ? .30f : .13f));
    const FVector From=GetActorLocation()+FVector(0,0,45)+GetActorForwardVector()*38;
    FVector Dir=(Aim-From).GetSafeNormal();
    if (Scatter) Dir=FMath::VRandCone(Dir,FMath::DegreesToRadians(2.4f));
    FHitResult Hit;
    FVector End=From+Dir*14000;
    GetWorld()->LineTraceSingleByChannel(Hit,From,End,ECC_Visibility,FCollisionQueryParams(NAME_None,false,this));
    if (Hit.bBlockingHit) End=Hit.ImpactPoint;
    if (auto* Victim=Cast<AContactResponder>(Hit.GetActor()); Victim && Victim->bAlien!=bAlien)
        Victim->ReceiveHit(Scatter ? 9 : (bPistol ? 38 : 26),this);
    ShotFX(bPistol && !IsTrooper() ? Sidearm->GetComponentLocation() : Gun->GetComponentTransform().TransformPosition(FVector(0,51,9)),End,Hit.bBlockingHit);
}

void AContactResponder::ShotFX_Implementation(FVector_NetQuantize From,FVector_NetQuantize To,bool Hit)
{
    if (GetNetMode()==NM_DedicatedServer) return;
    const FColor Color=bAlien ? FColor(255,63,38) : FColor(255,214,109);
    DrawDebugLine(GetWorld(),From,To,Color,false,.075f,0,1.4f);
    if (Hit) DrawDebugPoint(GetWorld(),To,5,Color,false,.13f);
    const auto* Listener=UGameplayStatics::GetPlayerCameraManager(this,0);
    if (Report && Listener && FVector::DistSquared(From,Listener->GetCameraLocation()) < FMath::Square(5500.f))
        UGameplayStatics::PlaySoundAtLocation(this,Report,From,IsTrooper() ? .06f : .38f,bAlien ? 1.45f : .88f);
}

void AContactResponder::ReceiveHit(int32 Damage,AContactResponder* Shooter)
{
    if (!HasAuthority() || !Health || Shooter->bAlien==bAlien) return;
    Health=FMath::Max(0,Health-Damage);
    if (IsTrooper())
    {
        HideUntil=GetWorld()->GetTimeSeconds()+FMath::FRandRange(.65f,1.5);
        if (!bBoss)
        {
            GreyPauseUntil=0;  // shooting immediately breaks the uncanny stare/reach
            bGreyReachPending=false;
            NextGreyBeat=GetWorld()->GetTimeSeconds()+3.0;
        }
    }
    if (bBoss && Shooter && !Shooter->IsTrooper())
    {
        HideUntil=GetWorld()->GetTimeSeconds()+.33; // short interrupt, not permanent stun
        NextMindAttack=FMath::Max(NextMindAttack,HideUntil+.3);
    }
    UE_LOG(LogTemp,Display,TEXT("BATTLE hit %s[%d] -> %s[%d] hp=%d"),Shooter->bAlien?TEXT("alien"):TEXT("unit"),Shooter->Slot,bAlien?TEXT("alien"):TEXT("unit"),Slot,Health);
    if (!Health)
    {
        if (bBoss)
            if (auto* State=GetWorld()->GetGameState<AContactState>()) State->bBossDefeated=true;
        GetCharacterMovement()->DisableMovement();
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (bAlien)
            TeleportFlash(false); // Flash-and-poof instead of an alien corpse.
        else
        {
            GetMesh()->SetRelativeRotation(FRotator(0,-90,85));
            GetMesh()->bPauseAnims=true;
        }
    }
}

AContactMode::AContactMode()
{
    GameStateClass=AContactState::StaticClass();
    DefaultPawnClass=AContactResponder::StaticClass();
    HUDClass=AContactHUD::StaticClass();
    PrimaryActorTick.bCanEverTick=true;
}
void AContactMode::InitGame(const FString& Map,const FString& Options,FString& Error)
{
    Super::InitGame(Map,Options,Error);
    GetWorld()->SpawnActor<AContactField>();
    auto* Start=GetWorld()->SpawnActor<APlayerStart>(FVector(-2190,-1570,100),FRotator(0,15,0));
    if (Start) Start->PlayerStartTag=TEXT("AlliedDeployment");
}
AActor* AContactMode::ChoosePlayerStart_Implementation(AController* Player)
{
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
        if (It->PlayerStartTag==TEXT("AlliedDeployment")) return *It;
    return Super::ChoosePlayerStart_Implementation(Player);
}
void AContactMode::RestartPlayer(AController* NewPlayer)
{
    Super::RestartPlayer(NewPlayer);
    if (auto* Pawn=Cast<AContactResponder>(NewPlayer->GetPawn()))
    {
        const bool bSolo=GetNetMode()==NM_Standalone &&
            !UGameplayStatics::ParseOption(OptionsString,TEXT("Mission")).Equals(
                TEXT("Relay"),ESearchCase::IgnoreCase);
        if (bSolo) Pawn->BeginSoloHuntWithPistol();
        UE_LOG(LogTemp,Display,TEXT("BATTLE deployed hp=%d ammo=%d position=%s"),Pawn->Health,Pawn->Ammo,*Pawn->GetActorLocation().ToCompactString());
    }
}
void AContactMode::StartPlay()
{
    Super::StartPlay();
    auto* State=GetGameState<AContactState>();
    State->bHunt=GetNetMode()==NM_Standalone && !UGameplayStatics::ParseOption(OptionsString,TEXT("Mission")).Equals(TEXT("Relay"),ESearchCase::IgnoreCase);
    if (State->bHunt)
    {
        State->Allies=0;
        State->Raiders=0;
        State->IntroStage=0;
        IntroStageStarted=GetWorld()->GetTimeSeconds();
        bCrashImpactPlayed=false;
        FallingSaucer=nullptr;
        SaucerVisual=nullptr;
        AbductorSaucer=nullptr;
        AbductorBeam=nullptr;
        AbductorGlow=nullptr;
        bReinforcementsSpawned=false;
        State->bDogTaken=false;
        // Start the player in a short, fake military-shooter vignette.
        // The HUD shows an original arcade parody until the dog interrupts.
        // Temporary in-engine dog silhouette: a production quadruped model/rig is still needed.
        StoryDog=GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),FVector(-1630,-1650,75),FRotator::ZeroRotator);
        if (StoryDog)
        {
            auto* DogRoot=NewObject<USceneComponent>(StoryDog,TEXT("StoryDogRoot"));
            StoryDog->SetRootComponent(DogRoot);
            DogRoot->RegisterComponent();
            StoryDog->SetActorLocation(FVector(-1630,-1650,75));
            // A real skeletal dog is preferred over the placeholder body.
            if (USkeletalMesh* DogAsset=ContactAssets::Dog())
            {
                USkeletalMeshComponent* DogMesh=NewObject<USkeletalMeshComponent>(
                    StoryDog,TEXT("ImportedDogMesh"));
                DogMesh->SetupAttachment(DogRoot);
                DogMesh->SetSkeletalMesh(DogAsset);
                DogMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                DogMesh->SetRelativeRotation(FRotator(0,-90,0));
                const float Height=FMath::Max(1.f,DogAsset->GetBounds().BoxExtent.Z*2.f);
                DogMesh->SetRelativeScale3D(FVector(65.f/Height));
                DogMesh->RegisterComponent();
                UE_LOG(LogTemp,Display,TEXT("INTRO real dog model loaded: %s"),*DogAsset->GetPathName());
            }
            else
            {
            auto AddPart=[&](const TCHAR* Name,FVector Offset,FVector Scale)
            {
                auto* Mesh=NewObject<UStaticMeshComponent>(StoryDog,Name);
                Mesh->SetupAttachment(StoryDog->GetRootComponent());
                Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere")));
                Mesh->SetRelativeLocation(Offset);
                Mesh->SetRelativeScale3D(Scale);
                Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Mesh->RegisterComponent();
            };
            AddPart(TEXT("DogBody"),FVector(0,0,30),FVector(.5f,.21f,.23f));
            AddPart(TEXT("DogHead"),FVector(38,0,44),FVector(.22f,.20f,.19f));
            AddPart(TEXT("DogSnout"),FVector(57,0,39),FVector(.17f,.11f,.10f));
            for (int S : {-1,1})
            {
                AddPart(S<0?TEXT("DogFrontL"):TEXT("DogFrontR"),FVector(24,S*13,14),FVector(.08f,.08f,.25f));
                AddPart(S<0?TEXT("DogBackL"):TEXT("DogBackR"),FVector(-24,S*13,14),FVector(.08f,.08f,.25f));
                AddPart(S<0?TEXT("DogEarL"):TEXT("DogEarR"),FVector(35,S*15,60),FVector(.1f,.07f,.16f));
            }
            AddPart(TEXT("DogTail"),FVector(-54,0,48),FVector(.25f,.06f,.06f));
            } // fallback geometry only when no imported dog asset exists
        }
        UE_LOG(LogTemp,Display,TEXT("INTRO home staged; dog=%s"),StoryDog?TEXT("spawned"):TEXT("missing"));
        return;
    }
    for(int32 Side=0; Side<2; ++Side)
        for(int32 Slot=0; Slot<12; ++Slot)
        {
            const FVector Post=Barricade(Side!=0,Slot);
            const FVector Start=Post+FVector((Side?1:-1)*440,195,100);
            auto* Unit=GetWorld()->SpawnActor<AContactResponder>(Start,FRotator(0,Side?180:0,0));
            if (ensure(Unit)) Unit->AssignPost(Side!=0,Slot);
        }
}
void AContactMode::StartFallingSaucer()
{
    if (FallingSaucer || !GetWorld()) return;
    UStaticMesh* Ship=ContactAssets::IntactUFO();
    if (!Ship) Ship=ContactAssets::CrashedUFO();
    if (!Ship)
    {
        UE_LOG(LogTemp,Warning,TEXT("INTRO UFO imported ship unavailable: using basic disc stand-in"));
        Ship=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder"));
    }
    if (!Ship) return;
    FallingSaucer=GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),
        SaucerFrom,FRotator(-12,27,0));
    if (!FallingSaucer) return;
    USceneComponent* Root=NewObject<USceneComponent>(FallingSaucer,TEXT("CrashShipRoot"));
    FallingSaucer->SetRootComponent(Root);
    Root->RegisterComponent();
    SaucerVisual=NewObject<UStaticMeshComponent>(FallingSaucer,TEXT("AnimatedSaucer"));
    SaucerVisual->SetupAttachment(Root);
    SaucerVisual->SetStaticMesh(Ship);
    SaucerVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    const FVector Extent=Ship->GetBounds().BoxExtent;
    const float Width=FMath::Max(1.f,2.f*FMath::Max(Extent.X,Extent.Y));
    SaucerVisual->SetRelativeScale3D(FVector(1300.f/Width));
    SaucerVisual->RegisterComponent();
    FallingSaucer->SetActorLocation(SaucerFrom);
    UE_LOG(LogTemp,Display,TEXT("INTRO falling UFO spawned mesh=%s"),*Ship->GetPathName());
}

void AContactMode::PlayCrashImpact()
{
    if (bCrashImpactPlayed || !GetWorld()) return;
    bCrashImpactPlayed=true;
    if (FallingSaucer)
    {
        // Prefer the user's crashed saucer as the wrecked landing appearance.
        if (SaucerVisual)
        {
            if (UStaticMesh* Crashed=ContactAssets::CrashedUFO())
            {
                SaucerVisual->SetStaticMesh(Crashed);
                const float Span=FMath::Max(1.f,2.f*FMath::Max(
                    Crashed->GetBounds().BoxExtent.X,Crashed->GetBounds().BoxExtent.Y));
                SaucerVisual->SetRelativeScale3D(FVector(1300.f/Span));
            }
        }
        FallingSaucer->SetActorLocation(SaucerImpact);
        FallingSaucer->SetActorRotation(FRotator(17,32,14));
    }
    // Generated low-frequency impact sound, no external downloads required.
    PlayIntroSound(this,SaucerImpact,false);
    // A short-lived light and radial physical debris mark the impact.
    if (APointLight* Flash=GetWorld()->SpawnActor<APointLight>(
        SaucerImpact+FVector(0,0,240),FRotator::ZeroRotator))
    {
        if (UPointLightComponent* Light=Flash->PointLightComponent.Get())
        {
            Light->SetIntensity(850000.f);
            Light->SetAttenuationRadius(6800.f);
            Light->SetLightColor(FLinearColor(.58f,.83f,1.f));
        }
        Flash->SetLifeSpan(1.2f);
    }
    // Persistent cold wreck light turns on only after actual impact.
    if (APointLight* GlowActor=GetWorld()->SpawnActor<APointLight>(
        SaucerImpact+FVector(0,0,140),FRotator::ZeroRotator))
    {
        if (UPointLightComponent* Glow=GlowActor->PointLightComponent.Get())
        {
            Glow->SetIntensity(3300.f);
            Glow->SetAttenuationRadius(1150.f);
            Glow->SetLightColor(FLinearColor(.18f,.64f,1.f));
        }
    }
    // Camera flash is mild so the intro doesn't override player control.
    if (APlayerController* PC=UGameplayStatics::GetPlayerController(this,0))
        if (PC->PlayerCameraManager)
            PC->PlayerCameraManager->StartCameraFade(0.f,.18f,.05f,
                FLinearColor(.75f,.85f,1.f),false,false);
    UE_LOG(LogTemp,Display,TEXT("INTRO UFO impacted: wreck visible at crater"));
}

void AContactMode::SetIntroStage(int32 NewStage)
{
    if (AContactState* State=GetGameState<AContactState>())
    {
        State->IntroStage=NewStage;
        IntroStageStarted=GetWorld()->GetTimeSeconds();
        UE_LOG(LogTemp,Display,TEXT("INTRO stage=%d"),NewStage);
        if (NewStage==2) StartFallingSaucer();
        if (NewStage==3) PlayCrashImpact();
    }
}

void AContactMode::StartAbductor()
{
    if (!GetWorld() || AbductorSaucer) return;
    UStaticMesh* Craft=ContactAssets::IntactUFO();
    if (!Craft) Craft=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder"));
    if (!Craft) { UE_LOG(LogTemp,Error,TEXT("ABDUCTION no UFO mesh available")); return; }
    const FVector DogWhere=StoryDog ? StoryDog->GetActorLocation() : StoryDogDestination;
    DogBeamStart=DogWhere;
    const FVector Entry=DogWhere+FVector(-1750,1350,2100);
    AbductorSaucer=GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),Entry,FRotator::ZeroRotator);
    if (!AbductorSaucer) return;
    USceneComponent* Root=NewObject<USceneComponent>(AbductorSaucer,TEXT("AbductorRoot"));
    AbductorSaucer->SetRootComponent(Root);
    Root->RegisterComponent();
    UStaticMeshComponent* Hull=NewObject<UStaticMeshComponent>(AbductorSaucer,TEXT("AbductorHull"));
    Hull->SetupAttachment(Root);
    Hull->SetStaticMesh(Craft);
    Hull->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    const float Width=FMath::Max(1.f,2.f*FMath::Max(Craft->GetBounds().BoxExtent.X,Craft->GetBounds().BoxExtent.Y));
    Hull->SetRelativeScale3D(FVector(1050.f/Width));
    Hull->RegisterComponent();
    AbductorBeam=NewObject<UStaticMeshComponent>(AbductorSaucer,TEXT("TractorBeam"));
    AbductorBeam->SetupAttachment(Root);
    AbductorBeam->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder")));
    AbductorBeam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AbductorBeam->SetVisibility(false);
    AbductorBeam->SetCastShadow(false);
    if (UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial")))
    {
        if (UMaterialInstanceDynamic* BeamMat=UMaterialInstanceDynamic::Create(Base,AbductorSaucer))
        {
            BeamMat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.10f,.85f,1.f));
            AbductorBeam->SetMaterial(0,BeamMat);
        }
    }
    AbductorBeam->RegisterComponent();
    AbductorGlow=NewObject<UPointLightComponent>(AbductorSaucer,TEXT("TractorGlow"));
    AbductorGlow->SetupAttachment(Root);
    AbductorGlow->SetLightColor(FLinearColor(.14f,.78f,1.f));
    AbductorGlow->SetIntensity(11000.f);
    AbductorGlow->SetAttenuationRadius(1800.f);
    AbductorGlow->RegisterComponent();
    AbductorSaucer->SetActorLocation(Entry);
    UE_LOG(LogTemp,Warning,TEXT("ABDUCTION second UFO appeared at %s"),*Entry.ToCompactString());
}

void AContactMode::TickAbduction(float Delta,double Since)
{
    if (!AbductorSaucer || !StoryDog) return;
    const FVector BeamCenter=DogBeamStart+FVector(0,0,1050);
    if (auto* State=GetGameState<AContactState>())
    {
        if (State->IntroStage==4)
        {
            // Second spacecraft sweeps in while the dog waits in the crater.
            const float T=FMath::Clamp(float(Since/3.2),0.f,1.f);
            const float Ease=T*T*(3.f-2.f*T);
            const FVector Entry=DogBeamStart+FVector(-1750,1350,2100);
            AbductorSaucer->SetActorLocation(FMath::Lerp(Entry,BeamCenter,Ease));
            AbductorSaucer->SetActorRotation(FRotator(0,75.f+T*220.f,0));
            if (T>=1.f) SetIntroStage(5);
        }
        else if (State->IntroStage==5)
        {
            // The beam connects ship to dog, then physically lifts dog up.
            if (AbductorBeam)
            {
                AbductorBeam->SetVisibility(true);
                const FVector Ship=AbductorSaucer->GetActorLocation();
                const FVector Dog=StoryDog->GetActorLocation();
                const FVector Mid=(Ship+Dog)*.5;
                const float Length=FVector::Distance(Ship,Dog);
                // Engine cylinder height is 200 cm, aligned on local Z.
                AbductorBeam->SetRelativeLocation(AbductorSaucer->GetActorTransform().InverseTransformPosition(Mid));
                AbductorBeam->SetRelativeScale3D(FVector(2.0f,2.0f,Length/200.f));
            }
            const float T=FMath::Clamp(float(Since/3.4),0.f,1.f);
            StoryDog->SetActorLocation(DogBeamStart+FVector(0,0,860*T*T));
            StoryDog->AddActorWorldRotation(FRotator(0,Delta*90.f,0));
            if (T>=1.f)
            {
                StoryDog->SetActorHiddenInGame(true);
                State->bDogTaken=true;
                SetIntroStage(6);
                if (auto* Pawn=Cast<AContactResponder>(
                    UGameplayStatics::GetPlayerPawn(this,0))) Pawn->BeginRescueMode();
                UE_LOG(LogTemp,Warning,TEXT("ABDUCTION DOG TAKEN - rescue objective active"));
            }
        }
        else if (State->IntroStage==6)
        {
            if (AbductorBeam) AbductorBeam->SetVisibility(false);
            const float T=FMath::Clamp(float(Since/3.0),0.f,1.f);
            AbductorSaucer->SetActorLocation(BeamCenter+FVector(
                -1300.f*T,1150.f*T,3300.f*T*T));
            AbductorSaucer->SetActorRotation(FRotator(-18.f*T,290.f+T*240.f,10.f*T));
            if (T>=1.f)
            {
                AbductorSaucer->SetActorHiddenInGame(true);
                AbductorSaucer->SetActorEnableCollision(false);
                SpawnStoryGreys();
                SetIntroStage(7);
            }
        }
    }
}

void AContactMode::SpawnReinforcements()
{
    if (bReinforcementsSpawned || !GetWorld()) return;
    bReinforcementsSpawned=true;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    int32 Count=0;
    for (int32 I=0;I<3;++I)
    {
        const FVector Where(2700.f+I*240.f,-3300.f+I*130.f,105.f);
        if (AContactResponder* Grey=GetWorld()->SpawnActor<AContactResponder>(
            AContactResponder::StaticClass(),Where,FRotator(0,90,0),Params))
        {
            Grey->AssignHunter(I+5);
            ++Count;
        }
    }
    UE_LOG(LogTemp,Warning,TEXT("ABDUCTION phase 4 reinforcements=%d"),Count);
}

void AContactMode::SpawnStoryGreys()
{
    auto* State=GetGameState<AContactState>();
    if (!State || !State->bHunt) return;
    int32 Spawned=0;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (int32 Index=0; Index<5; ++Index)
    {
        const FVector Start(1800+Index*140,-2050+Index*155,110);
        if (auto* Hunter=GetWorld()->SpawnActor<AContactResponder>(AContactResponder::StaticClass(),
            Start,FRotator(0,180,0),Params))
        {
            Hunter->AssignHunter(Index);
            ++Spawned;
        }
    }
    State->Raiders=Spawned;
    UE_LOG(LogTemp,Display,TEXT("INTRO grey sighting; spawned=%d"),Spawned);
}

void AContactMode::Tick(float Delta)
{
    Super::Tick(Delta);
    auto* State=GetGameState<AContactState>();
    if (!State || State->Result!=EContactResult::Active) return;
    auto* Player=Cast<AContactResponder>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Player) return;
    if (State->bHunt && State->IntroStage<7 && State->Result==EContactResult::Active)
    {
        const double Since=GetWorld()->GetTimeSeconds()-IntroStageStarted;
        const FVector Crash(3600,-2600,100);
        if (State->IntroStage==0)
        {
            if (Since>=7.0) { LastBarkAt=0; SetIntroStage(1); }
        }
        else if (State->IntroStage==1)
        {
            if (GetWorld()->GetTimeSeconds()-LastBarkAt>.78)
            {
                LastBarkAt=GetWorld()->GetTimeSeconds();
                if (StoryDog)
                {
                    PlayIntroSound(this,StoryDog->GetActorLocation(),true);
                    // Slight head-turn motion before running toward the crash.
                    StoryDog->SetActorRotation(FRotator(
                        0,25.f*FMath::Sin(float(Since)*5.f),0));
                }
            }
            if (Since>=4.0) SetIntroStage(2);
        }
        else if (State->IntroStage==2)
        {
            // Visible five-second flight: high over the house, then down into
            // the existing crater. Rotate and accelerate into the ground.
            const float T=FMath::Clamp(float(Since/5.0),0.f,1.f);
            const float Ease=T*T*(3.f-2.f*T);
            if (FallingSaucer)
            {
                FallingSaucer->SetActorLocation(FMath::Lerp(SaucerFrom,SaucerImpact,Ease));
                FallingSaucer->SetActorRotation(FRotator(-12.f+34.f*Ease,
                    27.f+540.f*T,6.f+13.f*Ease));
            }
            if (T>=1.f) SetIntroStage(3);
        }
        else if (State->IntroStage==3)
        {
            if (StoryDog)
            {
                const FVector Current=StoryDog->GetActorLocation();
                FVector Target=StoryDogDestination;
                Target.Z=90;
                const FVector Step=(Target-Current).GetSafeNormal2D()*Delta*520;
                if (FVector::Dist2D(Current,Target)>Step.Size())
                {
                    StoryDog->SetActorLocation(Current+Step);
                    StoryDog->SetActorRotation(Step.Rotation());
                }
            }
            // The abduction starts when the dog reaches the ship, not just
            // because the player wandered near the crash too early.
            if (!StoryDog || FVector::Dist2D(StoryDog->GetActorLocation(),StoryDogDestination)<180 || Since>20)
            {
                if (StoryDog)
                {
                    StoryDog->SetActorLocation(StoryDogDestination);
                    DogBeamStart=StoryDogDestination;
                }
                StartAbductor();
                SetIntroStage(4);
            }
        }
        else if (State->IntroStage>=4 && State->IntroStage<=6)
        {
            TickAbduction(Delta,Since);
        }
    }
    int32 Allies=0,Raiders=0;
    for (TActorIterator<AContactResponder> It(GetWorld()); It; ++It)
        if (It->IsTrooper() && It->Health>0) { if(It->IsAlien()) ++Raiders; else ++Allies; }
    State->Allies=Allies;
    State->Raiders=Raiders;
    // Phase-four pressure: first wave is already present, three more arrive
    // once the player has had a few seconds to register the loss.
    if (State->bHunt && State->IntroStage==7 && !bReinforcementsSpawned &&
        GetWorld()->GetTimeSeconds()-IntroStageStarted>=4.)
    {
        SpawnReinforcements();
        return; // re-count living raiders next tick before boss checks
    }
    if (!State->bHunt && FVector::Dist2D(Player->GetActorLocation(),FVector::ZeroVector)<400 && Player->Health)
        State->Uplink=FMath::Min(10.f,State->Uplink+Delta);
    if (!Player->Health) State->Result=EContactResult::Lost;
    else if (State->bHunt && State->IntroStage==7 && Raiders==0 && !State->bBossSpawned)
    {
        const FVector BossStart(350,1050,140);
        FActorSpawnParameters SpawnOptions;
        SpawnOptions.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (auto* Boss=GetWorld()->SpawnActor<AContactResponder>(AContactResponder::StaticClass(),
                 BossStart,FRotator(0,180,0),SpawnOptions))
        {
            Boss->AssignNordicBoss();
            State->bBossSpawned=true;
        }
        else UE_LOG(LogTemp,Error,TEXT("NORDIC BOSS spawn failed; mission remains active"));
    }
    else if (State->bHunt ? (State->IntroStage==7 && State->bBossDefeated) : (Raiders==0 || State->Uplink>=10))
        State->Result=EContactResult::Won;
    if (State->Result!=EContactResult::Active)
        UE_LOG(LogTemp,Display,TEXT("BATTLE end result=%d allies=%d raiders=%d uplink=%.1f"),int32(State->Result),Allies,Raiders,State->Uplink);
}
