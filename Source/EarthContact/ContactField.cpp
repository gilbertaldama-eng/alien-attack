#include "ContactMission.h"
#include "ContactImportedAssets.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

AContactField::AContactField()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Station")));
    auto* Sun=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Afternoon"));
    Sun->SetupAttachment(RootComponent);
    Sun->SetRelativeRotation(FRotator(-32,-115,0));
    Sun->SetAtmosphereSunLight(true);
    Sun->SetIntensity(5.f);
    Sun->SetLightColor(FLinearColor(1,.86f,.68f));
    auto* Sky=CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
    Sky->SetupAttachment(RootComponent);
    Sky->SetRelativeLocation(FVector(0,0,-150));
    auto* Fill=CreateDefaultSubobject<USkyLightComponent>(TEXT("Skylight"));
    Fill->SetupAttachment(RootComponent);
    Fill->SetIntensity(1.1f);
    Fill->SetRealTimeCaptureEnabled(true);
    auto* Haze=CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("DustHaze"));
    Haze->SetupAttachment(RootComponent);
    Haze->SetFogDensity(.012f);
    Haze->SetFogInscatteringColor(FLinearColor(.58f,.60f,.63f));
}

void AContactField::BeginPlay()
{
    Super::BeginPlay();
    // A graded central yard gives way to eroded ridgelines outside the station.
    auto* Terrain=NewObject<UProceduralMeshComponent>(this);
    Terrain->SetupAttachment(RootComponent);
    Terrain->SetCollisionProfileName(TEXT("BlockAll"));
    Terrain->RegisterComponent();
    TArray<FVector> V,N;
    TArray<FVector2D> UV;
    TArray<FColor> Colors;
    TArray<int32> Indices;
    constexpr int32 Grid=111;
    constexpr double Step=240;
    auto Height=[](double X,double Y) {
        const double Edge=FMath::Clamp((FMath::Max(FMath::Abs(X),FMath::Abs(Y))-2450)/2800,0.,1.);
        const float Hills=FMath::PerlinNoise2D(FVector2D(X,Y)*.00029f+FVector2D(63,19));
        const float Grain=FMath::PerlinNoise2D(FVector2D(X,Y)*.0016f);
        return -3.+Edge*(350+1750*FMath::Abs(Hills)+105*Grain);
    };
    for (int32 Row=0; Row<Grid; ++Row)
        for(int32 Col=0; Col<Grid; ++Col)
        {
            const double X=(Col-Grid/2)*Step,Y=(Row-Grid/2)*Step;
            V.Add({X,Y,Height(X,Y)});
            N.Add(FVector(Height(X-10,Y)-Height(X+10,Y),Height(X,Y-10)-Height(X,Y+10),20).GetSafeNormal());
            UV.Add({Col/4.,Row/4.});
            const float T=.5f+.46f*FMath::PerlinNoise2D(FVector2D(X,Y)*.0012f);
            Colors.Add(FLinearColor::LerpUsingHSV(FLinearColor(.24f,.105f,.052f),FLinearColor(.67f,.40f,.21f),T).ToFColor(true));
            if(Row==Grid-1 || Col==Grid-1) continue;
            const int32 I=Row*Grid+Col;
            Indices.Append({I,I+Grid,I+1,I+1,I+Grid,I+Grid+1});
        }
    Terrain->CreateMeshSection(0,V,Indices,N,UV,Colors,{},true);
    Terrain->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Field/M_ArroyoSand")));

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
    const bool bRealCrashedUFO=SpawnImported(ContactAssets::CrashedUFO(),
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
    if (!bRealCrashedUFO)
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

    FRandomStream Random(915);
    const double RockSize=Rocks->GetStaticMesh() ? FMath::Max(1.,Rocks->GetStaticMesh()->GetBounds().BoxExtent.GetMax()) : 100.;
    for(int i=0;i<340;++i)
    {
        const double X=Random.FRandRange(-9200,9200),Y=Random.FRandRange(-9200,9200);
        const bool Yard=FMath::Abs(X)<2400 && FMath::Abs(Y)<2400;
        const double S=Random.FRandRange(Yard?4:50,Yard?22:310)/RockSize;
        const auto Bounds=Rocks->GetStaticMesh()->GetBounds();
        Put(Rocks,{X,Y,Height(X,Y)-(Bounds.Origin.Z-.28f*Bounds.BoxExtent.Z)*S*.75f},
            FVector(S*1.3,S,S*.75f),FRotator(0,Random.FRandRange(0,360),0));
    }
}
