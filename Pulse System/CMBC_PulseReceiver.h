// Developed by Alejandro Cannizzo (alejandro.cannizzo@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CMBC_PulseReceiver.generated.h"

class UCMBC_PulseEmitter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPulseReceiverEvent, bool, receivedPulse);

/*
Component that acts as a simple mechanism that can be Powered ON/OFF based on PulseEmitter pulses.
Essentially observes all PulseEmitters it's connected to, reacting to changes in their Pulse state to determine if it
should Power ON/OFF.

This Component is NOT REPLICATED, as it's unnecessary, since clients can infer the state of every PulseReceiver by the state of the 
PulseEmitters they are connected to, which are Replicated.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CASMAGE_API UCMBC_PulseReceiver : public UActorComponent
{
	GENERATED_BODY()
private:
	bool lastRegisteredPulse;

protected:
	virtual void BeginPlay() override;

public:	

	UCMBC_PulseReceiver();

	/*
	Emitters this PulseReceiver is listening to.
	We do not use TSharedPtr for these as a deliberate design decision
	to force all connected Pulse Emitters/Receives to live in the same Level.
	*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Pulse Receiver")
	TArray<TObjectPtr<ACMB_PulseEmitter>> connectedPulseEmitters;

	/*
	Should we invert the Powered state of this Receiver?
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pulse Receiver")
	bool invertPulse;

	/*
	If set to TRUE, any Emitter with a TRUE Pulse will Power this Receiver
	If set to FALSE, all Emitters are required to have a TRUE Pulse in order to Power this Receiver
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pulse Receiver")
	bool canBePoweredPartially;

	/*
	Called when changes in connected PulseEmitters cause the GetPulse value of this Receiver to change.
	IMPORTANT: These calls only happen after the Owner's BeginPlay has been called. However, the value of
	GetPulse() will always be updated, so during BeginPlay it will have the most up to date value.
	*/
	UPROPERTY(BlueprintAssignable, Category = "Pulse Receiver")
	FPulseReceiverEvent OnReceivedPulseUpdated;
	
	UFUNCTION(BlueprintCallable)
	void RegisterPulseEmitter(ACMB_PulseEmitter* newPulseEmitter);

	/*
	Make this PulseReceiver listen to a new PulseEmitter
	*/
	UFUNCTION(BlueprintCallable)
	void PulseEmitterUpdated(ACMB_PulseEmitter* pulseEmitter);

	UFUNCTION(BlueprintPure)
	bool GetPulse() const;
};
