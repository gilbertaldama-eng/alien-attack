#include "ContactMission.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
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
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "ProceduralMeshComponent.h"

namespace
{
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

void AContactResponder::AssignPost(bool Alien, int32 Index)
{
    Slot = Index;
    bAlien = Alien;
    Health = Alien ? 110 : 100;
    if (Alien)
    {
        GetMesh()->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")));
        GetMesh()->SetRelativeScale3D({1.18,.8f,1.08});
        FitRaiderCarapace(this);
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
    GetMesh()->PlayAnimation(Moving ? JogPose : IdlePose,true);
}

void AContactResponder::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AContactResponder,Health);
    DOREPLIFETIME(AContactResponder,Ammo);
    DOREPLIFETIME(AContactResponder,bReloading);
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
    Input->BindAction("Retry",IE_Pressed,this,&AContactResponder::Retry);
}
void AContactResponder::Forward(float V) { AddMovementInput(GetActorForwardVector(),V); }
void AContactResponder::Right(float V) { AddMovementInput(GetActorRightVector(),V); }
void AContactResponder::SprintOn() { ServerSprint(true); }
void AContactResponder::SprintOff() { ServerSprint(false); }
void AContactResponder::ServerSprint_Implementation(bool On) { GetCharacterMovement()->MaxWalkSpeed = On ? 700 : 440; }
void AContactResponder::FireOn() { SetTrigger(true); }
void AContactResponder::FireOff() { SetTrigger(false); }
void AContactResponder::SetTrigger_Implementation(bool Down) { bTrigger = Down; }
void AContactResponder::Reload() { ServerReload(); }
void AContactResponder::ServerReload_Implementation()
{
    if (Health && Ammo < 30 && !bReloading && Active(GetWorld()))
    {
        bReloading = true;
        ReloadAt = GetWorld()->GetTimeSeconds() + 1.6;
    }
}
void AContactResponder::Retry()
{
    if (!Active(GetWorld()) && GetNetMode()==NM_Standalone)
        UGameplayStatics::OpenLevel(this,TEXT("/Engine/Maps/Entry"));
}

void AContactResponder::Think(double Now)
{
    const FVector Here = GetActorLocation();
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
    if (Moving != bJog) Animate(Moving);
    // The template rifle's barrel runs along local +Y. Keep its grip on the
    // animated palm while the muzzle follows the view or squad target.
    FVector AimDirection=GetBaseAimRotation().Vector();
    if (IsTrooper())
        AimDirection=Target.IsValid() ? (Target->GetActorLocation()+FVector(0,0,25)-Gun->GetComponentLocation()).GetSafeNormal() : GetActorForwardVector();
    Gun->SetWorldRotation(FRotationMatrix::MakeFromYZ(AimDirection,FVector::UpVector).Rotator());
    if (!HasAuthority()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (bReloading && Now >= ReloadAt)
    {
        Ammo=30;
        bReloading=false;
        if (!IsTrooper()) UE_LOG(LogTemp,Display,TEXT("BATTLE player reloaded ammo=%d"),Ammo);
    }
    if (IsTrooper())
    {
        if (Now >= NextThink) { Think(Now); NextThink=Now+FMath::FRandRange(.18f,.28f); }
        FVector Direction = Destination - GetActorLocation();
        if (Direction.Size2D()>35) AddMovementInput(Direction.GetSafeNormal2D(),1,true);
        auto* Enemy = Target.Get();
        FVector Facing = Enemy ? Enemy->GetActorLocation()-GetActorLocation() : Direction;
        if (!Facing.IsNearlyZero()) SetActorRotation(FRotator(0,Facing.Rotation().Yaw,0));
        if (Enemy && Enemy->Health && Now >= NextShot && Now>=HideUntil)
            Shoot(Enemy->GetActorLocation()+FVector(0,0,25),true);
        return;
    }
    if (bTrigger && !bReloading && Now>=NextShot && Controller)
    {
        FVector Eye; FRotator View;
        Controller->GetPlayerViewPoint(Eye,View);
        FHitResult Sight;
        FVector End=Eye+View.Vector()*14000;
        GetWorld()->LineTraceSingleByChannel(Sight,Eye,End,ECC_Visibility,FCollisionQueryParams(NAME_None,false,this));
        Shoot(Sight.bBlockingHit ? Sight.ImpactPoint : End,false);
    }
}

void AContactResponder::Shoot(const FVector& Aim, bool Scatter)
{
    if (bReloading) return;
    if (Ammo<=0) { ServerReload(); return; }
    --Ammo;
    if (!IsTrooper()) UE_LOG(LogTemp,Display,TEXT("BATTLE player fired ammo=%d position=%s"),Ammo,*GetActorLocation().ToCompactString());
    const double Now=GetWorld()->GetTimeSeconds();
    NextShot=Now+(Scatter ? FMath::FRandRange(.7f,1.45) : .13f);
    const FVector From=GetActorLocation()+FVector(0,0,45)+GetActorForwardVector()*38;
    FVector Dir=(Aim-From).GetSafeNormal();
    if (Scatter) Dir=FMath::VRandCone(Dir,FMath::DegreesToRadians(2.4f));
    FHitResult Hit;
    FVector End=From+Dir*14000;
    GetWorld()->LineTraceSingleByChannel(Hit,From,End,ECC_Visibility,FCollisionQueryParams(NAME_None,false,this));
    if (Hit.bBlockingHit) End=Hit.ImpactPoint;
    if (auto* Victim=Cast<AContactResponder>(Hit.GetActor()); Victim && Victim->bAlien!=bAlien)
        Victim->ReceiveHit(Scatter ? 9 : 34,this);
    ShotFX(Gun->GetComponentTransform().TransformPosition(FVector(0,51,9)),End,Hit.bBlockingHit);
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
    if (IsTrooper()) HideUntil=GetWorld()->GetTimeSeconds()+FMath::FRandRange(.65f,1.5);
    UE_LOG(LogTemp,Display,TEXT("BATTLE hit %s[%d] -> %s[%d] hp=%d"),Shooter->bAlien?TEXT("alien"):TEXT("unit"),Shooter->Slot,bAlien?TEXT("alien"):TEXT("unit"),Slot,Health);
    if (!Health)
    {
        GetCharacterMovement()->DisableMovement();
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetMesh()->SetRelativeRotation(FRotator(0,-90,85));
        GetMesh()->bPauseAnims=true;
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
    auto* Start=GetWorld()->SpawnActor<APlayerStart>(FVector(-2200,-520,100),FRotator(0,13,0));
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
        UE_LOG(LogTemp,Display,TEXT("BATTLE deployed hp=%d ammo=%d position=%s"),Pawn->Health,Pawn->Ammo,*Pawn->GetActorLocation().ToCompactString());
}
void AContactMode::StartPlay()
{
    Super::StartPlay();
    for(int32 Side=0; Side<2; ++Side)
        for(int32 Slot=0; Slot<12; ++Slot)
        {
            const FVector Post=Barricade(Side!=0,Slot);
            const FVector Start=Post+FVector((Side?1:-1)*440,195,100);
            auto* Unit=GetWorld()->SpawnActor<AContactResponder>(Start,FRotator(0,Side?180:0,0));
            if (ensure(Unit)) Unit->AssignPost(Side!=0,Slot);
        }
}
void AContactMode::Tick(float Delta)
{
    Super::Tick(Delta);
    auto* State=GetGameState<AContactState>();
    if (!State || State->Result!=EContactResult::Active) return;
    auto* Player=Cast<AContactResponder>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Player) return;
    int32 Allies=0,Raiders=0;
    for (TActorIterator<AContactResponder> It(GetWorld()); It; ++It)
        if (It->IsTrooper() && It->Health>0) { if(It->IsAlien()) ++Raiders; else ++Allies; }
    State->Allies=Allies;
    State->Raiders=Raiders;
    if (FVector::Dist2D(Player->GetActorLocation(),FVector::ZeroVector)<400 && Player->Health)
        State->Uplink=FMath::Min(10.f,State->Uplink+Delta);
    if (!Player->Health) State->Result=EContactResult::Lost;
    else if (Raiders==0 || State->Uplink>=10) State->Result=EContactResult::Won;
    if (State->Result!=EContactResult::Active)
        UE_LOG(LogTemp,Display,TEXT("BATTLE end result=%d allies=%d raiders=%d uplink=%.1f"),int32(State->Result),Allies,Raiders,State->Uplink);
}
