// Fill out your copyright notice in the Description page of Project Settings.


#include "K2Node_PlayerMoveTo.h"

#include "EditorCategoryUtils.h"

#include "UMoveAsyncTaskBlueprintProxy.h"

#define LOCTEXT_NAMESPACE "K2Node_PlayerMoveTo"


UK2Node_PlayerMoveTo::UK2Node_PlayerMoveTo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(UMoveAsyncTaskBlueprintProxy, CreateMoveToProxyObject);
	ProxyFactoryClass = UMoveAsyncTaskBlueprintProxy::StaticClass();
	ProxyClass = UMoveAsyncTaskBlueprintProxy::StaticClass();
}


FText UK2Node_PlayerMoveTo::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::AI);
}

FText UK2Node_PlayerMoveTo::GetTooltipText() const
{
	return LOCTEXT("PlayerMoveTo_Tooltip", "Simple order for Pawn with PlayerController to move to a specific location");
}

FText UK2Node_PlayerMoveTo::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("PlayerMoveTo", "Player MoveTo");
}