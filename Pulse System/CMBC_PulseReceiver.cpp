// Developed by Alejandro Cannizzo (alejandro.cannizzo@gmail.com)


#include "CMBC_PulseReceiver.h"
#include "CMB_PulseEmitter.h"
#include "CMB_Globals.h"

UCMBC_PulseReceiver::UCMBC_PulseReceiver()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCMBC_PulseReceiver::RegisterPulseEmitter(ACMB_PulseEmitter* newPulseEmitter)
{
	if (!IsValid(newPulseEmitter)) return;

	connectedPulseEmitters.AddUnique(newPulseEmitter);
	PulseEmitterUpdated(newPulseEmitter);
}

void UCMBC_PulseReceiver::BeginPlay()
{
	Super::BeginPlay();
}

void UCMBC_PulseReceiver::PulseEmitterUpdated(ACMB_PulseEmitter* pulseEmitter)
{
	bool currentPulse = !canBePoweredPartially;

	for (ACMB_PulseEmitter* emitter : connectedPulseEmitters)
	{
		check(IsValid(emitter))

		if (canBePoweredPartially)
			currentPulse |= emitter->GetPulse();
		else
			/*
			Could break out here if currentPulse == false but will prefer
			to keep iterating as a safety measure to ensure all connectedPulseEmitters
			are valid
			*/
			currentPulse &= emitter->GetPulse();
	}

	bool pulseChanged = currentPulse != lastRegisteredPulse;

	lastRegisteredPulse = currentPulse;

	if (pulseChanged)
	{
		/*
		We'll truncate any PulseUpdated events that happen before the Owner's BeginPlay call.
		This is a safety measure to ensure that these calls are only processed by Actors that have been properly
		initialized. This helps deal with issues related to Save states, and ensures that the values available
		when these calls happen are the correct ones.
		*/
		if (!GetOwner()->HasActorBegunPlay())
			return;
		else if (OnReceivedPulseUpdated.IsBound())
		{
			OnReceivedPulseUpdated.Broadcast(GetPulse());
		}
	}
}

bool UCMBC_PulseReceiver::GetPulse() const
{
	//XOR
	return lastRegisteredPulse != invertPulse;
}
