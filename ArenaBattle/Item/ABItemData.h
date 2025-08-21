// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ABItemData.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8 {
	Weapon = 0,
	Potion,
	Scroll
};

/**
 * 
 */
UCLASS()
class ARENABATTLE_API UABItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	FPrimaryAssetId GetPrimaryAssetId() const override {
		// "타입:이름" 형태의 ID를 반환. 이때 이름은 에셋 이름을 그대로 사용
		return FPrimaryAssetId("ABItemData", GetFName());
	}
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Type)
	EItemType Type;
};
