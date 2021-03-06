// // tessellation evaluation shader
// #version 450 core

// layout (quads, fractional_odd_spacing, ccw) in;

// in vec2 texture_coord[];

// out float height;

// uniform sampler2D u_heightMap;

// uniform mat4 u_projMatrix;
// uniform mat4 u_viewMatrix;
// uniform float u_heightScale;

// void main()
// {

//     // get patch coordinate
//     float u = gl_TessCoord.x;
//     float v = gl_TessCoord.y;

//     // ----------------------------------------------------------------------
//     // retrieve control point texture coordinates
//     vec2 t00 = texture_coord[0];
//     vec2 t01 = texture_coord[1];
//     vec2 t10 = texture_coord[2];
//     vec2 t11 = texture_coord[3];

//     // bilinearly interpolate texture coordinate across patch
//     vec2 t0 = (t01 - t00) * u + t00;
//     vec2 t1 = (t11 - t10) * u + t10;
//     vec2 texCoord = (t1 - t0) * v + t0;

//     // lookup texel at patch coordinate for height and scale + shift as desired
//     height = texture(u_heightMap, texCoord).y * 64.0 - 16.0;

//     // ----------------------------------------------------------------------
//     // retrieve control point position coordinates
//     vec4 p00 = gl_in[0].gl_Position;
//     vec4 p01 = gl_in[1].gl_Position;
//     vec4 p10 = gl_in[2].gl_Position;
//     vec4 p11 = gl_in[3].gl_Position;

//     // compute patch surface normal
//     vec4 uVec = p01 - p00;
//     vec4 vVec = p10 - p00;
//     vec4 normal = normalize( vec4(cross(vVec.xyz, uVec.xyz), 0) );

//     // bilinearly interpolate position coordinate across patch
//     vec4 p0 = (p01 - p00) * u + p00;
//     vec4 p1 = (p11 - p10) * u + p10;
//     vec4 p = (p1 - p0) * v + p0;

//     // displace point along normal
//     p += normal * height * u_heightScale;

//     // ----------------------------------------------------------------------
//     // output patch point position in clip space
//     gl_Position = u_projMatrix * u_viewMatrix * p;
// }

#version 450 core
layout(quads, fractional_odd_spacing, ccw) in;

uniform sampler2D heightMap;
uniform mat4 u_viewMatrix;
uniform mat4 u_projMatrix;

in vec2 texture_coord[];

out float Height;

void main()
{
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 t00 = texture_coord[0];
    vec2 t01 = texture_coord[1];
    vec2 t10 = texture_coord[2];
    vec2 t11 = texture_coord[3];

    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    vec2 texCoord = (t1 - t0) * v + t0;

    Height = texture(heightMap, texCoord).y * 64.0 - 16.0;

    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    vec4 uVec = p01 - p00;
    vec4 vVec = p10 - p00;
    vec4 normal = normalize( vec4(cross(vVec.xyz, uVec.xyz), 0) );

    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10;
    vec4 p = (p1 - p0) * v + p0 + normal * Height;

    gl_Position = u_projMatrix * u_viewMatrix * p;
}
