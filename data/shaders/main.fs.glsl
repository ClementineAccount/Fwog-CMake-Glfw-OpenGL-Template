#version 460 core

layout(binding = 0) uniform sampler2D s_baseColor;

layout(location = 0) in vec2 v_uv;

void main()
{
  o_color = texture(s_baseColor, v_uv).rgba;
}