

#include "MemDebug.h"
#include "ShipGraveyard.h"
#include "Sim.h"
#include "CameraDirector.h"
#include "Sprite.h"
#include "DataLoader.h"
#include "Bitmap.h"
#include "Game.h"
#include "Video.h"
#include "Graphic.h"
#include "Scene.h"
#include "ParseUtil.h"
#include "Random.h"
#include "Solid.h"
#include <thread>

static  Model*		debries[32];
static	Sprite*		dust;
static	Bitmap*		bit;
static	Bitmap		bitmap;


ShipGraveyard::ShipGraveyard(Ship* s,bool flydust, SimRegion* rgn)
:SimObject("Graveyard",SimObject::SIM_GRAVEYARD), Dship(s), loc(0,0,0), exec_time(0), init(false)
{
	sim = Sim::GetSim();
	loc = Dship->Location();
	MoveTo(loc);

	zone = rgn;

	if (flydust)
	puff = new(__FILE__,__LINE__) FlyDust(bit, loc, 1200, 2, 15, 3, 1.0f);	//FlyDust for render
	else puff=0;

	life=15;
	
}


ShipGraveyard::~ShipGraveyard(void)
{
	GRAPHIC_DESTROY(puff);
}

void
ShipGraveyard::Initialize()
{
	
			DataLoader* loader = DataLoader::GetLoader();
			loader->SetDataPath("Explosions/Tomb/");
			loader->LoadBitmap("Tombdust.pcx", bitmap, 2);	

			bit = new(__FILE__,__LINE__) Bitmap(bitmap);	//Bitmap

		ZeroMemory(debries, sizeof(debries));
		int n = 0;

		Model* bump = new(__FILE__,__LINE__) Model;
		if (bump) {
			bump->Load("Debris0A.mag", 2);
			bump->ScaleBy(1.5);
			debries[n++] = bump;
		}

		bump = new(__FILE__,__LINE__) Model;
		if (bump) {
			bump->Load("debri1.mag", 2);
			bump->ScaleBy(1.5);
			debries[n++] = bump;
		}

		bump = new(__FILE__,__LINE__) Model;
		if (bump) {
			bump->Load("debri2.mag", 2);
			bump->ScaleBy(1.5);
			debries[n++] = bump;
		}

		loader->SetDataPath(0);

}

void
ShipGraveyard::ExecFrame(double seconds)
{
	CameraDirector* camera = CameraDirector::GetInstance();
	cam_loc = camera->GetCamera()->Pos();

	life -= seconds;

	if(Dship && Dship->IsCold())
		loc = Dship->Location();

	if (camera && camera->GetCamera() && puff) {
		if (Point(cam_loc - loc).length() < 15e3 ) {	//15000				Target			Distance 1			Distance2
			puff->ExecFrame(seconds);					//**				 Point<--------->Reset on/off<----->Execframe off
			
			if (Point(cam_loc - loc).length() < 4000 )	//4000
				puff->hidden = false;
			else puff->hidden = true;						
		}		
	} 
}

void
ShipGraveyard::Activate(Scene& scene)
{
	if(puff) 
		scene.AddGraphic(puff);

	active = true;
	
}

void
ShipGraveyard::Deactivate(Scene& scene)
{
	if(puff) 
		scene.DelGraphic(puff);

	active = false;
	
}

void
ShipGraveyard::CreateCloud(void)
{
	

	Point vel = Point(0,0,0);
	float scale = 0;
	if (Dship->Class() > Ship::LCA) {
		scale = 6;
		stuff =new(__FILE__,__LINE__) FlyDust(3, loc, Dship, zone, 120, 1, 20, 600); //Debris
	}
	else scale = 2;

	exp = sim->CreateExplosion(loc,vel,23,scale,scale,Dship->GetRegion());	//Big Dust cloud

}

void
ShipGraveyard::CreateDebris(Point loc,Ship* ship, int secs)
{
	//stuff =new(__FILE__,__LINE__) FlyDust(3, loc, ship, ship->GetRegion(), 220, 1, 1, secs);

	int		np		= 1;
	int		models	= 3;
	double	RADIUS	= 220;
	position = new(__FILE__,__LINE__) Point[np];

	for(int i = 0; i < np; ++i) {
		for(int m = 0; m < models; ++m) {
		position[i]	 = Point(Random(-RADIUS,RADIUS) , 
							 Random(-RADIUS,RADIUS) , 
							 Random(-RADIUS,RADIUS) );

		Point vel = (position[i]+loc) - loc;
		vel.Normalize();
		vel += position[i];

		Debris* deb = sim->CreateDebris(position[i]+loc, vel*2 + ship->Velocity(), debries[m], 10, zone);
		deb->SetDrag(0.08);
		deb->SetLife(secs);
		}
	}

}

void
ShipGraveyard::CreateFlybyDust(void)
{			
}

//------------------------------------------------------------------------------------------------------------------------------------


FlyDust::FlyDust(Bitmap* bmp, const Point& pos, double rad, float multi, int number, float scale, double shade)		//Sprite constructor
:shade(shade), np(number), RADIUS(rad), multiplier(multi), last_delta(0),object(false),  hidden(true)
{
	cam		= CameraDirector::GetInstance();
	cam_pos = cam->GetCamera()->Pos();

//	if(pos)	origin	= pos;

	trans       = true;
	luminous    = false;
	shadow      = false;

	radius = 500.0f;

	obj_loc	= new(__FILE__,__LINE__) Point[np];
	intensity	= new(__FILE__,__LINE__) double[np];
	last_delta	= new(__FILE__,__LINE__) double[np];

	for(int i = 0; i < np; ++i) {
		obj_loc[i]		= Point(Random(-RADIUS,RADIUS) + cam_pos.x, 
								Random(-RADIUS,RADIUS) + cam_pos.y, 
								Random(-RADIUS,RADIUS) + cam_pos.z);
		intensity[i]	= 0;
	}
	dust = new(__FILE__,__LINE__) Sprite(bmp, 1, 0);
	if(dust) {
	dust->Scale(scale);
	dust->SetBlendMode(2);
	dust->SetLuminous(false);
	}
}
//+-----------------------------------------------------------------------------------------------------+

FlyDust::FlyDust(int models, const Point& pos, Ship* ship, SimRegion* zone, double rad, float multi, int number, int secs)		//3d model constructor
: np(number), RADIUS(rad), multiplier(multi),object(true), hidden(false)								
{
	cam		= CameraDirector::GetInstance();
	cam_pos = cam->GetCamera()->Pos();

	MoveTo(pos);

	obj_loc		= new(__FILE__,__LINE__) Point[np];							// This thing should really go directly into Shipgaveyard:create debris. Its pointless to use
	last_delta	= new(__FILE__,__LINE__) double[np];						// flyDust to make this since vanilla debris class is who is handling the stuff.

	for(int i = 0; i < np; ++i) {
		for(int m = 0; m < models; ++m) {
		obj_loc[i]	 = Point(Random(-RADIUS,RADIUS) , 
							 Random(-RADIUS,RADIUS) , 
							 Random(-RADIUS,RADIUS) );

		Point vel = (obj_loc[i]+pos) - pos;
		vel.Normalize();
		vel += obj_loc[i];

		Debris* deb = sim->CreateDebris(obj_loc[i]+pos, vel*2 + ship->Velocity(), debries[m], 10, zone);
		deb->SetDrag(0.08);
		deb->SetLife(secs);
		}
	}
}

FlyDust::~FlyDust()
{
	delete dust;
	delete [] intensity;
	delete [] obj_loc;		
	delete [] last_delta;
}

void
FlyDust::Reset(int i)
{
	Point vel;
	if(cam->GetShip())
		 vel = cam->GetShip()->Velocity();	

	if(!object) {
		obj_loc[i] = Point(Random(-RADIUS,RADIUS) + cam_pos.x + vel.x * multiplier, 
							Random(-RADIUS,RADIUS) + cam_pos.y + vel.y * multiplier, 
							Random(-RADIUS,RADIUS) + cam_pos.z + vel.z * multiplier);
		intensity[i]	= 0;
	}
}

void
FlyDust::ExecFrame(double seconds)
{
	cam_pos = cam->GetCamera()->Pos();
	MoveTo(cam_pos);

	for(int i=0; i < np; ++i) {
		if(!object) {

		if (hidden) {
			intensity[i] -= seconds;
			if (intensity[i] < 0) intensity[i] = 0;
			continue;
		}

		double delta = Point(obj_loc[i] - cam_pos).length();
		if(delta > RADIUS * multiplier || intensity[i] == 0) 
			Reset(i);

			if(last_delta[i] > delta) {
				intensity[i] += (1/delta) * 650 * seconds * 2;
				if(intensity[i] > shade)
					intensity[i] = shade;
			}
			else if(delta > RADIUS && last_delta[i] < delta) {
				intensity[i] -= seconds;			
			}
		
		if (intensity[i] <= 0)
			intensity[i] = 0;

		last_delta[i] = delta;
		}

	/*	if(object) {
			double delta = Point(obj_loc[i] - cam_pos).length();
			if(delta > RADIUS * multiplier && !hidden)
				Reset(i);
			
			last_delta[i] = delta;
		} */
	}
}

void
FlyDust::Render(Video* video, DWORD flags)
{
	if  (!video || !dust || object)
	return;

	MoveTo(cam_pos);

	for(int i=0; i < np; ++i) {
		if(intensity[i] <= 0)
			continue;

		Point dloc;
		dloc = obj_loc[i] - Location();
		
		dust->MoveTo(dloc);
		dust->SetShade(intensity[i]);
		dust->Render(video, flags);
	}	
}