#version 330

// Eingabevariablen vom C++ Code (Raylib)
in
vec2 fragTexCoord;
in
vec4 fragColor;

// Universelle Variablen (Uniforms)
uniform vec2 resolution;
uniform float time;

// Ausgabe an den Monitor
out
vec4 finalColor;

void main()
{
    // 1. Pixelkoordinaten normalisieren (-1.0 bis 1.0, zentriert)
    vec2 uv = (gl_FragCoord.xy * 2.0 - resolution.xy) / resolution.y;

    // 2. Mathematischer Abstand zum Zentrum (Schwarzes Loch)
    float distance = length(uv);

    // 3. Definition des Ereignishorizonts (Der pechschwarze Kern)
    float eventHorizon = 0.25;

    if (distance < eventHorizon)
    {
        // Die absolute Singularität schluckt jedes Photon
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
    else
    {
        // 4. Prozedurale Akkretionsscheibe mit ultra-hellem Additive-Gleißen
        // Wir berechnen eine rotierende Dichtewelle auf der GPU
        float angle = atan(uv.y, uv.x);
        float speed = time * 3.0;
        float wave = sin(angle * 5.0 - speed + (1.0 / distance * 10.0));

        // Intensives, hochenergetisches Plasma-Farb-Mapping (CIG-Orange/Gold)
        float glow = 0.03 / (distance - eventHorizon);
        vec3 diskColor = vec3(1.0, 0.5, 0.1) * glow;
        
        // Mische die rotierenden Gasströme hinein
        diskColor += vec3(0.9, 0.8, 0.2) * (wave * 0.15 * glow);

        // Nach außen hin weich ausblenden lassen
        float alpha = smoothstep(0.8, eventHorizon, distance);

        finalColor = vec4(diskColor * alpha, 1.0);
    }
}