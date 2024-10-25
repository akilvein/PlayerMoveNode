// Fill out your copyright notice in the Description page of Project Settings.

#include "UMoveAsyncTaskBlueprintProxy.h"

#include "AIController.h"
#include "NavigationPath.h"
#include "NavigationData.h"
#include "NavigationSystem.h"

#define LOCTEXT_NAMESPACE "PlayerControllerMove"

UMoveAsyncTaskBlueprintProxy::UMoveAsyncTaskBlueprintProxy(const FObjectInitializer& ObjectInitializer)
{

}


void UMoveAsyncTaskBlueprintProxy::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& MovementResult)
{
	if (MovementResult.IsSuccess())
	{
		OnSuccess.Broadcast(MovementResult.Code);
	}
	else
	{
		OnFail.Broadcast(MovementResult.Code);
	}

	PathFollowingComp->OnRequestFinished.RemoveAll(this);
}


UMoveAsyncTaskBlueprintProxy* UMoveAsyncTaskBlueprintProxy::CreateMoveToProxyObject(UObject* WorldContextObject, APawn* Pawn, FVector TargetLocation, AActor* TargetActor, float AcceptanceRadius, bool bStopOnOverlap)
{
	if (TargetActor)
		TargetLocation = TargetActor->GetActorLocation();

	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	UMoveAsyncTaskBlueprintProxy* MyObj = NewObject<UMoveAsyncTaskBlueprintProxy>(World);

	AController* Controller = Pawn->GetController();
	if (!Controller)
	{
		UE_LOG(LogNavigation, Warning, TEXT("MoveAsyncTaskBlueprint Pawn:%s has no controller!"), *GetNameSafe(Pawn));
		return nullptr;
	}

	UNavigationSystemV1* NavSys = Controller ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Controller->GetWorld()) : nullptr;
	if (NavSys == nullptr || Controller == nullptr || Controller->GetPawn() == nullptr)
	{
		UE_LOG(LogNavigation, Warning, TEXT("MoveAsyncTaskBlueprint called for NavSys:%s Controller:%s controlling Pawn:%s with goal actor %s (if any of these is None then there's your problem"),
			*GetNameSafe(NavSys), *GetNameSafe(Controller), Controller ? *GetNameSafe(Controller->GetPawn()) : TEXT("NULL"), *GetNameSafe(TargetActor));
		return nullptr;
	}

	AAIController* AsAIController = Cast<AAIController>(Controller);

	if (AsAIController)
	{
		MyObj->PathFollowingComp = AsAIController->GetPathFollowingComponent();
	}
	else
	{
		MyObj->PathFollowingComp = Controller->FindComponentByClass<UPathFollowingComponent>();
		if (!MyObj->PathFollowingComp)
		{
			MyObj->PathFollowingComp = NewObject<UPathFollowingComponent>(Controller);
			MyObj->PathFollowingComp->RegisterComponentWithWorld(Controller->GetWorld());
			MyObj->PathFollowingComp->Initialize();
		}
	}

	if (!MyObj->PathFollowingComp)
	{
		FMessageLog("PIE").Warning(FText::Format(
			LOCTEXT("MoveAsyncTaskBlueprint", "Move failed for {0}: movement not allowed"),
			FText::FromName(Controller->GetFName())
		));
		return nullptr;
	}

	if (!MyObj->PathFollowingComp->IsPathFollowingAllowed())
	{
		FMessageLog("PIE").Warning(FText::Format(
			LOCTEXT("MoveAsyncTaskBlueprint", "Move failed for {0}: movement not allowed"),
			FText::FromName(Controller->GetFName())
		));
		return nullptr;
	}

	MyObj->PathFollowingComp->OnRequestFinished.AddUObject(MyObj, &UMoveAsyncTaskBlueprintProxy::OnMoveCompleted);

	const bool bAlreadyAtGoal = MyObj->PathFollowingComp->HasReached(TargetLocation, EPathFollowingReachMode::OverlapAgentAndGoal);

	// script source, keep only one move request at time
	if (MyObj->PathFollowingComp->GetStatus() != EPathFollowingStatus::Idle)
	{
		MyObj->PathFollowingComp->AbortMove(*NavSys, FPathFollowingResultFlags::ForcedScript | FPathFollowingResultFlags::NewRequest
			, FAIRequestID::AnyRequest, bAlreadyAtGoal ? EPathFollowingVelocityMode::Reset : EPathFollowingVelocityMode::Keep);
	}	

	if (bAlreadyAtGoal)
	{
		MyObj->PathFollowingComp->RequestMoveWithImmediateFinish(EPathFollowingResult::Success);
	}
	else
	{
		const FVector AgentNavLocation = Controller->GetNavAgentLocation();
		const ANavigationData* NavData = NavSys->GetNavDataForProps(Controller->GetNavAgentPropertiesRef(), AgentNavLocation);
		if (NavData)
		{
			FPathFindingQuery Query(Controller, *NavData, AgentNavLocation, TargetLocation);
			FPathFindingResult Result = NavSys->FindPathSync(Query);
			if (Result.IsSuccessful())
			{
				if (TargetActor)
					Result.Path->SetGoalActorObservation(*TargetActor, 100.0f);
				MyObj->PathFollowingComp->RequestMove(FAIMoveRequest(TargetLocation), Result.Path);
			}
			else if (MyObj->PathFollowingComp->GetStatus() != EPathFollowingStatus::Idle)
			{
				MyObj->PathFollowingComp->RequestMoveWithImmediateFinish(EPathFollowingResult::Invalid);
			}
		}
	}

	return MyObj;
}