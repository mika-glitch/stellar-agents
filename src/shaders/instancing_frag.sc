$input v_color0, v_normal

#include "bgfx_shader.sh"

void main() {
    // Static directional light source approximation
    vec3 lightDir = normalize(vec3(0.8, 1.2, -1.0));
    
    // Compute diffuse lighting coefficient
    float nDotL = max(dot(v_normal, lightDir), 0.15); 
    
    // Modulate base color with lighting
    vec3 albedo = v_color0.rgb;
    vec3 finalIrradiance = albedo * nDotL;
    
    gl_FragColor = vec4(finalIrradiance, v_color0.a);
}