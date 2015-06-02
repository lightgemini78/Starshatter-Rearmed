/*
	STARS.

	Pilot Class model representation.

*/


#include "PilotRep.h"
#include "Solid.h"


PilotRep::PilotRep()
{

}


PilotRep::~PilotRep()
{
}

void
PilotRep::Render(Video* video,DWORD flags)
{
		Solid::Render(video, flags); 
}