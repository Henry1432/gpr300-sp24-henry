#version 450
layout (location = 0) in vec3 vPos;
uniform mat4 _ViewProjection;
uniform mat4 _Model;
void main()
{
    gl_Position = _ViewProjection * _Model * vec4(vPos.x, vPos.y, vPos.z, 1.0);
} 
