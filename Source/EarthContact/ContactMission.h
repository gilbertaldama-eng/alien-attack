#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "ContactMission.generated.h"

class UAnimSequence;
class USoundBase;

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
    void AssignHunter(int32 Index);
    void AssignNordicBoss();
    void TeleportFlash(bool bArrival);
    static constexpr int32 HunterHP = 90;
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
    TWeakObjectPtr<AContactResponder> Target;
    FVector Destination = FVector::ZeroVector;
    int32 Slot = INDEX_NONE;
    bool bAlien = false;
    bool bBoss = false;
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
};

UCLASS()
class AContactHUD : public AHUD
{
    GENERATED_BODY()
public:
    void DrawHUD() override;
};
