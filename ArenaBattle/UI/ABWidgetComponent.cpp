// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ABWidgetComponent.h"
#include "ABUserWidget.h"

void UABWidgetComponent::InitWidget()
{
	Super::InitWidget(); // 여기서 UserWidget의 인스턴스 생성

	// 이 WidgetComponent가 쓰고 있는 UserWidget를 가져와서 형변환
	UABUserWidget* ABUserWidget = Cast<UABUserWidget>(GetWidget());
	if (ABUserWidget) {
		ABUserWidget->SetOwningActor(GetOwner());
	}
}
