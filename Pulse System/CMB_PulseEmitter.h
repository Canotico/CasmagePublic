// Developed by Alejandro Cannizzo (alejandro.cannizzo@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "CMB_Actor.h"
#include "CMBI_Activateable.h"
#include "Systems/Save/CMBI_Saveable.h"
#include "CMB_PulseEmitter.generated.h"

UENUM(BlueprintType)
enum class EPulseEmitterMode : uint8
{
	NONE = 0,
	TOGGLE = 1			UMETA(Tooltip = "When Engaged it will invert its current Pulse"),
	HOLD = 2			UMETA(Tooltip = "When Engaged Pulse will be On until Disengaged"),
	TOGGLE_LOCK = 3		UMETA(Tooltip = "Same as TOGGLE but after Engaged the first time, it won't be engabeable anymore."),
	TIMER = 4			UMETA(Tooltip = "After Engaged, an internal Timer will start counting down, and when finished, will automatically flip the Pulse.")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPulseEmitterEvent, ACMB_PulseEmitter*, pulseEmitter, bool, pulse);

UCLASS(Abstract, Blueprintable)
class CASMAGE_API ACMB_PulseEmitter : public ACMB_Actor, public ICMBI_Saveable
{
	GENERATED_BODY()

private:

	/*
	Current value of the Pulse.
	Saved to preserve latest state of the PulseEmitter.
	*/
	UPROPERTY(ReplicatedUsing = OnRep_Pulse, SaveGame)
	bool pulse;

	UPROPERTY(Replicated, SaveGame)
	bool toggleIsLocked;

	/*
	Toggling the PulseEmitter has a "hidden" cooldown to prevent per-frame spamming
	*/
	FTimerHandle toggleCooldownHandle;
	float toggleCooldownDuration = 0.1f;

	/*
	Timer used in the event the PulseEmitter is in TIMER mode
	*/
	FTimerHandle timerModeHandle;

	void SetPulse(bool on);

protected:

	/*
	Called when any timer is done with its Countdown.
	These can either be the ToggleCooldown anti-spam timer or the TIMER timer countdown.
	*/
	virtual void TimerCountdownDone();

public:	

	ACMB_PulseEmitter(const class FObjectInitializer& ObjectInitializer);

	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	virtual void Destroyed() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

	/*
	Contains all the PulseReceivers that are currently listening to this PulseEmitter.
	We do not use TSharedPtr for these as a deliberate design decision
	to force all connected Pulse Emitters/Receives to live in the same Level.
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pulse Emitter")
	TArray<TObjectPtr<AActor>> connectedReceivers;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pulse Emitter")
	EPulseEmitterMode emitterMode = EPulseEmitterMode::TOGGLE;

	/*
	Determines how long the TIMER Mode will last
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pulse Emitter")
	float timerModeDuration = 1.0f;

	/*
	Determine what's the default state of this PulseEmitter's pulse
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pulse Emitter")
	bool startingPulse = false;

	UPROPERTY(BlueprintAssignable, Category = "Pulse Emitter")
	FPulseEmitterEvent OnPulseUpdated;

	/*
	Called whenever this PulseEmitter can or can't be engaged.
	Mostly used to inform the Player of when they should show the Interaction widget.
	*/
	UPROPERTY(BlueprintAssignable, Category = "Pulse Emitter")
	FPulseEmitterEvent OnCanEngageUpdated;

	/* SAVEABLE INTERFACE */
	bool ShouldBeSaved_Implementation() const;
	void OnActorLoaded_Implementation();
	////////////////////////

	UFUNCTION()
	void OnRep_Pulse(bool previousPulse);

	/*
	Called on Server and Clients whenever Enabled's value changes.
	Called initially at the end of BeginPlay() (after EnabledUpdated())
	*/
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Pulse Updated"))
	void K2_PulseUpdated();

	/*
	Attempt to Engage this PulseEmitter
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Pulse Emitter")
	bool TryEngage(AActor* engagingActor);

	/*
	What happens when successfully engaged
	*/
	UFUNCTION(BlueprintNativeEvent)
	void Engage(AActor* engagingActor);
	virtual void Engage_Implementation(AActor* engagingActor) {};

	/*
	Attempt to Disengage this PulseEmitter
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Pulse Emitter")
	void TryDisengage();

	/*
	What happens when successfully disengaged
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Pulse Emitter")
	void Disengage();
	virtual void Disengage_Implementation() {};

	UFUNCTION(BlueprintCallable, Category = "Pulse Emitter")
	bool GetPulse() const;

	UFUNCTION(BlueprintCallable, Category = "Pulse Emitter")
	bool CanEngage() const;

	UFUNCTION(BlueprintPure, Category = "Pulse Emitter")
	bool CanDisengage() const;

	/*
	Returns TRUE after the first engagement if the mode is TOGGLE_LOCK
	*/
	UFUNCTION(BlueprintPure, Category = "Pulse Emitter")
	bool IsToggleLocked() const;

	UFUNCTION(BlueprintPure, Category = "Pulse Emitter")
	bool IsEmitterMode(EPulseEmitterMode mode) const;

	/*
	Get information on the current state of this Pulse Emitter's Timer.
	Only really useable when EmitterMode == TIMER
	*/
	UFUNCTION(BlueprintPure, Category = "Pulse Emitter")
	void GetTimerModeState(bool& timerActive, float& totalDuration, float& normalizedTimer) const;

	/*
	Determines if this PulseEmitter can be Engaged by the EngagingActor
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Pulse Emitter")
	bool CanBeEngagedBy(AActor* engagingActor);
	virtual bool CanBeEngagedBy_Implementation(AActor* engagingActor) { return true;  };
};
