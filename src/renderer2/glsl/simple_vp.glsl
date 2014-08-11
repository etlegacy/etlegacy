attribute vec4 attr_Position;
uniform mat4   u_ModelViewProjectionMatrix;

void main()
{
	// transform vertex position into homogenous clip-space
	gl_Position = u_ModelViewProjectionMatrix * attr_Position;
}