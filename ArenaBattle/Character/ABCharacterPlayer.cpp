// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ABCharacterControlData.h"
#include "UI/ABHUDWidget.h"
#include "CharacterStat/ABCharacterStatComponent.h"
#include "Interface/ABGameInterface.h"
#include "GameFramework/GameModeBase.h"

AABCharacterPlayer::AABCharacterPlayer()
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("NAME_CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f; // 암 길이
	CameraBoom->bUsePawnControlRotation = true; // 컨트롤러의 회전 따름

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("NAME_FollowCamera"));
	// SetupAttachment의 두번째 인자 : 부모로 어떤 컴포넌트를 지정했을때, 위치 자동 설정 -> 소켓
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // 얘는 암에 달려있으니 추가 회전 X

	// Input

	//static ConstructorHelpers::FObjectFinder<UInputMappingContext> InputMappingContextRef(TEXT("/Script/EnhancedInput.InputMappingContext'/Game/ArenaBattle/Input/IMC_Default.IMC_Default'"));
	//if (InputMappingContextRef.Object) {
	//	DefaultMappingContext = InputMappingContextRef.Object;
	//}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionJumpRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_Jump.IA_Jump'"));
	if (InputActionJumpRef.Object) {
		JumpAction = InputActionJumpRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputChangeActionRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_ChangeControl.IA_ChangeControl'"));
	if (InputChangeActionRef.Object) {
		ChangeControlAction = InputChangeActionRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionShoulderMoveRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_ShoulderMove.IA_ShoulderMove'"));
	if (InputActionShoulderMoveRef.Object) {
		ShoulderMoveAction = InputActionShoulderMoveRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionShoulderLookRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_ShoulderLook.IA_ShoulderLook'"));
	if (InputActionShoulderLookRef.Object) {
		ShoulderLookAction = InputActionShoulderLookRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionQuaterMoveRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_QuaterMove.IA_QuaterMove'"));
	if (InputActionQuaterMoveRef.Object) {
		QuaterMoveAction = InputActionQuaterMoveRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionAttackRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_Attack.IA_Attack'"));
	if (InputActionAttackRef.Object) {
		AttackAction = InputActionAttackRef.Object;
	}

	CurrentCharacterControlType = ECharacterControlType::Quater;
}

void AABCharacterPlayer::BeginPlay()
{
	Super::BeginPlay();

	//APlayerController* PlayerController = CastChecked<APlayerController>(GetController());
	//// 서브시스템(Subsystem)은 언리얼 엔진 내부 또는 사용자 정의 기능을 전역적으로 공유/관리하는 시스템성 클래스
	//// U~~LocalPlayerSubsystem은 로컬 플레이어(플레이어마다) 하나씩 존재
	//// UEnhancedInputLocalPlayerSubsystem은 입력 매핑, 우선순위 관리 등 처리
	//if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) {
	//	Subsystem->AddMappingContext(DefaultMappingContext, 0); // 우선순위 0번으로 지정
	//	//Subsystem->RemoveMappingContext(DefaultMappingContext);
	//}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController) {
		EnableInput(PlayerController);
	}

	SetCharacterControl(CurrentCharacterControlType);
}

void AABCharacterPlayer::SetDead()
{
	Super::SetDead();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController) {
		DisableInput(PlayerController);
	}

	IABGameInterface* ABGameMode = Cast<IABGameInterface>(GetWorld()->GetAuthGameMode());
	
	if (ABGameMode) {
		ABGameMode->OnPlayerDead();
	}
}

void AABCharacterPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	// CastChecked : 캐스팅 실패시 크래시 발생 -> EnhancedInput 방식을 사용하는것을 강제

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	EnhancedInputComponent->BindAction(ShoulderMoveAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::ShoulderMove);
	EnhancedInputComponent->BindAction(ShoulderLookAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::ShoulderLook);
	EnhancedInputComponent->BindAction(QuaterMoveAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::QuaterMove);
	EnhancedInputComponent->BindAction(ChangeControlAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::ChangeCharacterControl);
	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::Attack);
}

void AABCharacterPlayer::ChangeCharacterControl()
{
	if (CurrentCharacterControlType == ECharacterControlType::Quater)
		SetCharacterControl(ECharacterControlType::Shoulder);
	else SetCharacterControl(ECharacterControlType::Quater);
}

void AABCharacterPlayer::SetCharacterControl(ECharacterControlType NewCharacterControlType)
{
	UABCharacterControlData* NewCharacterControl = CharacterControlManager[NewCharacterControlType];
	check(NewCharacterControl);

	SetCharacterControlData(NewCharacterControl);

	APlayerController* PlayerController = CastChecked<APlayerController>(GetController());
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())) {
		Subsystem->ClearAllMappings();
		
		UInputMappingContext* NewMappingContext = NewCharacterControl->InputMappingContext;
		if(NewMappingContext)
			Subsystem->AddMappingContext(NewMappingContext, 0);
	}

	CurrentCharacterControlType = NewCharacterControlType;
}

void AABCharacterPlayer::SetCharacterControlData(const UABCharacterControlData* CharacterControlData)
{
	Super::SetCharacterControlData(CharacterControlData);

	CameraBoom->TargetArmLength = CharacterControlData->TargetArmLength;
	CameraBoom->SetRelativeRotation(CharacterControlData->RelativeRotation);
	CameraBoom->bUsePawnControlRotation = CharacterControlData->bUsePawnControlRotation;
	CameraBoom->bInheritPitch = CharacterControlData->bInheritPitch;
	CameraBoom->bInheritYaw = CharacterControlData->bInheritYaw;
	CameraBoom->bInheritRoll = CharacterControlData->bInheritRoll;
	CameraBoom->bDoCollisionTest = CharacterControlData->bDoCollisionTest;
}

void AABCharacterPlayer::ShoulderMove(const FInputActionValue& Value)
{
	// 액션 바인드했을시, 함수 인자로 자동으로 FInputActionValue 객체 넘어감

	FVector2D MovementVector = Value.Get<FVector2D>(); // 입력값 가져오기

	const FRotator Rotation = Controller->GetControlRotation(); // 컨트롤러의 회전 구하기
	const FRotator YawRotation(0, Rotation.Yaw, 0); // 유의미한 회전만 분리

	//FRotation에 저장된 회전도를 3차원 행렬로 변형하여, 현재 회전을 기준으로 하는 X, Y축을 가져옴
	//주의 : 언리얼에선 기본적으로 X축이 Forward, Y축이 Right임 -> 즉 X,Y축이 반대라고 생각하면 됨 
	// -> Move의 InputAction에서 Modifier에 Swizzle Input Axis Values -> YXZ로 변경하면 원래 개념대로 됨
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// 컨트롤러의 회전을 기준으로 가져온 방향과 입력값 이용하여 실제 이동

	// 축 변경 이전
	/*AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);*/

	// 축 변경 이후
	AddMovementInput(ForwardDirection, MovementVector.X);
	AddMovementInput(RightDirection, MovementVector.Y);
}

void AABCharacterPlayer::ShoulderLook(const FInputActionValue& Value)
{
	// 마우스 위치 입력을 가져옴
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// 입력에 따라 컨트롤러 회전 -> 카메라암이 컨트롤러의 회전을 따르니 카메라도 회전
	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(-LookAxisVector.Y);
}

void AABCharacterPlayer::QuaterMove(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	float MovementVectorSize = 1.f;
	float MovementVectorSizeSquared = MovementVector.SquaredLength();

	if (MovementVectorSizeSquared > 1.f) {
		MovementVector.Normalize();
		MovementVectorSizeSquared = 1.f;
	}
	else {
		MovementVectorSize = FMath::Sqrt(MovementVectorSizeSquared);
	}

	FVector MoveDirection = FVector(MovementVector.X, MovementVector.Y, 0);
	GetController()->SetControlRotation(FRotationMatrix::MakeFromX(MoveDirection).Rotator());
	AddMovementInput(MoveDirection, MovementVectorSize);
}

void AABCharacterPlayer::Attack(const FInputActionValue& Value)
{
	ProcessComboCommand();
}

void AABCharacterPlayer::SetupHUDWidget(UABHUDWidget* InHUDWidget)
{
	if (InHUDWidget) {
		InHUDWidget->UpdateStat(Stat->GetBaseStat(), Stat->GetModifierStat());
		InHUDWidget->UpdateHpBar(Stat->GetCurrentHp());

		Stat->OnStatChanged.AddUObject(InHUDWidget, &UABHUDWidget::UpdateStat);
		Stat->OnHpChanged.AddUObject(InHUDWidget, &UABHUDWidget::UpdateHpBar);
	}
}
