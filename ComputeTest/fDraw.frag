#version 440 core

in vec2 texCoord;

uniform sampler2D tex;

out vec4 fragColor;

void main() {
	fragColor = sqrt(texture(tex,texCoord));
}