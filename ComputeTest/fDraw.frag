#version 450 core

in vec2 texCoord;

uniform sampler2D tex;

out vec4 fragColor;

void main() {
	fragColor = texture(tex,texCoord);
}