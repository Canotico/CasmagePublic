# CasmagePublic
Public repo with some systems used in the game I'm making, Casmage, which is a WIP and subject to change constantly.



## Pulse System

A pulse-based system to allow for easy creation of things like Switches, Buttons, Levers, etc, in a Networked environment.

The system defines the following elements:
	- PulseEmitter (Actor)
	- PulseReceiver (Component)
	- PulseReceiverHelper (Interface)

### PulseEmitter

It's an Actor capable of sending a Pulse to any connected PulseReceivers.

#### Replication

It's Pulse value is **replicated** and also Saved so as to preserve its latest state.

#### Modes

PulseEmitters can be Engaged/Disengaged with, which allows the engaging Actor (commonly the PlayerCharacter) to directly alter their Pulse value. 
The different modes an emitter can have are:
	- **TOGGLE**
		- When engaged, the Pulse value is flipped
	- **HOLD**
		- When engaged, the Pulse value is set to TRUE
		- When disengaged, the Pulse value returns to FALSE
	- **TIMER**
		- When engaged, the Pulse value is set to TRUE
		- After a specific amount of time, the emitter will set its Pulse value back to FALSE
	- **TOGGLE_LOCK**
		- When engaged, the Pulse value is set to TRUE
		- After the initial engagement, the emitter will not be engageable anymore

Additionally, it's possible to set a PulseEmitter to invert their Pulse value, in the event that we want to essentially change
what being ON/OFF means contextually.


### PulseReceiver

It's an Component capable of listening to 1 or more PulseEmitters and becoming Powered ON/OFF as a result.

It's effectively the counterpart to PulseEmitter, but we've made it a Component to facilitate the existence of receivers
in the game. By doing it like this, we don't need special dedicated Actors to listen to PulseEmitters, we can just give them
the PulseReceiver component on demand.

#### Replication

These are **NOT replicated**. Since their job is to listen to PulseEmitters, which ARE replicated, there's little point in
having them replicate their state as well. We know that whenever a client sees a PulseEmitter is TRUE, every receiver listening
to it will respond accordingly.

#### Modes

A PulseReceiver can be "powered On" in 2 different ways:
	- All PulseEmitters are ON
	- At least 1 PulseEmitter is ON

This is defined on a per-receiver basis.


### PulseReceiverHelper

It's an Interface whose purpose is to just expose the PulseReceiver component reference. This makes it easier on the PulseEmitter's side to bind itself to any connected receivers, which makes it so that pretty much any kind of
Actor can subscribe to this system easily.
