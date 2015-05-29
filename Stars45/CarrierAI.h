/*  Project Starshatter 4.5
	Destroyer Studios LLC
	Copyright © 1997-2004. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         CarrierAI.h
	AUTHOR:       John DiCamillo


	OVERVIEW
	========
	"Air Boss" AI class for managing carrier fighter squadrons
*/

#ifndef CarrierAI_h
#define CarrierAI_h

#include "Types.h"
#include "Director.h"
#include "Element.h"
#include "Ship.h"

// +--------------------------------------------------------------------+

class Sim;
class Ship;
class ShipAI;
class Instruction;
class Hangar;
class Element;
class FlightPlanner;

// +--------------------------------------------------------------------+

class CarrierAI : public Director
{
public:
	CarrierAI(Ship* s, int level);
	virtual ~CarrierAI();

	virtual void      ExecFrame(double seconds);


	struct			Deploy 
	{
		Element*	id;
		int			count;
		Point		position;
		Element*	target;
		bool		active;
		bool		assigned;

		Deploy() : id(0), count(0), position(0,0,0), target(0), active(false), assigned(false) {}

	}g1, g2, g3 ,g4 ;



protected:
	virtual bool      CheckPatrolCoverage();
	virtual bool      CheckHostileElements(Element* Elem);
	virtual void	  Guard(Element* elem, int size);
	virtual void	  Deployment();
	virtual void	  CarrierCommand();
	virtual void	  Balance();

	virtual bool      CreateStrike(Element* elem);

	virtual Element*  CreatePackage(int squad, int size, int code, const char* target=0, const char* loadname=0);
	virtual bool      LaunchElement(Element* elem);

	Sim*              sim;
	Ship*             ship;
	Hangar*           hangar;
	FlightPlanner*    flight_planner;
	int               exec_time;
	int               hold_time;
	int               ai_level;

	Element*          patrol_elem[4];



	bool			superior_small;
	bool			superior_big;

};

// +--------------------------------------------------------------------+

#endif CarrierAI_h

