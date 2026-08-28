$input v_color0, v_normal

#include "bgfx_shader.sh"

void main() {
    // Statische Lichtquelle
    vec3 lightDir = normalize(vec3(0.8, 1.2, -1.0));
    
    // Diffuses Licht mit angehobenem Ambient-Wert (0.4 statt 0.15) für weichere, hellere Schatten
    float nDotL = max(dot(v_normal, lightDir), 0.4); 
    
    vec3 albedo = v_color0.rgb;

    // Fallback-Sicherung: Wenn exakt Schwarz ankommt, erzwingen wir leuchtendes Neon-Cyan.
    if (length(albedo) < 0.05) {
        albedo = vec3(0.0, 0.8, 1.0);
    }
    
    // Emissive Boost: Wir multiplizieren das Ergebnis mit 3.0, damit die Agenten 
    // vor dem hellen Hintergrund des Schwarzen Lochs als echte Lichtquellen herausstechen.
    vec3 finalIrradiance = albedo * nDotL * 3.0;
    
    gl_FragColor = vec4(finalIrradiance, 1.0);
}