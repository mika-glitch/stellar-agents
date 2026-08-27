$input a_position, a_normal, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_color0, v_normal

#include "bgfx_shader.sh"

void main() {
    mat4 modelMatrix = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
    vec4 worldPos = mul(modelMatrix, vec4(a_position, 1.0));
    
    // 1. HARDCODED Parameter, um Uniform-Mismatches auszuschließen!
    vec3 instanceCenter = i_data3.xyz; // Translation der Instanz (Mittelpunkt)
    vec3 bhPos = vec3(0.0, 0.0, 0.0);  // Schwarzes Loch bei Null
    float rs = 0.6f;                    // Schwarzschild-Radius
    
    // 2. Kamera-Position absolut sicher aus BGFX Built-ins holen
    vec3 camPos = mul(u_invView, vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    
    float distCamToBH = length(bhPos - camPos);
    float distCamToAst = length(instanceCenter - camPos);
    
    // 3. Projektion in den 2D-Bildschirmraum
    vec4 clipBH = mul(u_viewProj, vec4(bhPos, 1.0));
    vec2 ndcBH = clipBH.xy / clipBH.w;
    
    vec4 clipCenterAst = mul(u_viewProj, vec4(instanceCenter, 1.0));
    vec2 ndcCenterAst = clipCenterAst.xy / clipCenterAst.w;
    
    vec2 deltaNDC = ndcCenterAst - ndcBH;
    
    // Stabiler Aspect-Ratio Fallback (16:9), um 0/0 Divisionen zu killen!
    float aspect = 1.777; 
    deltaNDC.x *= aspect; 
    
    float rNDC = length(deltaNDC) + 0.0001; 
    float apparentRs = (rs * 1.5) / (distCamToBH + 0.001); 
    
    // Gravitations-Lensing-Formel
    float deflection = (apparentRs * apparentRs) / (rNDC * rNDC + apparentRs * apparentRs);
    
    // WICHTIG: Lensing greift nur für Asteroiden, die HINTER dem Loch fliegen
    float depthFade = (distCamToAst > distCamToBH - rs) ? 1.0 : 0.0;
    
    // Faktor 4.0 -> Ziemlich stark, damit du den Effekt GANZ SICHER siehst!
    vec2 offsetNDC = normalize(deltaNDC) * deflection * apparentRs * 4.0 * depthFade;
    offsetNDC.x /= aspect;
    
    // 4. Vertex projizieren und den magischen 2D-Block-Shift anwenden
    gl_Position = mul(u_viewProj, worldPos);
    gl_Position.xy += offsetNDC * gl_Position.w;
    
    // 5. Normals & Farbe durchreichen
    vec3 worldNormal = mul(modelMatrix, vec4(a_normal, 0.0)).xyz;
    v_normal = normalize(worldNormal);
    
    v_color0 = i_data4;
}