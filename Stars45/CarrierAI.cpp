/*  Project Starshatter 4.5
	Destroyer Studios LLC
	Copyright © 1997-2004. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         CarrierAI.cpp
	AUTHOR:       John DiCamillo


	OVERVIEW
	========
	"Air Boss" AI class for managing carrier fighter squadrons
*/

#include "MemDebug.h"
#include "CarrierAI.h"
#include "ShipAI.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Element.h"
#include "FlightPlanner.h"
#include "Instruction.h"
#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "Hangar.h"
#include "FlightDeck.h"
#include "Mission.h"
#include "Contact.h"
#include "Sim.h"
#include "StarSystem.h"
#include "Callsign.h"
#include "NetUtil.h"
#include "CombatUnit.h"

#include "Game.h"
#include "Random.h"

// +----------------------------------------------------------------------+

CarrierAI::CarrierAI(Ship* s, int level)
: sim(0), ship(s), hangar(0), exec_time(0), flight_planner(0),
hold_time(0), ai_level(level)
{
	if (ship) {
		sim      = Sim::GetSim();
		hangar = ship->GetHangar();

		for (int i = 0; i < 4; i++)
		patrol_elem[i] = 0;


		if (ship)
		flight_planner = new(__FILE__,__LINE__) FlightPlanner(ship);

		hold_time = (int) Game::GameTime();
	}
}

CarrierAI::~CarrierAI()
{
	delete flight_planner;
}

// +--------------------------------------------------------------------+

void
CarrierAI::ExecFrame(double secs)
{
	const int INIT_HOLD   = 15000;
	const int EXEC_PERIOD =  3000;

	if (!sim || !ship || !hangar)
	return;

	if (((int) Game::GameTime() - hold_time >= INIT_HOLD) && 
			((int) Game::GameTime() - exec_time >  EXEC_PERIOD)) {

		CheckPatrolCoverage();

		if((int) Game::GameTime() - exec_time > 15000)
		Balance();

		CarrierCommand();

		exec_time = (int) Game::GameTime();
	}
}

// +--------------------------------------------------------------------+

bool
CarrierAI::CheckPatrolCoverage()
{
	const DWORD PATROL_PERIOD = 900 * 1000;

	// pick up existing patrol elements:

	ListIter<Element> iter = sim->GetElements();
	while (++iter) {
		Element* elem = iter.value();

		if (elem->GetCarrier()   == ship                   &&
				(elem->Type()         == Mission::PATROL        ||
					elem->Type()         == Mission::SWEEP         ||
					elem->Type()         == Mission::AIR_PATROL    ||
					elem->Type()         == Mission::AIR_SWEEP)    &&
				!elem->IsSquadron()                             &&
				!elem->IsFinished()) {

			bool found = false;
			int  open  = -1;

			for (int i = 0; i < 4; i++) {
				if (patrol_elem[i] == elem)
				found = true;

				else if (patrol_elem[i] == 0 && open < 0)
				open = i;
			}

			if (!found && open >= 0) {
				patrol_elem[open] = elem;
			}
		}
	}

	// manage the four screening patrols:

	for (int i = 0; i < 4; i++) {
		Element* elem = patrol_elem[i];

		if (elem) {
			if (elem->IsFinished()) {
				patrol_elem[i] = 0;
			}

			else {
				LaunchElement(elem);
			}
		}

		else if (Game::GameTime() - hangar->GetLastPatrolLaunch() > PATROL_PERIOD ||
				hangar->GetLastPatrolLaunch() == 0) {
			Element* patrol = CreatePackage(0, 2, Mission::PATROL, 0, "ACM Medium Range");
			if (patrol) {
				patrol_elem[i] = patrol;

				if (flight_planner)
				flight_planner->CreatePatrolRoute(patrol, i);

				hangar->SetLastPatrolLaunch(Game::GameTime());
				return true;
			}
		}
	}

	return false;
}

// +--------------------------------------------------------------------+

bool
CarrierAI::CheckHostileElements(Element* elem)
{
	List<Element>     assigned;
/*	ListIter<Element> iter = sim->GetElements();
	while (++iter) {
		Element* elem = iter.value();
*/
		// if this element is hostile to us
		// or if the element is a target objective
		// of the carrier, or is hostile to any
		// of our squadrons...

		bool hostile = false;

		if (elem->IsHostileTo(ship) || elem->IsObjectiveTargetOf(ship)) {
			hostile = true;
		}
		else {
			for (int i = 0; i < hangar->NumSquadrons() && !hostile; i++) {
				int squadron_iff = hangar->SquadronIFF(i);

				if (elem->IsHostileTo(squadron_iff))
				hostile = true;
			}
		}

		if (hostile) {
			sim->GetAssignedElements(elem, assigned);

			// is one of our fighter elements already assigned to this target?
			bool found = false;
			ListIter<Element> a_iter = assigned;
			while (++a_iter && !found) {
				Element* a = a_iter.value();

				if (a->GetCarrier() == ship)
				found = true;
			}

			// nobody is assigned yet, create an attack package
			if (!found ) { //&& CreateStrike(elem)) {
				//hold_time = (int) Game::GameTime() + 30000;
				return true;
			}
		}
	

	return false;
}

bool
CarrierAI::CreateStrike(Element* elem)
{
	Element* strike = 0;
	Ship*    target = elem->GetShip(1);

	if (target && !target->IsGroundUnit()) {
		Contact* contact = ship->FindContact(target);
		if (contact && contact->GetIFF(ship) > 0) {

			// fighter intercept
			if (target->IsDropship()) {
				int squadron = 0;
				if (hangar->NumShipsReady(1) >= hangar->NumShipsReady(0))
				squadron = 1;

				int count = 2;

				if (count < elem->NumShips())
				count = elem->NumShips();

				strike = CreatePackage(squadron, count, Mission::INTERCEPT, elem->Name(), "ACM Medium Range");

				if (strike) {
					strike->SetAssignment(elem);

					if (flight_planner)
					flight_planner->CreateStrikeRoute(strike, elem);
				}
			}

			// starship or station assault      
			else {
				int squadron = 0;
				if (hangar->NumSquadrons() > 1)
				squadron = 1;
				if (hangar->NumSquadrons() > 2)
				squadron = 2;

				int count = 2;

				if (target->Class() > Ship::FRIGATE) {        //**** First check, sends 1 strike group.
					count = 6;
					strike = CreatePackage(squadron, count, Mission::ASSAULT, elem->Name(), "Hvy Ship Strike");
					if (strike) {
					strike->SetAssignment(elem);
					
						if (flight_planner)
						flight_planner->CreateStrikeRoute(strike, elem);
					
					}
				
					if (target->Class() > Ship::DESTROYER) {       //****** Second check, sends a second strike if target is big.
						strike = CreatePackage(squadron,count, Mission::ASSAULT, elem->Name(), "Hvy Ship Strike");
					 if (strike) {
					 strike ->SetAssignment(elem);
					
					if (flight_planner)
					flight_planner->CreateStrikeRoute(strike, elem);
					 }
				 }
				}
				else {               //******** Lesser targets dealt with only 4 fighters.
					count = 4;
					strike = CreatePackage(squadron, count, Mission::ASSAULT, elem->Name(), "Ship Strike");
				

				if (strike) {
					strike->SetAssignment(elem);
					
					if (flight_planner)
					flight_planner->CreateStrikeRoute(strike, elem);
				  }
				}
										 					// strike escort if target has fighter protection:
				if (target->GetHangar()) {
					if (squadron > 1) squadron--;
						Element* circus = CreatePackage(squadron, 4, Mission::SWEEP, 0, "ACM Short Range");

					if (circus && flight_planner)
						flight_planner->CreateCircusRoute(circus, strike);
						}					

			}
		}
	}

	return strike != 0;
}

// +--------------------------------------------------------------------+

Element*
CarrierAI::CreatePackage(int squadron, int size, int code, const char* target, const char* loadname)
{
	if (squadron < 0 || size < 1 || code < Mission::PATROL || hangar->NumShipsReady(squadron) < size)
	return 0;

	Sim*        sim    = Sim::GetSim();
	const char* call   = sim->FindAvailCallsign(ship->GetIFF());
	Element*    elem   = sim->CreateElement(call, ship->GetIFF(), code);
	FlightDeck* deck   = 0;
	int         queue  = 1000;
	int*        load   = 0;
	const ShipDesign* 
	design = hangar->SquadronDesign(squadron);

	elem->SetSquadron(hangar->SquadronName(squadron));
	elem->SetCarrier(ship);

	if (target) {
		int i_code = 0;

		switch (code) {
		case Mission::ASSAULT:     i_code = Instruction::ASSAULT;   break;
		case Mission::STRIKE:      i_code = Instruction::STRIKE;    break;

		case Mission::AIR_INTERCEPT:
		case Mission::INTERCEPT:   i_code = Instruction::INTERCEPT; break;

		case Mission::SWEEP:	   i_code = Instruction::SWEEP; break;

		case Mission::ESCORT:
		case Mission::ESCORT_STRIKE:
		case Mission::ESCORT_FREIGHT:
			i_code = Instruction::ESCORT;    break;

		case Mission::DEFEND:      i_code = Instruction::DEFEND;    break;
		}

		Instruction* objective = new(__FILE__,__LINE__) Instruction(i_code, target);
		if (objective)
		elem->AddObjective(objective);
	}

	if (design && loadname) {
		Text name = loadname;
		name.setSensitive(false);

		ListIter<ShipLoad> sl = (List<ShipLoad>&) design->loadouts;
		while (++sl) {
			if (name == sl->name) {
				load = sl->load;
				elem->SetLoadout(load);
			}
		}
	}

	for (int i = 0; i < ship->NumFlightDecks(); i++) {
		FlightDeck* d = ship->GetFlightDeck(i);

		if (d && d->IsLaunchDeck()) {
			int dq = hangar->PreflightQueue(d);

			if (dq < queue) {
				queue = dq;
				deck  = d;
			}
		}
	}

	int npackage = 0;
	int slots[6];

	for (int i = 0; i < 6; i++)
	slots[i] = -1;

	for (int slot = 0; slot < hangar->SquadronSize(squadron); slot++) {
		const HangarSlot* s = hangar->GetSlot(squadron, slot);

		if (hangar->GetState(s) == Hangar::STORAGE) {
			if (npackage < 6)
			slots[npackage] = slot;

			hangar->GotoAlert(squadron, slot, deck, elem, load, code > Mission::SWEEP);
			npackage++;

			if (npackage >= size)
			break;
		}
	}

	NetUtil::SendElemCreate(elem, squadron, slots, code <= Mission::SWEEP);

	return elem;
}

// +--------------------------------------------------------------------+

bool
CarrierAI::LaunchElement(Element* elem)
{
	bool result = false;

	if (!elem)
	return result;

	for (int squadron = 0; squadron < hangar->NumSquadrons(); squadron++) {
		for (int slot = 0; slot < hangar->SquadronSize(squadron); slot++) {
			const HangarSlot* s = hangar->GetSlot(squadron, slot);

			if (hangar->GetState(s) == Hangar::ALERT &&
					hangar->GetPackageElement(s) == elem) {

				hangar->Launch(squadron, slot);
				NetUtil::SendShipLaunch(ship, squadron, slot);

				result = true;
			}
		}
	}

	return result;
}

void
CarrierAI::Guard(Element* elem, int size)
{
	if (elem) {

		int squadron = 0;
		if (hangar->NumShipsReady(1) >= hangar->NumShipsReady(0))
		squadron = 1;

		Element* escort = CreatePackage(squadron, size, Mission::ESCORT, elem->Name(), "ACM Short Range");

		if (escort && flight_planner) {
			flight_planner->CreateGuardRoute(escort, elem);
			elem->SetGuards(escort);
			elem->SetGuarded(true);
		}
	}
}

void
CarrierAI::Deployment()
{
	if(hangar->NumSquadrons() < 1) return;

	int max = hangar->NumSquadrons() - 1;
	int squadron = 0;	
	if(max < 1 && hangar->SquadronDesign(max)->type != Ship::FIGHTER)
		return;

	while(squadron < max) {
		if(hangar->SquadronDesign(squadron++)->type == Ship::FIGHTER) {
			if(hangar->NumShipsReady(squadron++) >= hangar->NumShipsReady(squadron) )
			squadron++;
		}
		max = squadron;
	}

	Element* squad1 = CreatePackage(squadron, 6, Mission::SWEEP, 0, "ACM Medium Range");


	if(squad1 && flight_planner) {
		if(!g1.active){
			g1.id=squad1; g1.count=6; g1.active=true;
			flight_planner->CreateDeployPoint(squad1,ship->GetElement());
		}
		else if(!g2.active) {
			g2.id=squad1; g2.count=6; g2.active=true;
			flight_planner->CreateDeployPoint(squad1,g1.id);
		}
		else if(!g3.active) {
			g3.id=squad1; g3.count=6; g3.active=true;
			flight_planner->CreateDeployPoint(squad1,g1.id);
		}
		else if(!g4.active) {
			g4.id=squad1; g4.count=6; g4.active=true;
			flight_planner->CreateDeployPoint(squad1,g1.id);
		}
	}
	
}

void
CarrierAI::CarrierCommand()
{
	if(!g1.active){
		Deployment();
	}
	ListIter<Element> iter = sim->GetElements();
	while (++iter) {
		Element*	elem = iter.value();
		Ship*		unit = elem->GetShip(1);


			//**Assign guards
		if(elem && elem->GetIFF() == ship->GetIFF() && !elem->IsGuarded() && elem->GetShipClass() > Ship::FRIGATE && !unit->GetHangar()) {			//**filter guard candidate
			int  size= 4;
			if(unit->Class() > Ship::DESTROYER) 
				size = 6;

					//**Assign guard to leader of group
			if(elem->GetCombatGroup() && elem->GetCombatUnit()->IsLeader()) {
				Guard(elem, size);			
			}
			else if(elem->GetCommander() && !elem->GetCommander()->IsGuarded()) {
				Guard(elem->GetCommander(), size);
			}
			else if(unit->GetWard() && !unit->GetWard()->GetElement()->IsGuarded()) {
				Guard(unit->GetWard()->GetElement(), size);
			}	  			
		}

		if(CheckHostileElements(elem)) {
			if(elem->GetShip(1)->GetHangar() && !g1.active || !g2.active) {
				Deployment();
			}
			CreateStrike(elem);
			//hold_time = (int) Game::GameTime() + 30000;	
		}
	}
	hold_time = (int) Game::GameTime() + 15000;
}

void
CarrierAI::Balance()
{
	double		blue	= 0;
	double		bluebig	= 0;
	double		red		= 0;
	double		redbig	= 0;

	ListIter<Contact> iter = ship->ContactList();

	while ( ++iter) {
		Contact* c = iter.value();
		Ship* unit = c->GetShip();

		if(unit && ship->IsHostileTo(unit)){
			if(unit->Class() > Ship::FREIGHTER)
			redbig += unit->AIValue();

			red +=	unit->AIValue();
		}
		else if(unit && unit->GetIFF() == ship->GetIFF()) {
			    if(unit->Class() > Ship::FREIGHTER)
				bluebig += unit->AIValue();

				blue +=	unit->AIValue();
		}
	}
	superior_small = (blue > red) ? true : false;
	superior_big = (bluebig > redbig) ? true : false;

}