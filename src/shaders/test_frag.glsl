#version 410 core

in float Height;
in vec3 debugColor;

out vec4 FragColor;

uniform int u_showDebugLOD;

void main()
{
    float h = (Height + 16)/64.0f;

    if (bool(u_showDebugLOD))
    {
        FragColor = vec4(debugColor, 1.0f);
    }
    else
    {
        FragColor = vec4(h, h, h, 1.0);
    }
}