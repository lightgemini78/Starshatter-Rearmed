#include "CanopyRep.h"
#include "Solid.h"
#include "Pilot.h"
#include "ShipDesign.h"
#include "Ship.h"


CanopyRep::CanopyRep()
{
}

CanopyRep::CanopyRep (const CanopyRep& c)

{
	
}

CanopyRep::~CanopyRep()
{
}

void
CanopyRep::Render(Video* video,DWORD flags)
{
		Solid::Render(video, flags); 
}