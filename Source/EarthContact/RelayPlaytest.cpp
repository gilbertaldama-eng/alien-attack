// Optional standalone playtest. Runs only when invoked through the developer console.
// Uses the ordinary player controller, keyboard bindings, collision and damage.
#if WITH_DEV_AUTOMATION_TESTS
#include "ContactMission.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"
#include "TimerManager.h"

namespace
{
    void Press(APlayerController* PC,FKey Key,EInputEvent Event)
    {
        PC->InputKey(FInputKeyEventArgs(nullptr,FInputDeviceId::CreateFromInternalId(0),Key,Event,FPlatformTime::Cycles64()));
    }
    void TestRelay(UWorld* World)
    {
        if (!World || World->GetNetMode()!=NM_Standalone) return;
        struct FRun { FTimerHandle Timer; int Stage=0; double Started=0; };
        auto Run=MakeShared<FRun>();
        Run->Started=World->GetTimeSeconds();
        World->GetTimerManager().SetTimer(Run->Timer,[World,Run]()
        {
            auto* PC=World->GetFirstPlayerController();
            auto* Player=PC ? Cast<AContactResponder>(PC->GetPawn()) : nullptr;
            auto* State=World->GetGameState<AContactState>();
            if (!Player || !State) return;
            const double Elapsed=World->GetTimeSeconds()-Run->Started;
            const FVector P=Player->GetActorLocation();
            if (State->Result!=EContactResult::Active || Elapsed>42)
            {
                Press(PC,EKeys::W,IE_Released);
                Press(PC,EKeys::LeftShift,IE_Released);
                UE_LOG(LogTemp,Display,TEXT("RELAY TEST %s: result=%d uplink=%.1f hp=%d allies=%d raiders=%d position=%s"),
                    State->Result==EContactResult::Won && State->Uplink>=10 ? TEXT("PASS") : TEXT("FAIL"),
                    int32(State->Result),State->Uplink,Player->Health,State->Allies,State->Raiders,*P.ToCompactString());
                FScreenshotRequest::RequestScreenshot(TEXT("RelayResult.png"),false,false);
                World->GetTimerManager().ClearTimer(Run->Timer);
                return;
            }
            if (Run->Stage==0 && Elapsed>1)
            {
                UE_LOG(LogTemp,Display,TEXT("RELAY TEST opening: hp=%d allies=%d raiders=%d"),Player->Health,State->Allies,State->Raiders);
                FScreenshotRequest::RequestScreenshot(TEXT("RelayOpening.png"),false,false);
                PC->SetControlRotation(FRotator(0,90,0));
                Press(PC,EKeys::LeftShift,IE_Pressed);
                Press(PC,EKeys::W,IE_Pressed);
                ++Run->Stage;
            }
            else if(Run->Stage==1 && P.Y > -30)
            {
                PC->SetControlRotation(FRotator::ZeroRotator);
                ++Run->Stage;
            }
            else if(Run->Stage==2 && P.X > -190)
            {
                Press(PC,EKeys::W,IE_Released);
                Press(PC,EKeys::LeftShift,IE_Released);
                ++Run->Stage;
            }
        },.05f,true);
    }
    FAutoConsoleCommandWithWorld Cmd(TEXT("Contact.TestRelay"),TEXT("Run standalone relay acceptance route"),
        FConsoleCommandWithWorldDelegate::CreateStatic(&TestRelay));
}
#endif
