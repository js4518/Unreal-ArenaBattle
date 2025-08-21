// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/ABAnimationAttackInterface.h"
#include "Interface/ABCharacterWidgetInterface.h"
#include "Interface/ABCharacterItemInterface.h"
#include "GameData/ABCharacterStat.h"
#include "ABCharacterBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogABCharacter, Log, All);

UENUM()
enum class ECharacterControlType : uint8 {
	Shoulder,
	Quater
};

DECLARE_DELEGATE_OneParam(FOnTakeItemDelegate, class UABItemData* /*InItemData*/);

USTRUCT(BlueprintType)
struct FTakeItemDelegateWrapper {
	GENERATED_BODY()
	FTakeItemDelegateWrapper(){}
	FTakeItemDelegateWrapper(const FOnTakeItemDelegate& InItemDelegate) : ItemDelegate(InItemDelegate) {}

	FOnTakeItemDelegate ItemDelegate;
};

UCLASS()
class ARENABATTLE_API AABCharacterBase : public ACharacter, public IABAnimationAttackInterface, public IABCharacterWidgetInterface, public IABCharacterItemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AABCharacterBase();

	virtual void PostInitializeComponents() override;

protected:
	virtual void SetCharacterControlData(const class UABCharacterControlData* CharacterControlData);

	UPROPERTY(EditAnywhere, Category = CharacterControl, Meta = (AllowPrivateAccess = "true"))
	TMap<ECharacterControlType, class UABCharacterControlData*> CharacterControlManager;
	
// Combo Action Section
protected:
	void ProcessComboCommand();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	TObjectPtr<UAnimMontage> ComboActionMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attack, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UABComboActionData> ComboActionData;

	void ComboActionBegin();

	// DECLARE_DELEGATE_TwoParams( FOnMontageEnded, class UAnimMontage*, bool /*bInterrupted*/) 
	// UAnimMontage에 위와 같이 델리게이트 선언되어 있음. 인자 맞춰주기
	void ComboActionEnd(class UAnimMontage* TargetMontage, bool IsProperlyEnded);
	virtual void NotifyComboActionEnd();

	int32 CurrentCombo = 0;
	FTimerHandle ComboTimerHandle;
	bool HasNextComboCommand = false;

	void SetComboCheckTimer();
	void ComboCheck();

// Attack Hit Section
protected:
	virtual void AttackHitCheck() override;

	// 액터에서 데미지 로직 구현되어있음
	// EventInstigator : 가해 플레이어(컨트롤러) / DamageCauser : 가해자가 빙의한 폰 혹은 무기
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

// Dead Section
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animation)
	TObjectPtr<UAnimMontage> DeadMontage;

	virtual void SetDead();
	void PlayDeadAnimation();

	float DeadEventDelayTime = 5.f;

// Stat section
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Stat, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UABCharacterStatComponent> Stat;
// UI Widget Section
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Widget, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UABWidgetComponent> HpBar;

	virtual void SetupCharacterWidget(class UABUserWidget* InUserWidget) override;

// Item Section
protected:
	virtual void TakeItem(class UABItemData* InItemData) override;

	UPROPERTY()
	TArray<FTakeItemDelegateWrapper> TakeItemActions;
	
	virtual void DrinkPotion(class UABItemData* InItemData);
	virtual void EquipWeapon(class UABItemData* InItemData);
	virtual void ReadScroll(class UABItemData* InItemData);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Equipment, Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USkeletalMeshComponent> Weapon;

// Stat Section
public:
	int32 GetLevel();
	void SetLevel(int32 InNewLevel);
	void ApplyStat(const FABCharacterStat& BaseStat, const FABCharacterStat& ModifierStat);
};
