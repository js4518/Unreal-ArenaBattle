// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ABCharacterControlData.h"
#include "Animation/AnimMontage.h"
#include "ABComboActionData.h"
#include "Physics/ABCollision.h"
#include "Engine/DamageEvents.h"
#include "CharacterStat/ABCharacterStatComponent.h"
#include "UI/ABWidgetComponent.h"
#include "UI/ABHpBarWidget.h"
#include "Item/ABItems.h"

DEFINE_LOG_CATEGORY(LogABCharacter);

// Sets default values
AABCharacterBase::AABCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationRoll = false; // 컨트롤러의 회전 따를지 설정. 각 X Y Z
	bUseControllerRotationPitch = false; 
	bUseControllerRotationYaw = false;
	
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(42.0f, 96.0f);
	Capsule->SetCollisionProfileName(CPROFILE_ABCAPSULE); // 유니티의 충돌 레이어 그룹 설정같은것

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true; // 이동 방향으로 캐릭터 회전
	Move->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // 회전률 설정(제한)
	Move->JumpZVelocity = 700.0f;
	Move->AirControl = 0.35f; // 공중에서 이동량 줄이기
	Move->MaxWalkSpeed = 500.f;
	Move->MinAnalogWalkSpeed = 20.f; // 이동에 따른 최소 입력값 설정
	Move->BrakingDecelerationWalking = 2000.f; // 감속
	
	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -100.f), FRotator(0.f, -90.f, 0.f)); // 메쉬 형태에 따라 기본 위치, 회전 설정
	SkelMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	SkelMesh->SetCollisionProfileName(TEXT("NoCollision")); // 메쉬는 직접 충돌은 X, 트레이스(빛 처리) 용도

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMeshRef(TEXT("/Script/Engine.SkeletalMesh'/Game/InfinityBladeWarriors/Character/CompleteCharacters/SK_CharM_Cardboard.SK_CharM_Cardboard'"));
	if (CharacterMeshRef.Object) {
		SkelMesh->SetSkeletalMesh(CharacterMeshRef.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimInstanceClassRef(TEXT("/Game/ArenaBattle/Animation/ABP_ABCharacter.ABP_ABCharacter_C"));
	if (AnimInstanceClassRef.Class) {
		SkelMesh->SetAnimInstanceClass(AnimInstanceClassRef.Class);
	}

	static ConstructorHelpers::FObjectFinder<UABCharacterControlData> ShoulderDataRef(TEXT("/Script/ArenaBattle.ABCharacterControlData'/Game/ArenaBattle/CharacterControl/ABC_Shoulder.ABC_Shoulder'"));
	if (ShoulderDataRef.Object) {
		CharacterControlManager.Add(ECharacterControlType::Shoulder, ShoulderDataRef.Object);
	}

	static ConstructorHelpers::FObjectFinder<UABCharacterControlData> QuaterDataRef(TEXT("/Script/ArenaBattle.ABCharacterControlData'/Game/ArenaBattle/CharacterControl/ABC_Quater.ABC_Quater'"));
	if (QuaterDataRef.Object) {
		CharacterControlManager.Add(ECharacterControlType::Quater, QuaterDataRef.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> ComboMontageRef(TEXT("/Script/Engine.AnimMontage'/Game/ArenaBattle/Animation/AM_ComboAttack.AM_ComboAttack'"));
	if (ComboMontageRef.Object) {
		ComboActionMontage = ComboMontageRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UABComboActionData> ComboActionDataRef(TEXT("/Script/ArenaBattle.ABComboActionData'/Game/ArenaBattle/CharacterAction/ABA_ComboAttack.ABA_ComboAttack'"));
	if (ComboActionDataRef.Object) {
		ComboActionData = ComboActionDataRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> DeadMontageRef(TEXT("/Script/Engine.AnimMontage'/Game/ArenaBattle/Animation/AM_Dead.AM_Dead'"));
	if (DeadMontageRef.Object) {
		DeadMontage = DeadMontageRef.Object;
	}

	// Stat Component
	Stat = CreateDefaultSubobject<UABCharacterStatComponent>(TEXT("Stat"));

	// Widget Component
	HpBar = CreateDefaultSubobject<UABWidgetComponent>(TEXT("Widget"));
	HpBar->SetupAttachment(GetMesh());
	HpBar->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
	static ConstructorHelpers::FClassFinder<UUserWidget> HpBarWidgetRef(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/ArenaBattle/UI/WBP_HpBar.WBP_HpBar_C'"));
	if (HpBarWidgetRef.Class) {
		HpBar->SetWidgetClass(HpBarWidgetRef.Class);
		HpBar->SetWidgetSpace(EWidgetSpace::Screen); // 2D로 지정
		HpBar->SetDrawSize(FVector2d(150.f, 15.f));
		HpBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Item Actions
	TakeItemActions.Add(FTakeItemDelegateWrapper(FOnTakeItemDelegate::CreateUObject(this, &AABCharacterBase::EquipWeapon)));
	TakeItemActions.Add(FTakeItemDelegateWrapper(FOnTakeItemDelegate::CreateUObject(this, &AABCharacterBase::DrinkPotion)));
	TakeItemActions.Add(FTakeItemDelegateWrapper(FOnTakeItemDelegate::CreateUObject(this, &AABCharacterBase::ReadScroll)));

	// Weapon Component
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));
	Weapon->SetupAttachment(GetMesh(), TEXT("hand_rSocket"));
}

void AABCharacterBase::PostInitializeComponents()
{
	// 그냥 이 함수 쓰는거 보여주려고 여기서 한것임. 그냥 생성자나 BeginPlay()에서 해도 이 경우에는 문제없음
	Super::PostInitializeComponents();

	Stat->OnHpZero.AddUObject(this, &AABCharacterBase::SetDead);
	Stat->OnStatChanged.AddUObject(this, &AABCharacterBase::ApplyStat);
}

void AABCharacterBase::SetCharacterControlData(const UABCharacterControlData* CharacterControlData)
{
	// Pawn
	bUseControllerRotationYaw = CharacterControlData->bUseControllerRotationYaw;

	// CharacterMovement
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = CharacterControlData->bOrientRotationToMovement;
	Move->bUseControllerDesiredRotation = CharacterControlData->bUseControllerDesiredRotation;
	Move->RotationRate = CharacterControlData->RotationRate;
}

void AABCharacterBase::ProcessComboCommand()
{
	if (CurrentCombo == 0) {
		ComboActionBegin();
		return;
	}

	if (!ComboTimerHandle.IsValid()) {
		HasNextComboCommand = false;
	}
	else {
		HasNextComboCommand = true;
	}
}

void AABCharacterBase::ComboActionBegin()
{
	CurrentCombo = 1;

	// 이동 기능 없애기
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

	const float AttackSpeedRate = Stat->GetTotalStat().AttackSpeed;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->Montage_Play(ComboActionMontage, AttackSpeedRate);
	
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AABCharacterBase::ComboActionEnd);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, ComboActionMontage);

	ComboTimerHandle.Invalidate();
	SetComboCheckTimer();
}

void AABCharacterBase::ComboActionEnd(UAnimMontage* TargetMontage, bool IsProperlyEnded)
{
	ensure(CurrentCombo != 0);
	CurrentCombo = 0;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

	NotifyComboActionEnd();
}

void AABCharacterBase::NotifyComboActionEnd()
{

}

void AABCharacterBase::SetComboCheckTimer()
{
	int32 ComboIndex = CurrentCombo - 1;
	ensure(ComboActionData->EffectiveFrameCount.IsValidIndex(ComboIndex));

	const float AttackSpeedRate = Stat->GetTotalStat().AttackSpeed;
	float ComboEffectiveTime = (ComboActionData->EffectiveFrameCount[ComboIndex] / ComboActionData->FrameRate) / AttackSpeedRate;
	if (ComboEffectiveTime > 0.f) {
		// 마지막 인자 : 중복작동 X
		GetWorld()->GetTimerManager().SetTimer(ComboTimerHandle, this, &AABCharacterBase::ComboCheck, ComboEffectiveTime, false);
	}
}

void AABCharacterBase::ComboCheck()
{
	ComboTimerHandle.Invalidate(); // 타이머 초기화
	if (HasNextComboCommand) {
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		CurrentCombo = FMath::Clamp(CurrentCombo + 1, 1, ComboActionData->MaxComboCount);
		FName NextSection = *FString::Printf(TEXT("%s%d"), *ComboActionData->MontageSectionNamePrefix, CurrentCombo);
		AnimInstance->Montage_JumpToSection(NextSection, ComboActionMontage);
		SetComboCheckTimer();
		HasNextComboCommand = false;
	}
}

void AABCharacterBase::AttackHitCheck()
{
	FHitResult OutHitResult; // 결과 받아올 구조체

	// FCollisionQueryParams(FName InTraceTag, bool bInTraceComplex=false, const AActor* InIgnoreActor=NULL)
	// 위와 같이 선언되어있음. 충돌 분석할때 사용할 이름, 복잡한 형태의 충돌체도 감지할지, 무시할 액터(자기자신 무시 필요)
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Attack), false, this);
	// SCENE_QUERY_STAT은 언리얼이 제공하는 분석 툴을 사용하는 것. Attack이라는 태그로 구분시키는것

	const float AttackRange = Stat->GetTotalStat().AttackRange;
	const float AttackRadius = Stat->GetAttackRadius();
	const float AttackDamage = Stat->GetTotalStat().Attack;

	// 시작 위치를 캡슐 기준으로
	const FVector Start = GetActorLocation() + GetActorForwardVector() * (GetCapsuleComponent()->GetScaledCapsuleRadius());
	const FVector End = Start + GetActorForwardVector() * AttackRange;

	// Sweep 방식 사용, 단일 타겟 대상, 채널을 통한 판정 / 충돌도 월드가 관리
	bool HitDetected = GetWorld()->SweepSingleByChannel(OutHitResult, Start, End, FQuat::Identity, CCHANNEL_ABACTION, FCollisionShape::MakeSphere(AttackRadius), Params);
	if (HitDetected) {
		FDamageEvent DamageEvent;
		OutHitResult.GetActor()->TakeDamage(AttackDamage, DamageEvent, GetController(), this);
	}

#if ENABLE_DRAW_DEBUG
	FVector CapsuleOrigin = Start + (End - Start) * 0.5f;
	float CapsuleHalfHeight = AttackRange * 0.5f;
	FColor DrawColor = HitDetected ? FColor::Green : FColor::Red;

	// 캡슐 눕혀서 그리기. 임시적으로, 5.0f만
	DrawDebugCapsule(GetWorld(), CapsuleOrigin, CapsuleHalfHeight, AttackRadius, FRotationMatrix::MakeFromZ(GetActorForwardVector()).ToQuat(),DrawColor, false, 5.0f);

#endif
}

float AABCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	Stat->ApplyDamage(DamageAmount);

	return DamageAmount; // 최종으로 받은 데미지 양
}

void AABCharacterBase::SetDead()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	PlayDeadAnimation();
	SetActorEnableCollision(false); // 액터에 대한 모든 콜리전 끄기
	HpBar->SetHiddenInGame(true);
}

void AABCharacterBase::PlayDeadAnimation()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	AnimInstance->StopAllMontages(0.0f); // 기존 공격 몽타주 종료
	AnimInstance->Montage_Play(DeadMontage, 1.f);
}

void AABCharacterBase::SetupCharacterWidget(UABUserWidget* InUserWidget)
{
	UABHpBarWidget* HpBarWidget = Cast<UABHpBarWidget>(InUserWidget);
	if (HpBarWidget) {
		HpBarWidget->UpdateStat(Stat->GetBaseStat(), Stat->GetModifierStat());
		HpBarWidget->UpdateHpBar(Stat->GetCurrentHp());
		Stat->OnHpChanged.AddUObject(HpBarWidget, &UABHpBarWidget::UpdateHpBar);
		Stat->OnStatChanged.AddUObject(HpBarWidget, &UABHpBarWidget::UpdateStat);
	}
}

void AABCharacterBase::TakeItem(UABItemData* InItemData)
{
	if (InItemData) {
		TakeItemActions[(uint8)InItemData->Type].ItemDelegate.ExecuteIfBound(InItemData);
	}
}

void AABCharacterBase::DrinkPotion(UABItemData* InItemData)
{
	UABPotionItemData* PotionItemData = Cast<UABPotionItemData>(InItemData);
	if (PotionItemData) {
		Stat->HealHp(PotionItemData->HealAmount);
	}
}

void AABCharacterBase::EquipWeapon(UABItemData* InItemData)
{
	UABWeaponItemData* WeaponItemData = Cast<UABWeaponItemData>(InItemData);
	if (WeaponItemData) {
		if (WeaponItemData->WeaponMesh.IsPending()) {
			WeaponItemData->WeaponMesh.LoadSynchronous();
		}
		Weapon->SetSkeletalMesh(WeaponItemData->WeaponMesh.Get());
		Stat->SetModifierStat(WeaponItemData->ModifierStat);
	}
}

void AABCharacterBase::ReadScroll(UABItemData* InItemData)
{
	UABScrollItemData* ScrollItemData = Cast<UABScrollItemData>(InItemData);
	if (ScrollItemData) {
		Stat->AddBaseStat(ScrollItemData->BaseStat);
	}
}

int32 AABCharacterBase::GetLevel()
{
	return Stat->GetCurrentLevel();
}

void AABCharacterBase::SetLevel(int32 InNewLevel)
{
	Stat->SetLevelStat(InNewLevel);
}

void AABCharacterBase::ApplyStat(const FABCharacterStat& BaseStat, const FABCharacterStat& ModifierStat)
{
	float MovementSpeed = (BaseStat + ModifierStat).MovementSpeed;
	GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
}
