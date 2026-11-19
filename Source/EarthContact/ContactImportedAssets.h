#pragma once

// Runtime discovery of user-imported models in /Game.
// This deliberately never chooses between ambiguous candidates; logs exact names.
// For packaged builds, add used assets as explicit cook references/Primary Assets.
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

namespace ContactAssets
{
    inline UObject* Configured(const TCHAR* Role, bool bSkeletal)
    {
        FString Path;
        const FString Filename=FPaths::ProjectConfigDir()/TEXT("ImportedAssets.ini");
        if (!GConfig || !GConfig->GetString(TEXT("EarthContact.ImportedAssets"),Role,Path,Filename) || Path.IsEmpty())
            return nullptr;
        UObject* Asset=StaticLoadObject(bSkeletal ? USkeletalMesh::StaticClass() : UStaticMesh::StaticClass(),
                                        nullptr,*Path);
        if (!Asset)
            UE_LOG(LogTemp,Warning,TEXT("CONTACT ASSET configured %s missing: %s"),Role,*Path);
        else UE_LOG(LogTemp,Display,TEXT("CONTACT ASSET configured %s => %s"),Role,*Path);
        return Asset;
    }

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
        if (auto* Asset=Cast<USkeletalMesh>(Configured(TEXT("Grey"),true))) return Asset;
        if (auto* Exact=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Greys/SK_GreyAlien.SK_GreyAlien"))) return Exact;
        return Cast<USkeletalMesh>(FindSingle(TEXT("/Game/Characters/Greys"),TEXT(""),true));
    }
    inline USkeletalMesh* Boss()
    {
        if (auto* Asset=Cast<USkeletalMesh>(Configured(TEXT("MonsterLeader"),true))) return Asset;
        if (auto* Mesh=Cast<USkeletalMesh>(FindSingle(TEXT("/Game/Characters/MonsterLeader"),TEXT(""),true))) return Mesh;
        return Cast<USkeletalMesh>(FindSingle(TEXT("/Game/Characters/Monsters"),TEXT(""),true));
    }
    inline UStaticMesh* CrashedUFO()
    {
        if (auto* Asset=Cast<UStaticMesh>(Configured(TEXT("CrashedUFO"),false))) return Asset;
        return Cast<UStaticMesh>(FindSingle(TEXT("/Game/Vehicles/UFOs/Crashed"),TEXT(""),false));
    }
    inline UStaticMesh* IntactUFO()
    {
        if (auto* Asset=Cast<UStaticMesh>(Configured(TEXT("IntactUFO"),false))) return Asset;
        return Cast<UStaticMesh>(FindSingle(TEXT("/Game/Vehicles/UFOs/Intact"),TEXT(""),false));
    }
}
