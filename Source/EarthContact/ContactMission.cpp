#include "ContactMission.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Components/DirectionalLightComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

namespace
{
    const FVector Stations[] = {{1700,-650,90}, {2850,850,90}, {850,1850,90}};
    const FVector Landing(0,0,90);
    const TCHAR* Labels[] = {TEXT("Telemetry mast"), TEXT("Crash debris"), TEXT("Weather station")};
    const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
    const TCHAR* Sphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");
    const TCHAR* Cone = TEXT("/Engine/BasicShapes/Cone.Cone");

    // Called only during actor construction. Engine primitives are 100 cm across.
    UStaticMeshComponent* Shape(AActor* Actor, FName Name, const TCHAR* Path, FVector Position, FVector Size, bool Solid = false)
    {
        auto* Mesh = Actor->CreateDefaultSubobject<UStaticMeshComponent>(Name);
        if (Actor->GetRootComponent()) Mesh->SetupAttachment(Actor->GetRootComponent());
        else Actor->SetRootComponent(Mesh);
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Path));
        Mesh->SetRelativeLocation(Position);
        Mesh->SetRelativeScale3D(Size);
        Mesh->SetCollisionEnabled(Solid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        return Mesh;
    }

    bool Within(const FVector& Position, const FVector& Target, float Radius)
    {
        return FVector::DistSquared(Position, Target) <= FMath::Square(Radius);
    }
}

void AContactState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AContactState, ScannedMask);
    DOREPLIFETIME(AContactState, Result);
}

void AContactMember::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AContactMember, Evidence);
    DOREPLIFETIME(AContactMember, Health);
}

AContactResponder::AContactResponder()
{
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->MaxWalkSpeed = 460;
    Shape(this, TEXT("FieldSuit"), Sphere, {0,0,-10}, {.49,.43,1.15});
    Shape(this, TEXT("Helmet"), Sphere, {0,0,61}, FVector(.42));
    auto* Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    Boom->SetupAttachment(RootComponent);
    Boom->SetRelativeLocation({0,0,60});
    Boom->TargetArmLength = 350;
    Boom->bUsePawnControlRotation = true;
    auto* Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
}

void AContactResponder::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis("Forward", this, &AContactResponder::Forward);
    Input->BindAxis("Right", this, &AContactResponder::Right);
    Input->BindAxis("Turn", this, &APawn::AddControllerYawInput);
    Input->BindAxis("Look", this, &APawn::AddControllerPitchInput);
    Input->BindAction("Sprint", IE_Pressed, this, &AContactResponder::SprintOn);
    Input->BindAction("Sprint", IE_Released, this, &AContactResponder::SprintOff);
    Input->BindAction("Scan", IE_Pressed, this, &AContactResponder::ServerScan);
}

void AContactResponder::Forward(float Value)
{
    AddMovementInput(FRotator(0,GetControlRotation().Yaw,0).Vector(), Value);
}

void AContactResponder::Right(float Value)
{
    AddMovementInput(FRotator(0,GetControlRotation().Yaw + 90,0).Vector(), Value);
}

void AContactResponder::SprintOn() { GetCharacterMovement()->MaxWalkSpeed = 730; ServerSprint(true); }
void AContactResponder::SprintOff() { GetCharacterMovement()->MaxWalkSpeed = 460; ServerSprint(false); }
void AContactResponder::ServerSprint_Implementation(bool Enabled)
{
    GetCharacterMovement()->MaxWalkSpeed = Enabled ? 730 : 460;
}

void AContactResponder::ServerScan_Implementation()
{
    auto* State = GetWorld()->GetGameState<AContactState>();
    auto* Member = GetPlayerState<AContactMember>();
    if (!Member || Member->Health <= 0 || !State || State->Result != EContactResult::Active) return;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Stations); ++Index)
    {
        const int32 Bit = 1 << Index;
        if (!(State->ScannedMask & Bit) && Within(GetActorLocation(), Stations[Index], 260))
        {
            State->ScannedMask |= Bit;
            ++Member->Evidence;
            UE_LOG(LogTemp, Display, TEXT("CONTACT scan=%d mask=%d"), Index, State->ScannedMask);
            break;
        }
    }
}

AContactField::AContactField()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SurveyRange")));
    auto* Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("DesertSun"));
    Sun->SetupAttachment(RootComponent);
    Sun->SetRelativeRotation(FRotator(-52,-35,0));
    Sun->SetLightColor(FLinearColor(1.f,.84f,.62f));
    Shape(this, TEXT("Basin"), Cube, {1200,550,-100}, {110,105,2}, true);
    Shape(this, TEXT("NorthRim"), Cube, {1200,5750,125}, {110,1,2.5}, true);
    Shape(this, TEXT("SouthRim"), Cube, {1200,-4650,125}, {110,1,2.5}, true);
    Shape(this, TEXT("EastRim"), Cube, {6650,550,125}, {1,105,2.5}, true);
    Shape(this, TEXT("WestRim"), Cube, {-4250,550,125}, {1,105,2.5}, true);
    Shape(this, TEXT("LandingPad"), Cube, {0,0,5}, {7,7,.1}, true);
    Shape(this, TEXT("Antenna"), Cube, Stations[0], {.2,.2,3.6});
    Shape(this, TEXT("Dish"), Sphere, Stations[0]+FVector(0,0,175), {1.5,1.5,.35});
    Shape(this, TEXT("Hull"), Cone, Stations[1], {2.8,1.3,1.8});
    Shape(this, TEXT("WeatherPod"), Sphere, Stations[2], {1.1,1.1,1.8});
    for (int32 Index = 0; Index < 18; ++Index)
    {
        const float Angle = 2 * PI * Index / 18;
        const FVector Center(1200 + 5300*FMath::Cos(Angle), 550 + 4900*FMath::Sin(Angle), 100);
        Shape(this, *FString::Printf(TEXT("Mesa%d"),Index), Cone, Center, {15,12,6.0 + Index%5});
    }
}

AContactStalker::AContactStalker()
{
    bReplicates = true;
    SetReplicateMovement(true);
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Origin")));
    Shape(this, TEXT("Carapace"), Sphere, {}, {.95,.65,.6});
    Shape(this, TEXT("Crest"), Cone, {0,0,45}, {.5,.45,.7});
}

void AContactStalker::Tick(float Delta)
{
    Super::Tick(Delta);
    auto* State = GetWorld()->GetGameState<AContactState>();
    if (!HasAuthority() || !State || State->Result != EContactResult::Active) return;
    AContactResponder* Target = nullptr;
    double Closest = TNumericLimits<double>::Max();
    for (TActorIterator<AContactResponder> It(GetWorld()); It; ++It)
    {
        auto* Member = It->GetPlayerState<AContactMember>();
        const double Distance = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
        if (Member && Member->Health > 0 && Distance < Closest)
        {
            Target = *It;
            Closest = Distance;
        }
    }
    if (!Target) return;
    const FVector Offset = Target->GetActorLocation() - GetActorLocation();
    const FVector Step = Offset.GetSafeNormal2D() * FMath::Min(Offset.Size2D(), 250.0 * Delta);
    AddActorWorldOffset(Step);
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextHit || !Within(GetActorLocation(), Target->GetActorLocation(), 150)) return;
    NextHit = Now + 1.25;
    auto* Victim = Target->GetPlayerState<AContactMember>();
    Victim->Health = FMath::Max(0, Victim->Health - 20);
    UE_LOG(LogTemp, Display, TEXT("CONTACT hit player=%d health=%d"), Victim->GetPlayerId(), Victim->Health);
    if (Victim->Health == 0) Target->GetCharacterMovement()->DisableMovement();
}

AContactMode::AContactMode()
{
    GameStateClass = AContactState::StaticClass();
    PlayerStateClass = AContactMember::StaticClass();
    DefaultPawnClass = AContactResponder::StaticClass();
    HUDClass = AContactHUD::StaticClass();
    PrimaryActorTick.bCanEverTick = true;
}

void AContactMode::InitGame(const FString& Map, const FString& Options, FString& Error)
{
    Super::InitGame(Map, Options, Error);
    auto* World = GetWorld();
    World->SpawnActor<AContactField>();
    for (int32 Index = 0; Index < 12; ++Index)
    {
        const FVector Location(-600 + 150*(Index%4), -450 + 150*(Index/4), 115);
        World->SpawnActor<APlayerStart>(Location, FRotator::ZeroRotator);
    }
    World->SpawnActor<AContactStalker>(FVector(3200,-1650,90), FRotator::ZeroRotator);
}

void AContactMode::Tick(float Delta)
{
    Super::Tick(Delta);
    auto* State = GetGameState<AContactState>();
    if (!State || State->Result != EContactResult::Active) return;
    int32 Responders = 0, Survivors = 0, OnPad = 0;
    for (TActorIterator<AContactResponder> It(GetWorld()); It; ++It)
    {
        auto* Member = It->GetPlayerState<AContactMember>();
        if (!Member || !It->GetController()) continue;
        ++Responders;
        if (Member->Health <= 0) continue;
        ++Survivors;
        if (Within(It->GetActorLocation(), Landing, 320)) ++OnPad;
    }
    if (Responders && !Survivors) State->Result = EContactResult::Lost;
    else if (State->ScannedMask == 7 && Survivors && OnPad == Survivors) State->Result = EContactResult::Extracted;
    if (State->Result != EContactResult::Active)
        UE_LOG(LogTemp, Display, TEXT("CONTACT end=%d survivors=%d"), int32(State->Result), Survivors);
}

void AContactHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* State = GetWorld()->GetGameState<AContactState>();
    if (!State || !Canvas) return;
    float Y = 28;
    auto Line = [&](const FString& Text) {
        DrawText(Text, FLinearColor::White, 30, Y, nullptr, 1.35f);
        Y += 25;
    };
    Line(TEXT("EARTH: CONTACT   /   ROSWELL SURVEY RANGE"));
    if (State->Result == EContactResult::Extracted) Line(TEXT("EXTRACTED   /   Evidence secured"));
    else if (State->Result == EContactResult::Lost) Line(TEXT("SIGNAL LOST   /   Team down"));
    else if (State->ScannedMask == 7) Line(TEXT("Return all surviving responders to the landing pad"));
    else Line(TEXT("WASD move   |   Mouse look   |   Shift sprint   |   E scan"));
    const APawn* Viewer = GetOwningPawn();
    auto Mark = [&](FVector Where, const FString& Name) {
        if (!Viewer) return;
        const int32 Metres = FMath::RoundToInt(FVector::Dist(Viewer->GetActorLocation(), Where) / 100);
        Line(FString::Printf(TEXT("%s   %d m"), *Name, Metres));
        FVector2D Screen;
        if (GetOwningPlayerController()->ProjectWorldLocationToScreen(Where + FVector(0,0,185), Screen, true))
            DrawText(Name, FLinearColor::Yellow, Screen.X, Screen.Y);
    };
    for (int32 Index = 0; Index < 3; ++Index)
    {
        if (State->ScannedMask & (1 << Index)) Line(FString(Labels[Index]) + TEXT("   [scanned]"));
        else Mark(Stations[Index], Labels[Index]);
    }
    if (State->ScannedMask == 7) Mark(Landing,TEXT("Extraction"));
    for (APlayerState* Player : State->PlayerArray)
        if (auto* Member = Cast<AContactMember>(Player))
            Line(FString::Printf(TEXT("%s   HP %d   Evidence %d"), *Member->GetPlayerName(), Member->Health, Member->Evidence));
}
