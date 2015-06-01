/*  Project Starshatter 4.5
	Destroyer Studios LLC
	Copyright © 1997-2004. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         ShipKiller.h
	AUTHOR:       John DiCamillo


	OVERVIEW
	========
	ShipKiller (i.e. death spiral) class
*/

#ifndef ShipKiller_h
#define ShipKiller_h

#include "Types.h"
#include "Geometry.h"
#include "Text.h"
#include "Camera.h"

// +--------------------------------------------------------------------+

class Ship;

// +--------------------------------------------------------------------+

class ShipKiller
{
public:
	const float       DEATH_CAM_LINGER;

	// CONSTRUCTORS:
	ShipKiller(Ship* ship);
	virtual ~ShipKiller();

	virtual void      BeginDeathSpiral(bool instakill);
	virtual void      ExecFrame(double seconds);

	// GENERAL ACCESSORS:
	virtual float     TransitionTime() const { return time; }
	virtual Point     TransitionLoc()  const { return loc;  }

	virtual Camera*   GetCamera()            { return &camera; }

protected:
	Ship*             ship;

	float             time;
	Point             loc;

	float             exp_time;
	int               exp_index;
	Camera			  camera;
};

#endif ShipKiller_h

