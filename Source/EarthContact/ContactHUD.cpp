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
    // Original in-universe shooter parody. No Call of Duty game files, art
    // or copyrighted audio are used. This is a staged opening overlay.
    if (State->bHunt && State->IntroStage<=1)
    {
        const double Now=GetWorld()->GetTimeSeconds();
        const FLinearColor Black(.018f,.024f,.032f,1.f);
        const FLinearColor Night(.08f,.12f,.14f,1.f);
        const FLinearColor Neon(.75f,.94f,.59f,1.f);
        DrawRect(Black,0,0,W,H);
        DrawRect(Night,55,62,W-110,H-143);
        DrawRect(FLinearColor(.15f,.20f,.20f,1.f),65,H*.58f,W-130,H*.23f);
        const float CenterX=W*.5f+FMath::Sin(Now*1.6)*W*.06f;
        const float CenterY=H*.47f+FMath::Cos(Now*2.3)*H*.025f;
        // Graphic target silhouettes in the fake game's crosshairs.
        for (int32 I=0;I<6;++I)
        {
            const float PX=120.f+(W-260.f)*I/5.f;
            const float PY=H*.55f+18.f*FMath::Sin(float(Now*.8+I));
            DrawRect(FLinearColor(.24f,.28f,.25f),PX,PY-42,26,58);
            DrawRect(FLinearColor(.24f,.28f,.25f),PX+4,PY-62,18,21);
        }
        DrawLine(CenterX-18,CenterY,CenterX-6,CenterY,Neon,2.f);
        DrawLine(CenterX+6,CenterY,CenterX+18,CenterY,Neon,2.f);
        DrawLine(CenterX,CenterY-18,CenterX,CenterY-6,Neon,2.f);
        DrawLine(CenterX,CenterY+6,CenterX,CenterY+18,Neon,2.f);
        DrawText(TEXT("DUTY CALLS  /  DUST WARFARE"),Neon,82,76,nullptr,1.7f);
        DrawText(TEXT("MISSION: HOLD THE LAST CHECKPOINT"),FLinearColor::White,84,117);
        DrawText(TEXT("WAVE 03       SCORE  00950"),Neon,92,H-170);
        DrawText(TEXT("PRIMARY: BLASTER  /  30"),FLinearColor::White,W-340,H-170);
        DrawText(TEXT("EARTHCONTACT  /  HOME  /  11:47 PM"),
            FLinearColor(.65f,.68f,.70f),55,H-56);
        if (State->IntroStage==1)
        {
            DrawRect(FLinearColor(.015f,.018f,.023f,.92f),W*.17f,H*.34f,W*.66f,H*.25f);
            const bool bBarkBeat=FMath::Fmod(float(Now*2.8),2.f)<1.f;
            DrawText(bBarkBeat ? TEXT("WOOF! WOOF! WOOF!") : TEXT("YOUR DOG IS BARKING OUTSIDE!"),
                FLinearColor(1.f,.68f,.34f),W*.21f,H*.40f,nullptr,1.55f);
            DrawText(TEXT("WHAT THE HELL WAS THAT?"),
                FLinearColor::White,W*.21f,H*.48f);
        }
        return;
    }
    const FLinearColor Ink(.015f,.025f,.033f,.86f), Ally(.22f,.86f,.92f), Red(1,.32f,.20f), Gold(1,.78f,.33f);
    DrawRect(Ink,20,20,340,94);
    DrawRect(Gold,20,20,4,94);
    DrawText(State->bHunt ? TEXT("EARTHCONTACT  /  DESERT ENCOUNTER") : TEXT("ROSWELL  /  SIGNAL STATION"),Gold,37,29);
    const TCHAR* IntroPrompt=State->IntroStage==2 ? TEXT("SOMETHING IS FALLING FROM THE SKY") :
        State->IntroStage==3 ? TEXT("THE SHIP HAS CRASHED  /  FOLLOW YOUR DOG") :
        State->IntroStage==4 ? TEXT("ANOTHER UFO  /  IT IS FOLLOWING YOUR DOG!") :
        State->IntroStage==5 ? TEXT("TRACTOR BEAM!  /  THEY ARE TAKING YOUR DOG!") :
        State->IntroStage==6 ? TEXT("DOG ABDUCTED  /  RESCUE HIM") : nullptr;
    DrawText(State->bHunt ? (IntroPrompt ? IntroPrompt :
        (State->bBossSpawned ? TEXT("MONSTER LEADER  /  FIND YOUR DOG") : State->bDogTaken ?
            TEXT("THEY TOOK YOUR DOG  /  FIGHT THROUGH THE GREYS") : TEXT("THE GREYS  /  SURVIVE"))) :
        TEXT("Hold the relay or eliminate all raiders"),FLinearColor::White,37,50,nullptr,1.15);
    DrawText(State->bHunt ? FString::Printf(TEXT("HOSTILES  %d     /     %s"),State->Raiders,State->IntroStage<7 ? TEXT("PROLOGUE") : State->bBossSpawned ? TEXT("BOSS PHASE") : TEXT("HUNT PHASE")) : FString::Printf(TEXT("UNIT  %02d       RAIDERS  %02d       RELAY  %d%%"),State->Allies,State->Raiders,FMath::RoundToInt(10*State->Uplink)),Ally,37,83);
    if (State->bHunt && State->bDogTaken)
    {
        DrawRect(FLinearColor(.13f,.025f,.025f,.90f),W*.5f-240,24,480,52);
        DrawText(TEXT("RESCUE OBJECTIVE: FIND YOUR DOG"),
            FLinearColor(1.f,.82f,.50f),W*.5f-210,36,nullptr,1.15f);
    }
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
