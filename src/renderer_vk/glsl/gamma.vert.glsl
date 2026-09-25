#version 450

// Full-screen triangle for the final gamma pass (no vertex
// buffer; positions/UVs derived from gl_VertexIndex 0..2). UV (0,0) is the
// top-left of the offscreen scene image, matching Vulkan's top-left
// framebuffer origin, so no flip is needed.

layout(location = 0) out vec2 fragUV;

void main()
{
	fragUV      = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(fragUV * 2.0 - 1.0, 0.0, 1.0);
}
