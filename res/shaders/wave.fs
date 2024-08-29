#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

uniform float time_since_start;

// NOTE: Add here your custom variables
const float renderWidth = 800;
const float renderHeight = 800;

float freqX = 25.0;
float freqY = 25.0;
float ampX = 5.0;
float ampY = 5.0;
float speedX = 8.0;
float speedY = 8.0;

void main() {
    float pixelWidth = 1.0 / renderWidth;
    float pixelHeight = 1.0 / renderHeight;
    float aspect = pixelHeight / pixelWidth;
    float boxLeft = 0.0;
    float boxTop = 0.0;

    vec2 p = fragTexCoord;
    p.x += cos((fragTexCoord.y - boxTop) * freqX / ( pixelWidth * 750.0) + (time_since_start * speedX)) * ampX * pixelWidth;
    p.y += sin((fragTexCoord.x - boxLeft) * freqY * aspect / ( pixelHeight * 750.0) + (time_since_start * speedY)) * ampY * pixelHeight;

    finalColor = texture(texture0, p)*colDiffuse*fragColor;
}
