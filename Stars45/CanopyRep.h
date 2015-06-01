


#include "solid.h"
class CanopyRep :	public Solid
{
public:
	CanopyRep();
	CanopyRep(const CanopyRep& rhs);
	virtual ~CanopyRep();

	virtual void		Render(Video* video,DWORD flags);

	virtual Model*			GetRep()			{ return rep;}



protected:

	Model*		rep;

};

