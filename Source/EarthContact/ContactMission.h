#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerState.h"
#include "ContactMission.generated.h"

UENUM()
enum class EContactResult : uint8 { Active, Extracted, Lost };

UCLASS()
class AContactState : public AGameStateBase
{
    GENERATED_BODY()
public:
    UPROPERTY(Replicated) int32 ScannedMask = 0;
    UPROPERTY(Replicated) EContactResult Result = EContactResult::Active;
    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

UCLASS()
class AContactMember : public APlayerState
{
    GENERATED_BODY()
public:
    // All responders belong to the same team and role in this mission.
    UPROPERTY(EditDefaultsOnly) FName Team = TEXT("Field Unit");
    UPROPERTY(EditDefaultsOnly) FName SurveyRole = TEXT("Surveyor");
    UPROPERTY(Replicated) int32 Evidence = 0;
    UPROPERTY(Replicated) int32 Health = 100;
    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

UCLASS()
class AContactResponder : public ACharacter
{
    GENERATED_BODY()
public:
    AContactResponder();
    void SetupPlayerInputComponent(UInputComponent* Input) override;
private:
    void Forward(float Value);
    void Right(float Value);
    void SprintOn();
    void SprintOff();
    UFUNCTION(Server, Reliable) void ServerSprint(bool Enabled);
    UFUNCTION(Server, Reliable) void ServerScan();
};

UCLASS()
class AContactStalker : public AActor
{
    GENERATED_BODY()
public:
    AContactStalker();
    void Tick(float Delta) override;
private:
    double NextHit = 0;
};

UCLASS()
class AContactField : public AActor
{
    GENERATED_BODY()
public:
    AContactField();
};

UCLASS()
class AContactMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AContactMode();
    void InitGame(const FString& Map, const FString& Options, FString& Error) override;
    void Tick(float Delta) override;
};

UCLASS()
class AContactHUD : public AHUD
{
    GENERATED_BODY()
public:
    void DrawHUD() override;
};
