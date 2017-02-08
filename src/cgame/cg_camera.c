/**
 * @file cg_camera.c
 * @author jere
 * @date 21.9.2015
 */

#include "cg_local.h"

/*
 * @brief CG_CalcBezierPoint
 * @param[in] start
 * @param[in] ctrl1
 * @param[in] ctrl2
 * @param[in] end
 * @param[in] offset
 * @param[out] out
 *
 * @note Unused
void CG_CalcBezierPoint(vec3_t start, vec3_t ctrl1, vec3_t ctrl2, vec3_t end, float offset, vec3_t out)
{
	float var1, var2, var3;

	// Below is the equation for a 4 control point bezier curve:
	// B(t) = P1 * ( 1 - t )^3 + P2 * 3 * t * ( 1 - t )^2 + P3 * 3 * t^2 * ( 1 - t ) + P4 * t^3

	var1   = 1 - offset;
	var2   = var1 * var1 * var1;
	var3   = offset * offset * offset;
	out[0] = var2 * start[0] + 3 * offset * var1 * var1 * ctrl1[0] + 3 * offset * offset * var1 * ctrl2[0] + var3 * end[0];
	out[1] = var2 * start[1] + 3 * offset * var1 * var1 * ctrl1[1] + 3 * offset * offset * var1 * ctrl2[1] + var3 * end[1];
	out[2] = var2 * start[2] + 3 * offset * var1 * var1 * ctrl1[2] + 3 * offset * offset * var1 * ctrl2[2] + var3 * end[2];
}
*/
