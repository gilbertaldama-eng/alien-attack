#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "ContactMission.generated.h"

class UAnimSequence;
class USoundBase;
class UPointLightComponent;
class UAudioComponent;
class UStaticMeshComponent;
class UCameraComponent;

UENUM()
enum class EContactResult : uint8 { Active, Won, Lost };

UCLASS()
class AContactState : public AGameStateBase
{
    GENERATED_BODY()
public:
    UPROPERTY(Replicated) EContactResult Result = EContactResult::Active;
    UPROPERTY(Replicated) int32 Allies = 12;
    UPROPERTY(Replicated) int32 Raiders = 12;
    UPROPERTY(Replicated) float Uplink = 0;
    bool bHunt = false;
    bool bBossSpawned = false;
    bool bBossDefeated = false;
    bool bDogTaken = false; // Solo Hunt: drives rescue objective/HUD
    // Solo: 0 game, 1 bark, 2 impact, 3 dog runs, 4 second UFO arrives,
    // 5 tractor beam, 6 escape, 7 combat / rescue objective.
    int32 IntroStage = 4;
    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

UCLASS()
class AContactResponder : public ACharacter
{
    GENERATED_BODY()
public:
    AContactResponder();
    void BeginPlay() override;
    void Tick(float Delta) override;
    void SetupPlayerInputComponent(UInputComponent* Input) override;
    void AssignPost(bool Alien, int32 Index);
    void BeginSoloHuntWithPistol();
    void BeginRescueMode();
    void AssignHunter(int32 Index);
    void AssignNordicBoss();
    void TeleportFlash(bool bArrival);
    static constexpr int32 HunterHP = 90; // 3-foot greys
    static constexpr int32 BossHP = 550;
    bool IsBoss() const { return bBoss; }
    bool IsAlien() const { return bAlien; }
    bool IsTrooper() const { return Slot != INDEX_NONE; }
    UPROPERTY(Replicated) int32 Health = 100;
    UPROPERTY(Replicated) int32 Ammo = 30;
    UPROPERTY(Replicated) bool bReloading = false;
    UPROPERTY(Replicated) bool bPistol = false;
    int32 RifleAmmo = 30;
    int32 PistolAmmo = 17;
    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    void Forward(float Value);
    void Right(float Value);
    void SprintOn();
    void SprintOff();
    void FireOn();
    void FireOff();
    void Reload();
    void EquipRifle();
    void EquipPistol();
    void Retry();
    void Think(double Now);
    void Shoot(const FVector& Aim, bool Scatter);
    void ReceiveHit(int32 Damage, AContactResponder* Shooter);
    void Animate(bool Moving);
    UFUNCTION(Server,Reliable) void ServerSprint(bool Enabled);
    UFUNCTION(Server,Reliable) void SetTrigger(bool Down);
    UFUNCTION(Server,Reliable) void ServerReload();
    UFUNCTION(Server,Reliable) void ServerEquip(bool Pistol);
    UFUNCTION(NetMulticast,Unreliable) void ShotFX(FVector_NetQuantize From, FVector_NetQuantize To, bool Hit);
    UPROPERTY() TObjectPtr<UAnimSequence> IdlePose;
    UPROPERTY() TObjectPtr<UAnimSequence> JogPose;
    UPROPERTY() TObjectPtr<USoundBase> Report;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Gun;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Sidearm;
    UPROPERTY() TObjectPtr<UCameraComponent> FirstPersonCamera;
    UPROPERTY() TObjectPtr<UCameraComponent> ThirdPersonCamera;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> FirstPersonRifle;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> FirstPersonPistol;
    bool bFirstPersonExperience = false;
    float FirstPersonStride = 0.f;
    float WeaponKick = 0.f;
    float TargetFOV = 92.f;
    void UpdateFirstPerson(float Delta);
    TWeakObjectPtr<AContactResponder> Target;
    FVector Destination = FVector::ZeroVector;
    int32 Slot = INDEX_NONE;
    bool bAlien = false;
    bool bBoss = false;
    // Solo Hunt Greys: unsettling pause, lateral stalking, and a telegraphed close touch.
    double NextGreyBeat = 0;
    double GreyPauseUntil = 0;
    bool bGreyReachPending = false;
    bool bGreyFlankLeft = false;
    // Deterministic worker roles assigned by index. Not independent thinkers.
    int32 GreyDirective = 0;
    int32 LastOverlordOrder = -1;
    double NextServoStep = 0;
    double ServoHoldUntil = 0;
    double BrainControlledUntil = 0;
    double NextMindAttack = 0;
    bool bTrigger = false;
    bool bJog = false;
    double NextShot = 0, NextThink = 0, ReloadAt = 0, HideUntil = 0;
};

UCLASS()
class AContactField : public AActor
{
    GENERATED_BODY()
public:
    AContactField();
    void BeginPlay() override;
};

UCLASS()
class AContactMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AContactMode();
    void InitGame(const FString& Map, const FString& Options, FString& Error) override;
    void StartPlay() override;
    AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    void RestartPlayer(AController* NewPlayer) override;
    void Tick(float Delta) override;
private:
    double IntroStageStarted = 0;
    UPROPERTY() TObjectPtr<AActor> StoryDog;
    UPROPERTY() TObjectPtr<AActor> FallingSaucer;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> SaucerVisual;
    UPROPERTY() TObjectPtr<AActor> AbductorSaucer;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> AbductorBeam;
    UPROPERTY() TObjectPtr<UPointLightComponent> AbductorGlow;
    FVector DogBeamStart=FVector::ZeroVector;
    bool bReinforcementsSpawned=false;
    void StartAbductor();
    void TickAbduction(float Delta,double Since);
    void SpawnReinforcements();
    FVector SaucerFrom=FVector(-4400,2000,4600);
    FVector SaucerImpact=FVector(3600,-2600,20);
    bool bCrashImpactPlayed=false;
    double LastBarkAt=0;
    void StartFallingSaucer();
    void PlayCrashImpact();
    UPROPERTY() TObjectPtr<UAudioComponent> BattleMusic;
    void StartBattleMusic();
    FVector StoryDogDestination = FVector(3200,-2550,120);
    void SetIntroStage(int32 NewStage);
    void SpawnStoryGreys();
};

UCLASS()
class AContactHUD : public AHUD
{
    GENERATED_BODY()
public:
    void DrawHUD() override;
};
