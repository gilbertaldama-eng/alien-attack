#include "ContactMission.h"
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
