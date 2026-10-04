#include "GardenPrepareCommandlet.h"
#include "GardenField.h"
#include "GardenSoil.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetCompilingManager.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "PhysicsEngine/BodySetup.h"
#include "Materials/Material.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/SavePackage.h"

namespace {
bool Save(UObject* Object) {
    UPackage* P=Object->GetOutermost(); P->MarkPackageDirty();
    FString File=FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
    return UPackage::SavePackage(P,Object,*File,Args);
}
struct Geometry {
    TArray<FVector3f> P; TArray<int32> T;
    void Quad(FVector A,FVector B,FVector C,FVector D) {
        int32 V=P.Num(); for(FVector Point:{A,B,C,D}) P.Add(FVector3f(Point));
        T.Append({V,V+1,V+2,V,V+2,V+3});
    }
    void Box(FVector C,FVector E) {
        FVector A=C-E,B=C+E;
        Quad({A.X,A.Y,A.Z},{A.X,A.Y,B.Z},{A.X,B.Y,B.Z},{A.X,B.Y,A.Z});
        Quad({B.X,B.Y,A.Z},{B.X,B.Y,B.Z},{B.X,A.Y,B.Z},{B.X,A.Y,A.Z});
        Quad({B.X,A.Y,A.Z},{B.X,A.Y,B.Z},{A.X,A.Y,B.Z},{A.X,A.Y,A.Z});
        Quad({A.X,B.Y,A.Z},{A.X,B.Y,B.Z},{B.X,B.Y,B.Z},{B.X,B.Y,A.Z});
        Quad({A.X,A.Y,B.Z},{B.X,A.Y,B.Z},{B.X,B.Y,B.Z},{A.X,B.Y,B.Z});
        Quad({A.X,B.Y,A.Z},{B.X,B.Y,A.Z},{B.X,A.Y,A.Z},{A.X,A.Y,A.Z});
    }
};
bool Build(const FString& Name,const Geometry& G,UMaterialInterface* Material,int32 Realm=-1) {
    FString Package=TEXT("/Game/Garden/")+Name;
    if(LoadObject<UStaticMesh>(nullptr,*(Package+TEXT(".")+Name))) return true;
    UStaticMesh* M=NewObject<UStaticMesh>(CreatePackage(*Package),*Name,RF_Public|RF_Standalone);
    M->SetNumSourceModels(1); M->GetStaticMaterials().Add(FStaticMaterial(Material,TEXT("Surface"),TEXT("Surface")));
    FMeshDescription* Mesh=M->CreateMeshDescription(0,FMeshDescription());
    FStaticMeshAttributes A(*Mesh); A.Register(); A.GetVertexInstanceUVs().SetNumChannels(1);
    auto Pos=A.GetVertexPositions(); auto Norm=A.GetVertexInstanceNormals(); auto UV=A.GetVertexInstanceUVs();
    auto Color=A.GetVertexInstanceColors(); auto Tangent=A.GetVertexInstanceTangents(); auto Sign=A.GetVertexInstanceBinormalSigns();
    FPolygonGroupID PG=Mesh->CreatePolygonGroup(); A.GetPolygonGroupMaterialSlotNames()[PG]=TEXT("Surface");
    TArray<FVertexID> V; V.Reserve(G.P.Num());
    for(FVector3f Point:G.P) { auto ID=Mesh->CreateVertex(); Pos[ID]=Point; V.Add(ID); }
    for(int32 I=0;I<G.T.Num();I+=3) {
        TArray<FVertexInstanceID> VI;
        FVector Face=FVector(FVector3f::CrossProduct(G.P[G.T[I+2]]-G.P[G.T[I]],G.P[G.T[I+1]]-G.P[G.T[I]])).GetSafeNormal();
        for(int32 J=0;J<3;J++) {
            int32 Index=G.T[I+J]; auto ID=Mesh->CreateVertexInstance(V[Index]); VI.Add(ID);
            FVector3f P=G.P[Index]; FVector N=Realm>=0?GardenField::Normal(P.X,P.Y,Realm):Face;
            Norm[ID]=FVector3f(N);
            FVector Basis=FMath::Abs(N.X)>.9?FVector::RightVector:FVector::ForwardVector;
            Tangent[ID]=FVector3f((Basis-N*FVector::DotProduct(N,Basis)).GetSafeNormal()); Sign[ID]=1;
            UV.Set(ID,0,Realm>=0?FVector2f(P.X/300,P.Y/300):FVector2f((P.X+P.Y)/160,P.Z/160));
            Color[ID]=Realm>=0?GardenField::Color(P.X,P.Y,Realm):FVector4f(1,1,1,1);
        }
        Mesh->CreatePolygon(PG,VI);
    }
    auto& Settings=M->GetSourceModel(0).BuildSettings;
    Settings.bRecomputeNormals=false; Settings.bRecomputeTangents=false; Settings.bGenerateLightmapUVs=false;
    M->NaniteSettings.bEnabled=Realm>=0;
    M->NaniteSettings.FallbackTarget=ENaniteFallbackTarget::PercentTriangles;
    M->NaniteSettings.FallbackPercentTriangles=1; M->NaniteSettings.FallbackRelativeError=0;
    M->CommitMeshDescription(0); TArray<FText> Errors; M->Build(false,&Errors);
    if(!Errors.IsEmpty()) return false;
    FAssetCompilingManager::Get().FinishAllCompilation();
    M->CreateBodySetup(); auto* Body=M->GetBodySetup(); Body->CollisionTraceFlag=CTF_UseComplexAsSimple;
    Body->bDoubleSidedGeometry=false; Body->InvalidatePhysicsData(); Body->CreatePhysicsMeshes();
    FAssetRegistryModule::AssetCreated(M);
    UE_LOG(LogTemp,Display,TEXT("GARDEN_MESH_READY name=%s vertices=%d triangles=%d collision=%d"),*Name,G.P.Num(),G.T.Num()/3,M->ContainsPhysicsTriMeshData(true));
    return Save(M);
}
}
UGardenPrepareCommandlet::UGardenPrepareCommandlet() { IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true; }
int32 UGardenPrepareCommandlet::Main(const FString& Params) {
    auto* Ground=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/VOID/M_Terrain.M_Terrain"));
    auto* Surface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_HalvethSurface.M_HalvethSurface"));
    auto* Rock=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/VOID/Models/rock_moss_set_01/rock_moss_set_01_2k/StaticMeshes/rock_moss_set_01_rock01.rock_moss_set_01_rock01"));
    if(!Ground||!Surface||!Rock) { UE_LOG(LogTemp,Error,TEXT("GARDEN_REQUIRED_ASSETS_MISSING")); return 2; }
    for(int32 Realm=0;Realm<4;Realm++) {
        Geometry G; int32 N=GardenField::Grid;
        for(int32 Y=0;Y<N;Y++) for(int32 X=0;X<N;X++) {
            double WX=-GardenField::HalfSize+X*2*GardenField::HalfSize/(N-1),WY=-GardenField::HalfSize+Y*2*GardenField::HalfSize/(N-1);
            G.P.Add(FVector3f(WX,WY,GardenField::Height(WX,WY,Realm)));
        }
        for(int32 Y=0;Y<N-1;Y++) for(int32 X=0;X<N-1;X++) {
            const FVector Mid=(FVector(G.P[Y*N+X])+FVector(G.P[(Y+1)*N+X+1]))*.5;
            if(GardenSoil::Cutout(Realm,Mid.X,Mid.Y))continue;
            int32 V=Y*N+X; G.T.Append({V,V+N,V+1,V+1,V+N,V+N+1});
        }
        if(!Build(FString::Printf(TEXT("Terrain_R%d"),Realm),G,Ground,Realm)) return 3;
    }
    Geometry Arch;
    Arch.Box(FVector(-171,0,120),FVector(26,42,120)); Arch.Box(FVector(171,0,120),FVector(26,42,120));
    for(int32 I=0;I<32;I++) {
        double A=PI*I/32,B=PI*(I+1)/32;
        FVector I0(145*FMath::Cos(A),-42,240+145*FMath::Sin(A)),O0(197*FMath::Cos(A),-42,240+197*FMath::Sin(A));
        FVector I1(145*FMath::Cos(B),-42,240+145*FMath::Sin(B)),O1(197*FMath::Cos(B),-42,240+197*FMath::Sin(B));
        FVector Depth(0,84,0);
        Arch.Quad(I0,O0,O1,I1); Arch.Quad(I1+Depth,O1+Depth,O0+Depth,I0+Depth);
        Arch.Quad(I0,I1,I1+Depth,I0+Depth); Arch.Quad(O1,O0,O0+Depth,O1+Depth);
    }
    if(!Build(TEXT("PortalArch"),Arch,Rock->GetMaterial(0))) return 4;
    Geometry Crystal;
    for(int32 I=0;I<9;I++) {
        double A=2*PI*I/9,B=2*PI*(I+1)/9;
        double R0=45+5*FMath::Sin(I*1.9),R1=45+5*FMath::Sin((I+1)*1.9);
        FVector P0(R0*FMath::Cos(A),R0*FMath::Sin(A),-50),P1(R1*FMath::Cos(B),R1*FMath::Sin(B),-50);
        FVector Q0(R0*.8*FMath::Cos(A),R0*.8*FMath::Sin(A),7),Q1(R1*.8*FMath::Cos(B),R1*.8*FMath::Sin(B),7);
        Crystal.Quad(P1,P0,Q0,Q1); Crystal.Quad(Q1,Q0,FVector(8,-5,55),FVector(8,-5,55));
        Crystal.Quad(P0,P1,FVector(0,0,-50),FVector(0,0,-50));
    }
    if(!Build(TEXT("CrystalCrown"),Crystal,Surface)) return 5;
    FAssetCompilingManager::Get().FinishAllCompilation(); return 0;
}
