#version 410 core

layout(vertices=4) out;

in vec2 TexCoord[];
out vec2 TextureCoord[];
out vec3 lodColor[];

uniform mat4 u_viewMatrix;
uniform int u_minTessLevel;
uniform int u_maxTessLevel;
uniform float u_minRange;
uniform float u_maxRange;

const float transition_range = 0.33f;
const int num_lod_ranges = 4;
const float LODRanges[4] = float[4](200.0f, 400.0f, 800.0f, 1000.0f);

vec3 selectLOD(float d)
{
    if (d < LODRanges[0])
    {
        float weight = clamp(abs(d) / LODRanges[0], 0.0, 1.0 );
        float tess_level = u_maxTessLevel;
        float debugColor = 1.0f;

        // if (weight <= transition_range)
        // {
        //     float next_tess_level = floor(float(u_maxTessLevel) / float(num_lod_ranges) * 3);
        //     float inner_weight = (transition_range - weight) / (transition_range);
        //     tess_level = mix(tess_level, next_tess_level, inner_weight);
        //     debugColor = mix(1.0, 0.75f, inner_weight);
        // }

        return vec3(weight, tess_level, debugColor);
    }
    else if (d < LODRanges[1])
    {
        float weight = clamp((LODRanges[1] - abs(d)) / (LODRanges[1] - LODRanges[0]), 0.0, 1.0 );
        float tess_level = u_maxTessLevel / num_lod_ranges * 3;
        float debugColor = 0.75f;

        // if (weight <= transition_range)
        // {
        //     float next_tess_level = floor(float(u_maxTessLevel) / float(num_lod_ranges) * 3);
        //     float inner_weight = (transition_range - weight) / (transition_range);
        //     tess_level = mix(tess_level, next_tess_level, inner_weight);
        //     debugColor = mix(0.75f, 0.5f, inner_weight);
        // }
        return vec3(weight, tess_level, debugColor);
    }
    else if (d < LODRanges[2])
    {
        float weight = clamp((LODRanges[2] - abs(d)) / (LODRanges[2] - LODRanges[1]), 0.0, 1.0 );
        float tess_level = u_maxTessLevel / num_lod_ranges * 2;
        float debugColor = 0.5f;

        // if (weight <= transition_range)
        // {
        //     float next_tess_level = floor(float(u_maxTessLevel) / float(num_lod_ranges) * 3);
        //     float inner_weight = (transition_range - weight) / (transition_range);
        //     tess_level = mix(tess_level, next_tess_level, inner_weight);
        //     debugColor = mix(0.5f, 0.25f, inner_weight);
        // }
        return vec3(weight, tess_level, debugColor);
    }
    else
    {
        float weight = clamp((LODRanges[3] - abs(d)) / (LODRanges[3] - LODRanges[2]), 0.0, 1.0 );
        float tess_level = u_maxTessLevel / num_lod_ranges;
        return vec3(weight, tess_level, 0.25);
    }
}

void main()
{
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    TextureCoord[gl_InvocationID] = TexCoord[gl_InvocationID];

    if(gl_InvocationID == 0)
    {
        const int MIN_TESS_LEVEL = 4;
        const int MAX_TESS_LEVEL = 64;
        const float MIN_DISTANCE = 20;
        const float MAX_DISTANCE = 800;

        vec4 eyeSpacePos00 = u_viewMatrix * gl_in[0].gl_Position;
        vec4 eyeSpacePos01 = u_viewMatrix * gl_in[1].gl_Position;
        vec4 eyeSpacePos10 = u_viewMatrix * gl_in[2].gl_Position;
        vec4 eyeSpacePos11 = u_viewMatrix * gl_in[3].gl_Position;

        vec4 patch_center = (eyeSpacePos00 + eyeSpacePos01 + eyeSpacePos10 + eyeSpacePos11) / 4.0f;
        float dist_to_center = length(patch_center);
        
        float tess_level = 1.0f;
        vec3 debugColor = vec3(0.0f);

        vec3 lod00 = selectLOD(length(eyeSpacePos00));
        vec3 lod01 = selectLOD(length(eyeSpacePos01));
        vec3 lod10 = selectLOD(length(eyeSpacePos10));
        vec3 lod11 = selectLOD(length(eyeSpacePos11));

        lodColor[gl_InvocationID] = vec3(float(lod00.z + lod01.z + lod10.z + lod11.z) / 4.0f);

        gl_TessLevelOuter[0] = max(lod10.y, lod00.y);
        gl_TessLevelOuter[1] = max(lod00.y, lod01.y);
        gl_TessLevelOuter[2] = max(lod01.y, lod11.y);
        gl_TessLevelOuter[3] = max(lod11.y, lod10.y);

        gl_TessLevelInner[0] = max(lod01.y, lod11.y);
        gl_TessLevelInner[1] = max(lod00.y, lod10.y);

        // "distance" from camera scaled between 0 and 1
        // float distance00 = clamp( (abs(eyeSpacePos00.z) - u_minRange) / (u_maxRange-u_minRange), 0.0, 1.0 );
        // float distance01 = clamp( (abs(eyeSpacePos01.z) - u_minRange) / (u_maxRange-u_minRange), 0.0, 1.0 );
        // float distance10 = clamp( (abs(eyeSpacePos10.z) - u_minRange) / (u_maxRange-u_minRange), 0.0, 1.0 );
        // float distance11 = clamp( (abs(eyeSpacePos11.z) - u_minRange) / (u_maxRange-u_minRange), 0.0, 1.0 );

        // vec3 lod00 = selectLOD(eyeSpacePos00.z);
        // vec3 lod01 = selectLOD(eyeSpacePos01.z);
        // vec3 lod10 = selectLOD(eyeSpacePos10.z);
        // vec3 lod11 = selectLOD(eyeSpacePos11.z);


        // float tessLevel0 = mix( u_maxTessLevel, u_minTessLevel, min(distance10, distance00) );
        // float tessLevel1 = mix( u_maxTessLevel, u_minTessLevel, min(distance00, distance01) );
        // float tessLevel2 = mix( u_maxTessLevel, u_minTessLevel, min(distance01, distance11) );
        // float tessLevel3 = mix( u_maxTessLevel, u_minTessLevel, min(distance11, distance10) );

        // gl_TessLevelOuter[0] = tessLevel0;
        // gl_TessLevelOuter[1] = tessLevel1;
        // gl_TessLevelOuter[2] = tessLevel2;
        // gl_TessLevelOuter[3] = tessLevel3;

        // gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
        // gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
    }
}