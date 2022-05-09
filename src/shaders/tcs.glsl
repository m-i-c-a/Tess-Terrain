// Tessellation Control Shader
#version 450 core

layout (vertices = 4) out;

in vec2 tex_coord[];

out vec2 texture_coord[];

uniform mat4 u_viewMatrix;

uniform int u_minTessLevel;
uniform int u_maxTessLevel;
uniform float u_minRange;
uniform float u_maxRange;

void main()
{
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    texture_coord[gl_InvocationID] = tex_coord[gl_InvocationID];

    if (gl_InvocationID == 0)
    {
        vec4 eyeSpacePos00 = u_viewMatrix * gl_in[0].gl_Position;
        vec4 eyeSpacePos01 = u_viewMatrix * gl_in[1].gl_Position;
        vec4 eyeSpacePos10 = u_viewMatrix * gl_in[2].gl_Position;
        vec4 eyeSpacePos11 = u_viewMatrix * gl_in[3].gl_Position;

        // "distance" from camera scaled between 0 and 1
        float distance00 = clamp( (abs(eyeSpacePos00.z) - u_minRange) / (u_maxRange-u_minRange), 0.0, 1.0 );
        float distance01 = clamp( (abs(eyeSpacePos01.z) - u_minRange) / (u_maxRange-u_minRange), 0.0, 1.0 );
        float distance10 = clamp( (abs(eyeSpacePos10.z) - u_minRange) / (u_maxRange-u_minRange), 0.0, 1.0 );
        float distance11 = clamp( (abs(eyeSpacePos11.z) - u_minRange) / (u_maxRange-u_minRange), 0.0, 1.0 );

        float tessLevel0 = mix( u_maxTessLevel, u_minTessLevel, min(distance10, distance00) );
        float tessLevel1 = mix( u_maxTessLevel, u_minTessLevel, min(distance00, distance01) );
        float tessLevel2 = mix( u_maxTessLevel, u_minTessLevel, min(distance01, distance11) );
        float tessLevel3 = mix( u_maxTessLevel, u_minTessLevel, min(distance11, distance10) );

        gl_TessLevelOuter[0] = tessLevel0;
        gl_TessLevelOuter[1] = tessLevel1;
        gl_TessLevelOuter[2] = tessLevel2;
        gl_TessLevelOuter[3] = tessLevel3;

        gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
        gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
    }
}