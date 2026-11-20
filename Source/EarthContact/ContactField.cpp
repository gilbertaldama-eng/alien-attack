#include "ContactMission.h"
#include "ContactImportedAssets.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

AContactField::AContactField()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Station")));
    auto* Sun=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Afternoon"));
    Sun->SetupAttachment(RootComponent);
    Sun->SetRelativeRotation(FRotator(-8,-115,0)); // low dusk sun, long desert shadows
    Sun->SetAtmosphereSunLight(true);
    Sun->SetIntensity(2.0f);
    Sun->SetLightColor(FLinearColor(1.f,.61f,.38f));
    auto* Sky=CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
    Sky->SetupAttachment(RootComponent);
    Sky->SetRelativeLocation(FVector(0,0,-150));
    auto* Fill=CreateDefaultSubobject<USkyLightComponent>(TEXT("Skylight"));
    Fill->SetupAttachment(RootComponent);
    Fill->SetIntensity(.48f);
    Fill->SetRealTimeCaptureEnabled(true);
    auto* Haze=CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("DustHaze"));
    Haze->SetupAttachment(RootComponent);
    Haze->SetFogDensity(.019f);
    Haze->SetFogHeightFalloff(.17f);
    Haze->SetVolumetricFog(true);
    Haze->SetFogInscatteringColor(FLinearColor(.21f,.27f,.34f));
}

void AContactField::BeginPlay()
{
    Super::BeginPlay();
    // Standalone Solo Hunt is a lonely desert homestead, NOT a relay shooting range.
    // The old station and barricades survive in the explicit Relay Battle mode.
    AGameModeBase* Mode=GetWorld()->GetAuthGameMode();
    const bool bSolo=GetNetMode()==NM_Standalone &&
        (!Mode || !UGameplayStatics::ParseOption(Mode->OptionsString,TEXT("Mission"))
                 .Equals(TEXT("Relay"),ESearchCase::IgnoreCase));
    UE_LOG(LogTemp,Display,TEXT("WORLD atmosphere=%s"),bSolo?TEXT("Desert Encounter"):TEXT("Relay Battle"));
    // Natural desert ridges with a shallow eroded wash and a subdued traversal corridor.
    auto* Terrain=NewObject<UProceduralMeshComponent>(this);
    Terrain->SetupAttachment(RootComponent);
    Terrain->SetCollisionProfileName(TEXT("BlockAll"));
    Terrain->RegisterComponent();
    TArray<FVector> V,N;
    TArray<FVector2D> UV;
    TArray<FColor> Colors;
    TArray<int32> Indices;
    // 170 cm sampling resolves the crater edge and drainage channels without
    // creating imported assets or editing any user-owned Content Browser meshes.
    constexpr int32 Grid=161;
    constexpr double Step=170.;
    const FVector2D HouseXY(-2200.,-1600.);
    const FVector2D CrashXY(3600.,-2600.);
    auto Height=[bSolo,HouseXY,CrashXY](double X,double Y) -> double {
        const FVector2D P(X,Y);
        const double HouseDistance=FVector2D::Distance(P,HouseXY);
        if (!bSolo)
        {
            // Preserve the original level's relay-station footprint and cover.
            const double Edge=FMath::Clamp((FMath::Max(FMath::Abs(X),FMath::Abs(Y))-2450.)/2800.,0.,1.);
            const double Hills=FMath::PerlinNoise2D(P*.00029+FVector2D(63,19));
            const double Grain=FMath::PerlinNoise2D(P*.0016);
            return -3.+Edge*(350.+1750.*FMath::Abs(Hills)+105.*Grain);
        }
        // Protect the player's home, porch and opening path from steep spikes.
        const double HouseBlend=FMath::Clamp((HouseDistance-650.)/1200.,0.,1.);
        const double CrashDistance=FVector2D::Distance(P,CrashXY);
        const double CorridorX=FMath::Clamp((X-HouseXY.X)/(CrashXY.X-HouseXY.X),0.,1.);
        const FVector2D PathCenter=FMath::Lerp(HouseXY,CrashXY,CorridorX);
        const double PathDistance=FVector2D::Distance(P,PathCenter);
        const double CorridorBlend=FMath::Clamp((PathDistance-400.)/1050.,0.,1.);
        // Layered low-frequency erosion is actual ground displacement, not a
        // color/lighting trick. Ridges surround the player's navigable valley.
        const double RidgeNoise=FMath::Abs(FMath::PerlinNoise2D(P*.00024+FVector2D(37.,71.)));
        const double MesaNoise=FMath::Abs(FMath::PerlinNoise2D(P*.00058+FVector2D(12.,91.)));
        const double DetailNoise=FMath::PerlinNoise2D(P*.0021+FVector2D(4.,27.));
        const double Outer=FMath::Clamp((FMath::Max(FMath::Abs(X),FMath::Abs(Y))-1900.)/5900.,0.,1.);
        const double Ridges=Outer*(230.+1050.*RidgeNoise+430.*MesaNoise);
        const double Undulations=35.*DetailNoise+70.*MesaNoise;
        const double Valley=HouseBlend*(CorridorBlend*Ridges+Undulations*.45);
        // A slightly meandering dry arroyo cuts toward the crash.
        const double WashCenterY=-1670.-.16*(X+2200.)+170.*FMath::Sin(X*.00061);
        const double WashDistance=FMath::Abs(Y-WashCenterY);
        const double Wash=-65.*FMath::Square(FMath::Clamp(1.-WashDistance/690.,0.,1.))*HouseBlend;
        // Real impact depression and piled-up raised ring. Keep the wreck's
        // center traversable and avoid blocking the enemy approach.
        const double CraterBowl=-145.*FMath::Square(FMath::Clamp(1.-CrashDistance/940.,0.,1.));
        const double RimDistance=FMath::Abs(CrashDistance-1110.);
        const double CraterRim=105.*FMath::Square(FMath::Clamp(1.-RimDistance/470.,0.,1.));
        return -3.+Valley+Wash+CraterBowl+CraterRim;
    };
    for (int32 Row=0; Row<Grid; ++Row)
        for(int32 Col=0; Col<Grid; ++Col)
        {
            const double X=(Col-Grid/2)*Step,Y=(Row-Grid/2)*Step;
            V.Add({X,Y,Height(X,Y)});
            N.Add(FVector(Height(X-10,Y)-Height(X+10,Y),Height(X,Y-10)-Height(X,Y+10),20).GetSafeNormal());
            UV.Add({Col/4.,Row/4.});
            const double Gradient=FMath::Abs(Height(X+100,Y)-Height(X-100,Y))
                                  +FMath::Abs(Height(X,Y+100)-Height(X,Y-100));
            const float Dust=FMath::Clamp(.5f+.42f*FMath::PerlinNoise2D(FVector2D(X,Y)*.0012f),0.f,1.f);
            const FLinearColor Sand=FLinearColor::LerpUsingHSV(
                FLinearColor(.24f,.105f,.052f),FLinearColor(.60f,.37f,.20f),Dust);
            const FLinearColor RockTint(.20f,.16f,.13f);
            const float RockAmount=bSolo ? FMath::Clamp(float((Gradient-58.)/200.),0.f,.8f) : 0.f;
            Colors.Add(FLinearColor::LerpUsingHSV(Sand,RockTint,RockAmount).ToFColor(true));
            if(Row==Grid-1 || Col==Grid-1) continue;
            const int32 I=Row*Grid+Col;
            Indices.Append({I,I+Grid,I+1,I+1,I+Grid,I+Grid+1});
        }
    Terrain->CreateMeshSection(0,V,Indices,N,UV,Colors,{},true);
    UMaterialInterface* SandMaterial=LoadObject<UMaterialInterface>(
        nullptr,TEXT("/Game/Field/M_ArroyoSand.M_ArroyoSand"));
    if (!SandMaterial)
    {
        // Previously the code silently assigned a missing material to the
        // landscape. Provide a visible engine material instead of null.
        SandMaterial=LoadObject<UMaterialInterface>(
            nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        UE_LOG(LogTemp,Warning,TEXT("WORLD missing /Game/Field/M_ArroyoSand; using visible fallback material"));
    }
    Terrain->SetMaterial(0,SandMaterial);
    UE_LOG(LogTemp,Display,TEXT("WORLD terrain grid=%dx%d, SoloHunt=%d, crater=3600,-2600"),
        Grid,Grid,bSolo);

    auto* Paint=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
    auto Batch=[&](const TCHAR* Path,FLinearColor Color,bool Solid) {
        auto* BatchMesh=NewObject<UInstancedStaticMeshComponent>(this);
        BatchMesh->SetupAttachment(RootComponent);
        BatchMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Path));
        auto* Mat=UMaterialInstanceDynamic::Create(Paint,this);
        Mat->SetVectorParameterValue(TEXT("Color"),Color);
        BatchMesh->SetMaterial(0,Mat);
        BatchMesh->SetCollisionProfileName(Solid?TEXT("BlockAll"):TEXT("NoCollision"));
        BatchMesh->RegisterComponent();
        return BatchMesh;
    };
    const TCHAR* Cube=TEXT("/Engine/BasicShapes/Cube");
    auto* Concrete=Batch(Cube,{.40f,.32f,.23f},true);
    auto* Metal=Batch(Cube,{.13f,.21f,.20f},true);
    auto* Glass=Batch(Cube,{.035f,.12f,.15f},true);
    auto* Ochre=Batch(Cube,{.73f,.33f,.066f},false);
    auto* Mast=Batch(TEXT("/Engine/BasicShapes/Cylinder"),{.28f,.33f,.32f},true);
    auto* Rocks=Batch(TEXT("/Game/SimSandbox/Meshes/Rock"),{.33f,.18f,.10f},false);
    auto Put=[](UInstancedStaticMeshComponent* Mesh,FVector P,FVector Size,FRotator Rotation=FRotator::ZeroRotator) {
        Mesh->AddInstance(FTransform(Rotation,P,Size));
    };
    if (!bSolo)
    {
    for (int32 Side : {-1,1})
        for(int32 Index=0; Index<12; ++Index)
        {
            const FVector P(Side*(600+670*(Index/6)),(Index%6-2.5)*490,0);
            Put(Concrete,P+FVector(0,0,78),{.65f,2.05,1.56});
            Put(Concrete,P+FVector(0,0,20),{1.02,2.22,.4f});
            Put(Ochre,P+FVector(0,0,137),{.68f,2.08,.13f});
        }
    // Radio huts, corrugated roofs, inset windows and structural ribs frame the lanes.
    for(int32 Side : {-1,1})
        for(int32 i=0; i<3; ++i)
        {
            const FVector P((i-1)*1320,Side*2050,0);
            Put(Concrete,P+FVector(0,0,157),{9,4,3.14});
            Put(Metal,P+FVector(0,0,329),{9.6,4.5,.30f});
            for(int32 j=-2;j<=2;++j)
            {
                Put(Glass,P+FVector(j*158,-Side*205,178),{1.22,.12f,.83f});
                Put(Metal,P+FVector(j*158-77,-Side*212,150),{.09f,.21f,3.0});
            }
            Put(Mast,P+FVector(230,15,419),{.17f,.17f,1.62});
        }
    Put(Mast,{0,0,196},{.32f,.32f,3.92});
    Put(Metal,{0,0,50},{.96f,.85f,1.0});
    for(int i=0;i<3;++i) Put(Ochre,{0,0,280.+i*55},{1.9-i*.38f,.14f,.11f});
    // Relay revetment opens to the allied approach and shields the terminal.
    Put(Concrete,{138,0,105},{.78,6.8,2.1});
    for (int Sign : {-1,1}) Put(Concrete,{55,Sign*329.,88},{2.4,.45,1.76});
    // The receiving dish gives the relay station a recognizable silhouette from spawn.
    const FVector Receiver(2700,480,830);
    Put(Mast,{2700,480,390},{1.1,1.1,7.8});
    for(int Sign : {-1,1})
        Put(Metal,{2700,480.+Sign*148,210},{.32f,.32f,4.8},FRotator(0,0,Sign*26));
    auto* Dish=NewObject<UProceduralMeshComponent>(this);
    Dish->SetupAttachment(RootComponent);
    Dish->SetRelativeLocationAndRotation(Receiver,FRotator(0,0,-48));
    Dish->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Dish->RegisterComponent();
    V.Reset(); N.Reset(); UV.Reset(); Indices.Reset(); Colors.Reset();
    for(int Ring=0;Ring<=8;++Ring)
        for(int Sector=0;Sector<=48;++Sector)
        {
            const double R=Ring/8., A=2*PI*Sector/48.;
            V.Add({470*R*FMath::Cos(A),470*R*FMath::Sin(A),154*R*R});
            N.Add(FVector(-.66f*R*FMath::Cos(A),-.66f*R*FMath::Sin(A),1).GetSafeNormal());
            UV.Add({Sector/48.,R});
            if(Ring<8 && Sector<48)
            {
                const int K=Ring*49+Sector;
                Indices.Append({K,K+1,K+49,K+1,K+50,K+49,K,K+49,K+1,K+1,K+49,K+50});
            }
        }
    Dish->CreateMeshSection(0,V,Indices,N,UV,Colors,{},false);
    Dish->SetMaterial(0,Concrete->GetMaterial(0));
    Put(Metal,{2700,520,1015},{.11f,.11f,4.8},FRotator(0,0,-29));
    Put(Mast,{2700,601,1200},{.44f,.44f,.71f});
    } // End relay-only station scenery; don't spawn a firing range in Solo Hunt.

    // Sky silhouettes for the crash-site nightmare. These are lightweight procedural
    // primitives until rigged, art-directed meshes and materials replace them.
    auto* VoidMetal=Batch(TEXT("/Engine/BasicShapes/Sphere"),{.012f,.015f,.024f},false);
    auto* Shadow=Batch(Cube,{.008f,.011f,.018f},false);
    auto* Chrome=Batch(TEXT("/Engine/BasicShapes/Sphere"),{.65f,.75f,.84f},false);
    auto* ChromeBars=Batch(Cube,{.60f,.70f,.78f},false);
    auto* Glow=Batch(TEXT("/Engine/BasicShapes/Sphere"),{.20f,.72f,.94f},false);
    // The separate black jellyfish craft and chrome witch remain intentionally unchanged.
    // User-imported intact and crashed saucers replace only their matching stand-ins.
    auto SpawnImported=[&](UStaticMesh* Asset, FVector Where, float DesiredWidthCm, FRotator Rotation)
    {
        if(!Asset) return false;
        auto* Mesh=NewObject<UStaticMeshComponent>(this);
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetStaticMesh(Asset);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        const FVector Bounds=Asset->GetBounds().BoxExtent;
        const float Width=FMath::Max(1.f,2.f*FMath::Max(Bounds.X,Bounds.Y));
        Mesh->SetRelativeScale3D(FVector(DesiredWidthCm/Width));
        Mesh->SetRelativeLocationAndRotation(Where,Rotation);
        Mesh->RegisterComponent();
        return true;
    };
    const bool bRealIntactUFO=SpawnImported(ContactAssets::IntactUFO(),
        FVector(1800,3400,1550),1500,FRotator(0,-15,0));
    // During Solo Hunt, the crash actor is spawned by AContactMode
    // *after* the player sees the descending saucer. Do not pre-place a wreck.
    const bool bRealCrashedUFO=!bSolo && SpawnImported(ContactAssets::CrashedUFO(),
        FVector(3600,-2600,140),1300,FRotator(0,19,14));
    UE_LOG(LogTemp,Display,TEXT("CONTACT UFOs intact=%d crashed=%d"),bRealIntactUFO,bRealCrashedUFO);
    // Black hovering jellyfish craft, visibly distinct from the grounded wreck.
    const FVector Jelly(-3400,1200,2300);
    Put(VoidMetal,Jelly,{9.2f,9.2f,1.65f});
    Put(VoidMetal,Jelly+FVector(0,0,-110),{6.f,6.f,1.7f});
    for (int32 I=0;I<12;++I)
    {
        const double A=2*PI*I/12.;
        const FVector Edge(Jelly.X+350*FMath::Cos(A),Jelly.Y+350*FMath::Sin(A),Jelly.Z-180);
        const float Length=FMath::FRandRange(7.f,14.f);
        Put(Shadow,Edge+FVector(0,0,-Length*50),{.18f,.18f,Length});
        Put(Glow,Edge+FVector(0,0,-Length*92),{.23f,.23f,.45f});
    }
    // A chrome broom-riding witch: high-altitude silhouette and unmistakable broom.
    const FVector Witch(2700,-1400,1700);
    Put(Chrome,Witch+FVector(0,0,90),{.42f,.42f,.8f}); // head
    Put(Chrome,Witch+FVector(0,0,-20),{.55f,.44f,1.3f}); // torso
    Put(ChromeBars,Witch+FVector(105,0,-120),{4.1f,.12f,.13f}); // broom shaft
    for (int32 J=-3;J<=3;++J)
        Put(ChromeBars,Witch+FVector(320,J*13,-118),{1.0f,.07f,.08f},FRotator(0,J*7,0));
    Put(Chrome,Witch+FVector(0,0,145),{1.2f,1.2f,.11f}); // wide-brim silhouette
    // A crashed circular craft in the outer desert, partly buried in sand.
    const FVector Wreck(3600,-2600,110);
    if (!bSolo && !bRealCrashedUFO)
    {
        Put(Metal,Wreck,{5.2f,5.2f,.35f},FRotator(0,19,14));
        Put(Chrome,Wreck+FVector(0,0,75),{3.4f,3.4f,.7f});
        Put(Glow,Wreck+FVector(130,0,-55),{.6f,.6f,.18f});
    }
    // Desert homestead: playable prologue begins inside a modest, roofed cabin.
    // East side has an open doorway, so the player can step out toward the crash.
    const FVector Home(-2200,-1600,0);
    auto* HouseWall=Batch(Cube,{.31f,.24f,.18f},true);
    auto* HouseRoof=Batch(Cube,{.18f,.14f,.12f},true);
    auto* HouseFloor=Batch(Cube,{.20f,.16f,.11f},true);
    auto* Porch=Batch(Cube,{.29f,.24f,.20f},true);
    Put(HouseFloor,Home+FVector(0,0,8),{6.6f,5.4f,.16f});
    Put(HouseWall,Home+FVector(-325,0,160),{.22f,5.4f,3.2f});
    Put(HouseWall,Home+FVector(0,-260,160),{6.6f,.20f,3.2f});
    Put(HouseWall,Home+FVector(0,260,160),{6.6f,.20f,3.2f});
    // Front wall split around a wide central door for reliable player clearance.
    for (int S : {-1,1}) Put(HouseWall,Home+FVector(325,S*190,160),{.22f,1.6f,3.2f});
    Put(HouseWall,Home+FVector(325,0,290),{.22f,2.3f,.6f});
    Put(HouseRoof,Home+FVector(0,0,340),{7.0f,5.9f,.25f});
    Put(Porch,Home+FVector(430,0,12),{2.0f,4.5f,.24f});
    Put(Concrete,Home+FVector(-140,125,67),{1.6f,.7f,1.2f}); // kitchen counter
    Put(Metal,Home+FVector(-130,-130,45),{1.5f,1.2f,.5f}); // old couch
    Put(Glass,Home+FVector(-320,-155,180),{.05f,1.1f,.85f}); // rear window

    if (bSolo)
    {
        // House practical light contrasts with the cold, distant crash glow.
        auto AddLocalLight=[&](const TCHAR* Name,FVector Location,FLinearColor Color,float Intensity,float Radius)
        {
            auto* Light=NewObject<UPointLightComponent>(this,Name);
            Light->SetupAttachment(RootComponent);
            Light->SetRelativeLocation(Location);
            Light->SetLightColor(Color);
            Light->SetIntensity(Intensity);
            Light->SetAttenuationRadius(Radius);
            Light->RegisterComponent();
        };
        AddLocalLight(TEXT("PorchLight"),Home+FVector(280,0,260),
            FLinearColor(1.f,.65f,.32f),900.f,850.f);
        AddLocalLight(TEXT("CrashAfterglow"),FVector(3600,-2600,190),
            FLinearColor(.18f,.62f,1.f),3400.f,1350.f);
        // Scraps and a worn dirt track replace geometric barriers in Solo Hunt.
        auto Track=Batch(Cube,{.20f,.14f,.10f},false);
        for (int I=0;I<38;++I)
        {
            const float T=(I+.5f)/38.f;
            const FVector P=FMath::Lerp(Home,FVector(3600,-2600,0),T);
            const float Drift=125.f*FMath::Sin(T*19.f);
            // Thin ground-level markings; never block player movement.
            Put(Track,FVector(P.X,P.Y+Drift,2),FVector(1.7f,2.8f,.018f),
                FRotator(0,-10,0));
        }
        auto Debris=Batch(Cube,{.10f,.11f,.14f},false);
        FRandomStream Scatter(1808);
        for (int I=0;I<75;++I)
        {
            const float Theta=Scatter.FRandRange(0.f,2.f*PI);
            const float Radius=Scatter.FRandRange(720.f,2100.f);
            const FVector P(3600.f+Radius*FMath::Cos(Theta),
                -2600.f+Radius*FMath::Sin(Theta),Scatter.FRandRange(8.f,50.f));
            Put(Debris,P,FVector(Scatter.FRandRange(.1f,.55f),
                Scatter.FRandRange(.08f,.40f),Scatter.FRandRange(.04f,.20f)),
                FRotator(Scatter.FRandRange(-30.f,30.f),Scatter.FRandRange(0.f,360.f),
                Scatter.FRandRange(-45.f,45.f)));
        }
    }

    FRandomStream Random(915);
    // Never dereference the rock model when /Game/SimSandbox/Meshes/Rock
    // is not installed. Missing rocks must not crash the entire world.
    const UStaticMesh* RockMesh=Rocks->GetStaticMesh();
    const double RockSize=RockMesh ? FMath::Max(1.,RockMesh->GetBounds().BoxExtent.GetMax()) : 100.;
    if (!RockMesh)
        UE_LOG(LogTemp,Warning,TEXT("WORLD optional Rock mesh not installed; skip scattered rocks"));
    for(int i=0;RockMesh && i<340;++i)
    {
        const double X=Random.FRandRange(-9200,9200),Y=Random.FRandRange(-9200,9200);
        const bool Yard=!bSolo && FMath::Abs(X)<2400 && FMath::Abs(Y)<2400;
        const double S=Random.FRandRange(Yard?4:30,Yard?22:245)/RockSize;
        const auto Bounds=RockMesh->GetBounds();
        Put(Rocks,{X,Y,Height(X,Y)-(Bounds.Origin.Z-.28f*Bounds.BoxExtent.Z)*S*.75f},
            FVector(S*1.3,S,S*.75f),FRotator(0,Random.FRandRange(0,360),0));
    }
}
