#pragma once
#include "system.h"
#include "Geometry.h"
#include "Types.h"
#include "Polygon.h"

class CanopyRep;


class Pilot :	public System
{
public:

	Pilot();
	Pilot(const Pilot& rhs);
	virtual ~Pilot(void);

	virtual void	ExecFrame(double seconds);

	virtual bool	Alive()		{ return alive;	}
	virtual bool	CheckAlive();
	virtual bool	Ejected()	{ return ejected; }

    virtual void	Define();
	virtual void	Disabled();
	virtual void	Eject();

	const char*		GetName()		{ return name;	}
	const char*		GetSurname()	{ return surname;}
	virtual int		GetGender()		{ return gender;}		//** 1= male,  2= female.


protected:

 const char*	name;
 const char*	surname;
	int		gender;
	bool	alive;
	bool	ejected;

	int		FM;
	int		side;
	bool	init;

};

