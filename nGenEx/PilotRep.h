
/* Pilot Class model representation

*/


#include "solid.h"

class PilotRep : public Solid

{
public:
	PilotRep();
	virtual ~PilotRep();
	

	virtual void	Render(Video* video,DWORD flags);


protected:


};

