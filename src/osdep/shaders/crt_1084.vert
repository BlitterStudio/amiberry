#version 450

// Fullscreen triangle — no vertex buffer needed.
// gl_VertexIndex 0,1,2 generates a triangle covering [-1,3]x[-1,3],
// clipped by the GPU to the [-1,1] viewport.

layout(location = 0) out vec2 outUV;

void main()
{
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 uvs[3] = vec2[](
        vec2(0.0, 0.0),
        vec2(2.0, 0.0),
        vec2(0.0, 2.0)
    );
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    outUV = uvs[gl_VertexIndex];
}
