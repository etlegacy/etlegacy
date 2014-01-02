/* vertexAnimation_vp.glsl - interpolates .md3/.mdc vertex animations */

vec4 InterpolatePosition(vec4 from, vec4 to, float frac)
{
	return from + ((to - from) * frac);
}

vec3 InterpolateNormal(vec3 from, vec3 to, float frac)
{
	return normalize(from + ((to - from) * frac));
}

void VertexAnimation_P_N(vec4 fromPosition, vec4 toPosition,
                         vec3 fromNormal, vec3 toNormal,
                         float frac,
                         inout vec4 position, inout vec3 normal)
{
	position = InterpolatePosition(fromPosition, toPosition, frac);
	normal   = InterpolateNormal(fromNormal, toNormal, frac);
}

void VertexAnimation_P_TBN(vec4 fromPosition, vec4 toPosition,
                           vec3 fromTangent, vec3 toTangent,
                           vec3 fromBinormal, vec3 toBinormal,
                           vec3 fromNormal, vec3 toNormal,
                           float frac,
                           inout vec4 position, inout vec3 tangent, inout vec3 binormal, inout vec3 normal)
{
	position = InterpolatePosition(fromPosition, toPosition, frac);

	tangent  = InterpolateNormal(fromTangent, toTangent, frac);
	binormal = InterpolateNormal(fromBinormal, toBinormal, frac);
	normal   = InterpolateNormal(fromNormal, toNormal, frac);
}
