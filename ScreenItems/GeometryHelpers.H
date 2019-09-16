//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#ifndef	ScreenItems_GeometryHelpers_H
#define	ScreenItems_GeometryHelpers_H

#ifndef   _INC_MATH
#define    _USE_MATH_DEFINES		// This definition is needed to pick up the M_PI definition from math.h.
#include  <math.h>
#endif

struct TPoint
{
	TPoint() { x = y = 0; }
	TPoint(POINT &pos) { x = pos.x; y = pos.y; }

	long		X() const { return((long)floor(x + 0.5 )); }
	long		Y() const { return((long)floor(y + 0.5 )); }
				// Rounding is a big issue here.

	void SetPos(long prm_x, long prm_y) { x = prm_x; y = prm_y; }

	void ShiftAbsolute(TPoint &dest, double distance)
	{
		double dist = Distance(*this, dest);
		if (dist < 1)
			return;

		x += (dest.x-x)*distance/dist;
		y += (dest.y-y)*distance/dist;
	}

	static double Distance(TPoint &p1, TPoint &p2) { double dx = p1.x-p2.x, dy = p1.y-p2.y;  return(sqrt(dx*dx+dy*dy)); }

public:

	double		x, y;
};

class TGeometryHelper
{
public:

	static double	GetDistance(long px1, long py1, long px2, long py2) { long dx = px1-px2; long dy = py1-py2; return(sqrt((double)(dx*dx+dy*dy))); }

	static void	ShiftPoint(POINT &pnt_start, POINT pnt_dest, double distance);
					// When value of the param distance is 0.0, the pnt_start is not moved. When value of the distance is 1.0,
					// the pnt_start becomes the pnt_dest.

	static void	ShiftPointAbsolute(POINT &pnt_start, POINT pnt_dest, double distance);
					// Param distance is expressed in pixels.
};

#endif	// ScreenItems_GeometryHelpers_H


