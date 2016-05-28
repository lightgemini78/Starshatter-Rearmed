
/* 
Ship Graveyard class
------------------------
	Creates persistent dead ship debries area.
*/

#ifndef ShipGraveyard_h
#define ShipGraveyard_h

#include "Ship.h"
#include "Camera.h"
#include "CameraDirector.h"
#include "Debris.h"
#include "Bitmap.h"
#include "Explosion.h"

class Sprite;
class Graphic;
class ShipGraveyard;
class FlyDust;


class ShipGraveyard : public SimObject
{
	friend class FlyDust;

public:
	ShipGraveyard(Ship* ship,bool flydust, SimRegion* rgn);
    virtual~ShipGraveyard();

	virtual void		ExecFrame(double seconds);
			void		Setlife(double secs) { life = secs;}

	static void			Initialize();

	virtual void		Activate(Scene& scene);
	virtual void		Deactivate(Scene& scene);
	virtual Point		GetLoc()			{ return loc;}
	virtual void		CreateCloud(void);
	virtual void		CreateDebris(Point loc,Ship* ship, int secs);
	virtual void		CreateFlybyDust(void);


		Point		loc;

protected:

	Ship*		Dship;
	Sim*		sim;
	FlyDust*	puff;
	FlyDust*	stuff;
	Explosion*	exp;
	SimRegion*  zone;

	Point		cam_loc;
	Point*		position;

	int		exec_time;
	bool	init;

};

class FlyDust :	public Graphic
{
	friend class ShipGraveyard;

public:

	FlyDust(Bitmap* bmp, const Point& pos, double rad, float multi, int number, float scale, double shade);
	FlyDust::FlyDust(int models, const Point& pos, Ship* ship, SimRegion* zone, double rad, float multi, int number, int secs);
	virtual ~FlyDust();

	virtual void	ExecFrame(double seconds);
	virtual void	Reset(int i);
	virtual void    Render(Video* video, DWORD flags);

protected:

	Sprite* dust;
	Point*	obj_loc;
	Point	cam_pos;
	Point	origin;

	CameraDirector*	cam;
	Sim*	sim;

	int		np;
	float	multiplier;
	double*	last_delta;
	double	RADIUS;
	double*	intensity;
	double	shade;
	bool	object;
	bool	hidden;



};

#endif ShipGraveyard_h

