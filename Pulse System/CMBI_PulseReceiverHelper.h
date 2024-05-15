// Developed by Alejandro Cannizzo (alejandro.cannizzo@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CMBI_PulseReceiverHelper.generated.h"

UINTERFACE(BlueprintType)
class UCMBI_PulseReceiverHelper : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface meant to be implemented by Actors that have a PulseReceiver component
 * to facilitate referencing them from PulseEmitters.
 */
class CASMAGE_API ICMBI_PulseReceiverHelper
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	class UCMBC_PulseReceiver* GetPulseReceiver() const;
};
