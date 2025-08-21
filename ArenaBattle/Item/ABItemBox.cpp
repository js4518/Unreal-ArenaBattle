// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ABItemBox.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Physics/ABCollision.h"
#include "Interface/ABCharacterItemInterface.h"
#include "Engine/AssetManager.h"
#include "ABItemData.h"

// Sets default values
AABItemBox::AABItemBox()
{
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Effect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Effect"));

	RootComponent = Trigger;
	Mesh->SetupAttachment(Trigger);
	Effect->SetupAttachment(Trigger);

	Trigger->SetCollisionProfileName(CPROFILE_ABTRIGGER); // ABTrigger
	Trigger->SetBoxExtent(FVector(40.f, 42.f, 30.f)); // 박스 크기 지정
	// PrimitiveComponent에 선언되어있는 델리게이트. 다이나믹이기에 블루프린트에서도 사용가능
	// 따라서 연결하는 함수에 UFUNCTION() 해줘야함

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BoxMeshRef(TEXT("/Script/Engine.StaticMesh'/Game/ArenaBattle/Environment/Props/SM_Env_Breakables_Box1.SM_Env_Breakables_Box1'"));
	if (BoxMeshRef.Object) {
		Mesh->SetStaticMesh(BoxMeshRef.Object);
	}
	Mesh->SetRelativeLocation(FVector(0.f, -3.5f, -30.f));
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));

	static ConstructorHelpers::FObjectFinder<UParticleSystem> EffectRef(TEXT("/Script/Engine.ParticleSystem'/Game/ArenaBattle/Effect/P_TreasureChest_Open_Mesh.P_TreasureChest_Open_Mesh'"));
	if (EffectRef.Object) {
		Effect->SetTemplate(EffectRef.Object);
		Effect->bAutoActivate = false;
	}
}

void AABItemBox::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 싱글톤으로 존재
	UAssetManager& Manager = UAssetManager::Get();
	
	TArray<FPrimaryAssetId> Assets;
	Manager.GetPrimaryAssetIdList(TEXT("ABItemData"), Assets);
	ensure(Assets.Num() > 0);

	int32 RandomIndex = FMath::RandRange(0, Assets.Num() - 1);
	FSoftObjectPtr AssetPtr(Manager.GetPrimaryAssetPath(Assets[RandomIndex]));
	if (AssetPtr.IsPending()) {
		AssetPtr.LoadSynchronous();
	}
	Item = Cast<UABItemData>(AssetPtr.Get());
	ensure(Item);

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AABItemBox::OnOverlapBegin);
}

void AABItemBox::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult)
{
	if (Item == nullptr) {
		Destroy();
		return;
	}

	IABCharacterItemInterface* OverlappingPawn = Cast<IABCharacterItemInterface>(OtherActor);
	if (OverlappingPawn) {
		OverlappingPawn->TakeItem(Item);
	}

	Effect->Activate(true);
	Mesh->SetHiddenInGame(true);
	SetActorEnableCollision(false);
	Effect->OnSystemFinished.AddDynamic(this, &AABItemBox::OnEffectFinished);
}

void AABItemBox::OnEffectFinished(UParticleSystemComponent* ParticleSystem)
{
	Destroy();
}

