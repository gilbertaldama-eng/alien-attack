#pragma once

// Runtime discovery of user-imported models in /Game.
// This deliberately never chooses between ambiguous candidates; logs exact names.
// For packaged builds, add used assets as explicit cook references/Primary Assets.
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"

namespace ContactAssets
{
    inline UObject* FindSingle(const TCHAR* Folder, const TCHAR* Token, bool bSkeletal)
    {
        FAssetRegistryModule& Module=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
        TArray<FAssetData> Assets;
        Module.Get().GetAssetsByPath(FName(Folder),Assets,true);
        TArray<FAssetData> Matches;
        for (const FAssetData& Data:Assets)
        {
            const FName ClassName=Data.AssetClassPath.GetAssetName();
            if (bSkeletal ? ClassName!=TEXT("SkeletalMesh") : ClassName!=TEXT("StaticMesh"))
                continue;
            if (Token && *Token && !Data.AssetName.ToString().Contains(Token,ESearchCase::IgnoreCase))
                continue;
            Matches.Add(Data);
        }
        if (Matches.Num()==1)
        {
            UObject* Model=Matches[0].GetAsset();
            UE_LOG(LogTemp,Display,TEXT("CONTACT ASSET selected %s"),*Matches[0].GetObjectPathString());
            return Model;
        }
        if (Matches.Num()>1)
        {
            UE_LOG(LogTemp,Warning,TEXT("CONTACT ASSET ambiguous folder %s (%d candidates); use a dedicated folder"),Folder,Matches.Num());
            for(const auto& Match:Matches)
                UE_LOG(LogTemp,Warning,TEXT("CONTACT ASSET candidate %s"),*Match.GetObjectPathString());
        }
        else UE_LOG(LogTemp,Warning,TEXT("CONTACT ASSET no model in %s with token %s"),Folder,Token?Token:TEXT("(any)"));
        return nullptr;
    }
    inline USkeletalMesh* Grey()
    {
        if (auto* Exact=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Greys/SK_GreyAlien.SK_GreyAlien"))) return Exact;
        return Cast<USkeletalMesh>(FindSingle(TEXT("/Game/Characters/Greys"),TEXT(""),true));
    }
    inline USkeletalMesh* Boss()
    {
        if (auto* Mesh=Cast<USkeletalMesh>(FindSingle(TEXT("/Game/Characters/MonsterLeader"),TEXT(""),true))) return Mesh;
        return Cast<USkeletalMesh>(FindSingle(TEXT("/Game/Characters/Monsters"),TEXT(""),true));
    }
    inline UStaticMesh* CrashedUFO()
    {
        return Cast<UStaticMesh>(FindSingle(TEXT("/Game/Vehicles/UFOs/Crashed"),TEXT(""),false));
    }
    inline UStaticMesh* IntactUFO()
    {
        return Cast<UStaticMesh>(FindSingle(TEXT("/Game/Vehicles/UFOs/Intact"),TEXT(""),false));
    }
}
