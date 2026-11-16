#include "ContactMission.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void AContactHUD::DrawHUD()
{
    Super::DrawHUD();
    auto* State=GetWorld()->GetGameState<AContactState>();
    auto* Player=Cast<AContactResponder>(GetOwningPawn());
    if (!State || !Player || !Canvas) return;
    const float W=Canvas->SizeX,H=Canvas->SizeY;
    const FLinearColor Ink(.015f,.025f,.033f,.86f), Ally(.22f,.86f,.92f), Red(1,.32f,.20f), Gold(1,.78f,.33f);
    DrawRect(Ink,20,20,340,94);
    DrawRect(Gold,20,20,4,94);
    DrawText(TEXT("ROSWELL    /    SIGNAL STATION"),Gold,37,29);
    DrawText(State->bHunt ? (State->bBossSpawned ? TEXT("THE NORDIC HAS ARRIVED  /  DEFEAT THE GIANT") : TEXT("Solo hunt  /  Clear the five small entities")) : TEXT("Hold the relay or eliminate all raiders"),FLinearColor::White,37,50,nullptr,1.15);
    DrawText(State->bHunt ? FString::Printf(TEXT("HOSTILES  %d     /     %s"),State->Raiders,State->bBossSpawned ? TEXT("BOSS PHASE") : TEXT("HUNT PHASE")) : FString::Printf(TEXT("UNIT  %02d       RAIDERS  %02d       RELAY  %d%%"),State->Allies,State->Raiders,FMath::RoundToInt(10*State->Uplink)),Ally,37,83);
    DrawRect(Ink,20,H-86,258,66);
    DrawText(FString::Printf(TEXT("HP  %03d"),Player->Health),Ally,36,H-78,nullptr,1.45);
    DrawText(Player->bReloading ? TEXT("RELOADING") : FString::Printf(TEXT("%s  %02d / %d"),Player->bPistol ? TEXT("PISTOL") : TEXT("RIFLE"),Player->Ammo,Player->bPistol ? 17 : 30),Gold,130,H-77);
    DrawRect(FLinearColor(.17f,.22f,.23f),36,H-43,224,6);
    DrawRect(Ally,36,H-43,224*Player->Health/100.f,6);
    DrawText(TEXT("WASD move   Shift sprint   1 rifle   2 pistol   LMB fire   R reload"),FLinearColor::White,20,H-18);
    const float X=W*.5f,Y=H*.5f;
    DrawLine(X-9,Y,X-3,Y,FLinearColor::White,1.5);
    DrawLine(X+3,Y,X+9,Y,FLinearColor::White,1.5);
    DrawLine(X,Y-8,X,Y-3,FLinearColor::White,1.5);
    DrawLine(X,Y+3,X,Y+8,FLinearColor::White,1.5);
    for(TActorIterator<AContactResponder> It(GetWorld());It;++It)
    {
        if (!It->IsTrooper() || !It->Health) continue;
        FVector2D S;
        if (GetOwningPlayerController()->ProjectWorldLocationToScreen(It->GetActorLocation()+FVector(0,0,State->bHunt ? 25 : 115),S,true))
        {
            const auto Color=It->IsAlien()?Red:Ally;
            DrawRect(Ink,S.X-8,S.Y,16,4);
            DrawRect(Color,S.X-8,S.Y,16*It->Health/(State->bHunt ? (It->IsBoss() ? AContactResponder::BossHP : AContactResponder::HunterHP) : (It->IsAlien()?110.f:100.f)),3);
            if (State->bHunt) DrawText(FString::FromInt(It->Health),Red,S.X-12,S.Y-15);
        }
    }
    FVector2D S;
    if (!State->bHunt && GetOwningPlayerController()->ProjectWorldLocationToScreen(FVector(0,0,430),S,true))
        DrawText(TEXT("RELAY"),Gold,S.X-15,S.Y);
    if (State->Result!=EContactResult::Active)
    {
        DrawRect(Ink,X-175,Y-52,350,104);
        DrawText(State->Result==EContactResult::Won?TEXT("STATION SECURED"):TEXT("RESPONDER DOWN"),Gold,X-147,Y-37,nullptr,2);
        DrawText(TEXT("Enter   /   Deploy again"),FLinearColor::White,X-145,Y+15,nullptr,1.2);
    }
}
