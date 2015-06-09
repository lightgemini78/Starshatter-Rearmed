/*		Pilot class

*/

#include "MemDebug.h"
#include "Pilot.h"
#include "PilotRep.h"
#include "CanopyRep.h"
#include "Solid.h"
#include "Game.h"
#include "Ship.h"
#include "RadioView.h"
#include "SimEvent.h"
#include "Random.h"
#include "Physical.h"
#include "Graphic.h"
#include "ShipDesign.h"
#include "Bitmap.h"
#include "Color.h"
#include "Player.h"
#include "Sim.h"
#include "Camera.h"


static char pilot_male [32] [16] = {
	"Allan",	"Henry",	"Kieran",	"Derek",	"Morgan",		"Dimitri",
	"Imran",	"Nyle",		"Reza",		"Ron",		 "Jarrod",		"Anson",
	"Benson",	"Athan",	"Alaric",	"Trey",		"Knox",			"Paul",
	"Donnie",	"Zayne",	"Aldrin",	"Brock",	"Chester",		"Shay",
	"Darrell",	"Austen",	"Scott",	"Judd",		"Jeffrey",		"Michael",
	"Blain",	"Sheldon"
};

static char pilot_female [32] [16] = {
	"Tala",		"Cali",		"Asha",		"Ayla",		"Cayla",	"Corina",
	"Brisa",	"Haley",	"Jessica",	"Lily",		"Sarah",	"Anna",
	"Katie",	"Kara",		"Jade",		"Amy",		"Darcy",	"Anika",
	"Karen",	"Nell",		"Lori",		"Kseniya",	"Melina",	"Uma",
	"Genna",	"Alka",		"Alake",	"Ione",		"Kacey",	"Aurora",
	"Nausica",	"Deunan"
};

static char pilot_surname [32] [16] = {
	" Rorty",	" Hoss",		" Sagan",	" Cyert",	" Sears",	" Hunter",
	" Gunther",	" Melchiott",	" Nelson",	" Bielert",	" Milton",	" Nielsen",
	" Daerden",	" Valt",		" Bles",	" Jaeger",	" Knute",	" Wong",
	" Sud",		" Speirs",		" Shadel",	" Ayling",	" Moore",	" Fidler",
	" Perlin",	" Bailey",		" Witmer",	" Sugar",	" Wesson",	" Kort",
	" Hanser",	" Lo"				
};

static char pilot_rank [5] [16] = {
	"CDT",	"ENS",	"LT",	"LCMD",	"CMDR"
};


Pilot::Pilot()
	:System(PILOT,9,"Pilot",1,1,1,1), alive(true)
{
	power_flags = POWER_WATTS | POWER_CRITICAL;


}

Pilot::Pilot(const Pilot& p)
	:System(p), alive(true), ejected(false), init(false)
{
	Mount(p);
	power_flags  = POWER_WATTS | POWER_CRITICAL;

	Define();


}

Pilot::~Pilot(void)
{
}

void
Pilot::ExecFrame(double seconds)
{
	energy = 0.0f;
	System::ExecFrame(seconds);

	if(!init && gender != 1) {
		if(ship->GetPilotRep() && ship->Design()->pilot_model2) {			//** use second model for female pilot
			ship->GetPilotRep()->UseModel(ship->GetPilotRep2());
			init = true;
		}
	}


/* Get a copy of blank texture and restore it if needed.
	if(can == nullptr) {
		Model* cm  = ship->GetCanopyRep()->GetRep();
		can = (Material*) cm->FindMaterial("killed");

		if(alive && can && can->tex_diffuse->GetFilename() == can->tex_bumpmap->GetFilename()) {	//**if texture got swaped whn pilot died,restore it.
			can->tex_diffuse = pbmp;
			can->blend = 4;
			}
		else if(can) {							//**Get a copy of blank texture.Should be executed once when Pilot is created.
			Bitmap* tmp = can->tex_diffuse;
			bmp.CopyBitmap(*tmp);
			bmp.SetFilename(tmp->GetFilename());
		}
	} */

	//** check if pilot should die.
	if (alive && availability < 0.6f) {
		alive = false;
		if(ship->GetCanopyRep() && ship->Design()->pilot_canopy_dead) {
		ship->GetCanopyRep()->UseModel(ship->GetCanopyDeadRep());
		}
//		can->tex_diffuse = can->tex_bumpmap;	//** swap texture
//		can->blend = 2;
		Disabled();
	}
}

bool
Pilot::CheckAlive()
{
	if(!alive)
		return false;

	if(availability >= 0.6f)
		return true;

	else return false;
}

void
Pilot::Define()
{
	//* Using this random formula:  nMin + (int)((double)rand() / (RAND_MAX+1) * (nMax-nMin+1))

	 int g = 1 + (int)((double)rand() / (RAND_MAX+1) * (2-1+1));
	 gender = g;
	 int n = 1 + (int)((double)rand() / (RAND_MAX+1) * (31-1+1));
	 int s = 1 + (int)((double)rand() / (RAND_MAX+1) * (31-1+1));

	 if(g == 1) 
		name = pilot_male[n];

	 else name = pilot_female[n];

	 surname = pilot_surname[s];

}

void							
Pilot::Disabled()			//** leave the ship a dead derelict when pilot is dead or bailed out
{
	ship->DropTarget();
	ship->SetIFF(0);
	ship->SetControls(0);
	ship->SetThrottle(0);

	ListIter<System> iter = ship->Systems();
	while (++iter) {
		if(iter->Type() < PILOT && iter->Type() != DRIVE) {
			iter->PowerOff();
			iter->SetPowerLevel(0);
		}
	}
	ship->SetupAgility();
	Point torque = RandomVector(ship->Mass()/7);
	ship->ApplyTorque(torque);
	ship->SetDrag(2);
	ship->SetCold(true);

	if(Sim::GetSim()->GetPlayerShip() == ship) {
		if(!alive) {
			ship->SetDirectorInfo(Game::GetText("Pilot Killed"));
			const char* msg = "PILOT IS DEAD";       
			RadioView::Message(msg);
			RadioView::Message(msg);
			RadioView::Message(msg);
	}
		else if(ejected) {
			const char* msg = "BAILED OUT";
			RadioView::Message(msg);
			RadioView::Message(msg);
			RadioView::Message(msg);
		}
	}
}

void
Pilot::Eject()
{
	if(ejected)
		return;

	Sim* sim = Sim::GetSim();

	char named[32];
	char reg[64];

	sprintf_s(named, "%s" "%s", name, surname);
	strcpy_s(reg, ship->Registry());

	Text region;
	if (ship->GetRegion())
	region = ship->GetRegion()->Name();

	Matrix facing = ship->Cam().Orientation();
	Point bail	  = Vec3(0, 150, -100) * facing;
	Point loc	  = ship->Location();
	Camera s;
	s.Clone(ship->Cam());
	
	ShipDesign* design;
	Ship* eject = sim->CreateShip(named, reg, design->Get("Ejected"), region, Point (0,0,0), 0);
	if(eject) {
		eject->MoveTo(loc);
		eject->SetVelocity(ship->Velocity() + bail);
		eject->CloneCam(s);
		eject->SetTargeteable(false);

		Ship* hero_ship = sim->GetPlayerShip();
		if(ship == hero_ship) {
			eject->SetIFF(ship->GetIFF());
			ship->GetRegion()->SetPlayerShip(eject);
		}

		ejected = true;
		Disabled();

		ShipStats* dis = ShipStats::Find(ship->Name());
		dis->AddEvent(SimEvent::EJECT);
	
		if(ship->GetPilot()->GetGender() != 1) {
			Solid* s = (Solid*) eject->Rep();
			s->UseModel(ship->GetPilotRep2());
		}

		if (ship->GetCanopyRep()) {
			ship->GetCanopyRep()->Hide();
		}

		if (ship->GetPilotRep()) {
			ship->GetPilotRep()->Hide();
		}
	}
}