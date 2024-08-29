#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

// NOTE: Render size values must be passed from code
const float renderWidth = 800;
const float renderHeight = 800;
float offset = 0.0;
float scanline_brightness = 0.1;

void main()
{
    float frequency = renderHeight/1.0;

    float y_pos = (fragTexCoord.y + offset) * frequency;
    float wavePos = cos((fract(y_pos) - 0.5)*3.14);
    vec4 texelColor = texture(texture0, fragTexCoord);

    finalColor = mix(vec4(scanline_brightness, scanline_brightness, scanline_brightness, 0.0), texelColor, wavePos);
}