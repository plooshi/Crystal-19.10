#pragma once
#include "pch.h"
#include "Utils.h"


struct FAbilityReplicatedData
{
	/** Event has triggered */
	bool bTriggered;

	/** Optional Vector payload for event */
	FVector_NetQuantize100 VectorPayload;

	/** Delegate that will be called on replication */
	TMulticastInlineDelegate<void()> Delegate;
};

struct FAbilityReplicatedDataCache
{
	/** What elements this activation is targeting */
	FGameplayAbilityTargetDataHandle TargetData;

	/** What tag to pass through when doing an application */
	FGameplayTag ApplicationTag;

	/** True if we've been positively confirmed our targeting, false if we don't know */
	bool bTargetConfirmed;

	/** True if we've been positively cancelled our targeting, false if we don't know */
	bool bTargetCancelled;

	/** Delegate to call whenever this is modified */
	TDelegate<void(FGameplayAbilityTargetDataHandle&, FGameplayTag)> TargetSetDelegate;

	/** Delegate to call whenever this is confirmed (without target data) */
	TMulticastInlineDelegate<void()> TargetCancelledDelegate;

	/** Generic events that contain no payload data */
	FAbilityReplicatedData	GenericEvents[12];

	/** Prediction Key when this data was set */
	FPredictionKey PredictionKey;
};

struct FGameplayAbilityReplicatedDataContainer
{
public:
	typedef TPair<FGameplayAbilitySpecHandleAndPredictionKey, TSharedRef<FAbilityReplicatedDataCache>> FKeyDataPair;

	TArray<FKeyDataPair> InUseData;
	TArray<TSharedRef<FAbilityReplicatedDataCache>> FreeData;
};

class Abilities {
private:
	static void InternalServerTryActivateAbility(UFortAbilitySystemComponentAthena*, FGameplayAbilitySpecHandle, bool, FPredictionKey&, FGameplayEventData*);
public:
	static void GiveAbility(UAbilitySystemComponent*, UObject*);
	static void GiveAbilitySet(UAbilitySystemComponent*, UFortAbilitySet*);

	InitHooks;
};
