// Developed by Alejandro Cannizzo (alejandro.cannizzo@gmail.com)


#include "CMB_PulseEmitter.h"
#include "CMB_Globals.h"
#include "Net/UnrealNetwork.h"
#include "CMBC_PulseReceiver.h"
#include "CMBI_PulseReceiverHelper.h"
#include "Systems/Save/CMB_SaveGameSubsystem.h"

// Sets default values
ACMB_PulseEmitter::ACMB_PulseEmitter(const class FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACMB_PulseEmitter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	/*
	ACMB_Actor::spawnActive determines the initial Active state of the Actor when spawned
	*/
	active = spawnActive;

	pulse = startingPulse;
	SetPulse(pulse);

	/*
	For each connected PulseReceiver, ensure they are valid and bind them to this PulseEmitter
	*/
	for (TObjectPtr<AActor> receiver : connectedReceivers)
	{
		if (!IsValid(receiver))
		{
			UE_LOG(CasmagePulsesLog, Error, TEXT("%s is connected to an Invalid Actor."), *GetName());
			continue;
		}

		if (!receiver->GetClass()->ImplementsInterface(UCMBI_PulseReceiverHelper::StaticClass()))
		{
			UE_LOG(CasmagePulsesLog, Error, TEXT("%s is connected to %s which doesn't implement the CMBI_PulseReceiverHelper."), *GetName(), *receiver->GetName());
			continue;
		}

		UCMBC_PulseReceiver* pulseReceiver = ICMBI_PulseReceiverHelper::Execute_GetPulseReceiver(receiver);
		if (pulseReceiver)
		{
			pulseReceiver->RegisterPulseEmitter(this);
		}
	}
}

void ACMB_PulseEmitter::BeginPlay()
{
	Super::BeginPlay();

	//Called here to perform any initial setup BP side
	K2_PulseUpdated();
}

void ACMB_PulseEmitter::Destroyed()
{
	//We don't want this to be called in editor
	if (GetGameInstance())
	{
		UCMB_SaveGameSubsystem* saveGameSubsys = GetGameInstance()->GetSubsystem<UCMB_SaveGameSubsystem>();
		saveGameSubsys->QuickSaveActor(this);
	}

	GetWorldTimerManager().ClearAllTimersForObject(this);

	Super::Destroyed();
}

void ACMB_PulseEmitter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACMB_PulseEmitter, pulse);
	DOREPLIFETIME(ACMB_PulseEmitter, toggleIsLocked);
}

//https://benui.ca/unreal/uproperty-edit-condition-can-edit-change/
#if WITH_EDITOR
bool ACMB_PulseEmitter::CanEditChange(const FProperty* InProperty) const
{
	// If other logic prevents editing, we want to respect that
	const bool ParentVal = Super::CanEditChange(InProperty);

	// Can we edit flower color?
	if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ACMB_PulseEmitter, timerModeDuration))
	{
		return ParentVal && emitterMode == EPulseEmitterMode::TIMER;
	}

	return ParentVal;
}
#endif

void ACMB_PulseEmitter::SetPulse(bool on)
{
	if (!HasAuthority()) return;

	if (on == pulse)
		return;

	pulse = on;

	//Servers do not get to call RepNotify automatically, so we do it here manually
	OnRep_Pulse(!pulse);
}

void ACMB_PulseEmitter::TimerCountdownDone()
{
	if (!GetWorld()) return;

	if (emitterMode == EPulseEmitterMode::TIMER && GetPulse())
	{
		SetPulse(false);
	}

	GetWorldTimerManager().ClearAllTimersForObject(this);
}

bool ACMB_PulseEmitter::ShouldBeSaved_Implementation() const
{
	return true;
}

void ACMB_PulseEmitter::OnActorLoaded_Implementation()
{
	switch (emitterMode)
	{
	case EPulseEmitterMode::TOGGLE_LOCK:
		/*
		toggleIsLocked is Saved, so TOGGLE_LOCK emitters will
		be automatically locked if they were locked in the past
		*/
	case EPulseEmitterMode::TOGGLE:
		startingPulse = pulse;
		/*
		We don't care about the saved value of startingPulse for HOLD and TIMER modes
		since these always revert to default automatically
		*/
	case EPulseEmitterMode::HOLD:
	case EPulseEmitterMode::TIMER:
		spawnActive = active;
	default:break;
	}
}

void ACMB_PulseEmitter::OnRep_Pulse(bool previousPulse)
{
	if (OnPulseUpdated.IsBound())
		OnPulseUpdated.Broadcast(this, pulse);

	/*
	Inform all connected PulseReceivers of the pulse update
	*/
	for (TObjectPtr<AActor> receiver : connectedReceivers)
	{
		if (!IsValid(receiver))
		{
			UE_LOG(CasmagePulsesLog, Warning, TEXT("%s has an Invalid Connected Receiver."), *GetName());
			continue;
		}
		UCMBC_PulseReceiver* pulseReceiver = ICMBI_PulseReceiverHelper::Execute_GetPulseReceiver(receiver);
		if (pulseReceiver)
			pulseReceiver->PulseEmitterUpdated(this);
	}

	//Expose to BP to handle additional responses toit
	K2_PulseUpdated();
}

bool ACMB_PulseEmitter::TryEngage(AActor* engagingActor)
{
	if (!CanEngage() || !CanBeEngagedBy(engagingActor)) return false;

	switch (emitterMode)
	{
	case EPulseEmitterMode::TOGGLE:
		SetPulse(!pulse);
		GetWorldTimerManager().SetTimer(toggleCooldownHandle, this, &ACMB_PulseEmitter::TimerCountdownDone, toggleCooldownDuration, false);
		break;
	case EPulseEmitterMode::HOLD:
		SetPulse(true);
		break;
	case EPulseEmitterMode::TOGGLE_LOCK:
		if (!toggleIsLocked)
		{
			SetPulse(true);
			toggleIsLocked = true;
			return true;
		}
		return false;
	case EPulseEmitterMode::TIMER:
		SetPulse(true);
		if (timerModeDuration <= 0.0f)
			GetWorldTimerManager().SetTimerForNextTick(this, &ACMB_PulseEmitter::TimerCountdownDone);
		else
			GetWorldTimerManager().SetTimer(timerModeHandle, this, &ACMB_PulseEmitter::TimerCountdownDone, timerModeDuration, false);
	default:break;
	}

	return true;
}

void ACMB_PulseEmitter::TryDisengage()
{
	if (!CanDisengage()) return;

	switch (emitterMode)
	{
	case EPulseEmitterMode::HOLD:
		SetPulse(false);
		break;
	default:break;
	}
}

bool ACMB_PulseEmitter::GetPulse() const
{
	return pulse;
}

bool ACMB_PulseEmitter::CanEngage() const
{
	if (!IsActive()) return false;

	switch (emitterMode)
	{
	case EPulseEmitterMode::TOGGLE:
		return !GetWorldTimerManager().IsTimerActive(toggleCooldownHandle);
	case EPulseEmitterMode::HOLD:
		return !GetPulse();
	case EPulseEmitterMode::TOGGLE_LOCK:
		return !toggleIsLocked;
	case EPulseEmitterMode::TIMER:
		return !GetWorldTimerManager().IsTimerActive(timerModeHandle);
	default:
		return false;
	}
}

bool ACMB_PulseEmitter::CanDisengage() const
{
	switch (emitterMode)
	{
	case EPulseEmitterMode::HOLD:
		return GetPulse();
	case EPulseEmitterMode::TOGGLE:
	case EPulseEmitterMode::TOGGLE_LOCK:
	case EPulseEmitterMode::TIMER:
	default:
		return false;
	}
}

bool ACMB_PulseEmitter::IsToggleLocked() const
{
	return toggleIsLocked;
}

bool ACMB_PulseEmitter::IsEmitterMode(EPulseEmitterMode mode) const
{
	return emitterMode == mode;
}

void ACMB_PulseEmitter::GetTimerModeState(bool& timerActive, float& totalDuration, float& normalizedTimer) const
{
	if (emitterMode != EPulseEmitterMode::TIMER)
	{
		timerActive = false;
		totalDuration = 0.0f;
		normalizedTimer = 0.0f;
	}
	else
	{
		timerActive = GetWorldTimerManager().IsTimerActive(timerModeHandle);
		totalDuration = timerModeDuration;
		if (timerActive && timerModeDuration > 0.0f)
			normalizedTimer = GetWorldTimerManager().GetTimerElapsed(timerModeHandle) / timerModeDuration;
		else
			normalizedTimer = 0.0f;
	}
}
