//
//      Kirill Kobelev, Moscow-Paris-Sammamish.
//  -------------------------------------------------
//   All rights reserved. Commercial use without written permission prohibited.
//

#define    STRICT
#include  <stdio.h>
#include  <math.h>
#include  <windows.h>
#include  <assert.h>

#pragma hdrstop

#include  "ScreenItems/GeometryHelpers.H"

// ------------------------------------------------------------------------
//   ==================  TGeometryHelper  =======================
// ------------------------------------------------------------------------

void TGeometryHelper::ShiftPoint(POINT &pnt_start, POINT pnt_dest, double distance)
{
	if (pnt_start.x == pnt_dest.x && pnt_start.y == pnt_dest.y)
	{
		// The starting point cannot be shifted.
		return;
	}

	// Pick up the intermediate point.
	pnt_start.x = (long)(pnt_start.x + (pnt_dest.x-pnt_start.x)*distance);
	pnt_start.y = (long)(pnt_start.y + (pnt_dest.y-pnt_start.y)*distance);
}

void TGeometryHelper::ShiftPointAbsolute(POINT &pnt_start, POINT pnt_dest, double distance)
{
	if (pnt_start.x == pnt_dest.x && pnt_start.y == pnt_dest.y)
	{
		// The point cannot be shifted.
		return;
	}

	double len = GetDistance(pnt_start.x, pnt_start.y, pnt_dest.x, pnt_dest.y);
	pnt_start.x = (long)(pnt_start.x + (pnt_dest.x-pnt_start.x)*distance/len);
	pnt_start.y = (long)(pnt_start.y + (pnt_dest.y-pnt_start.y)*distance/len);
}


