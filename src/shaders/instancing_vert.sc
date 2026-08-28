// ============================================================================
// [Core/Graphics] HARDWARE INSTANCING VERTEX PIPELINE
// Description: Executes per-instance model-view transformations alongside 
//              screen-space gravitational lensing displacement for high-density 
//              topological manifolds.
// Standard: GLSL / BGFX Shader Model
// ============================================================================
$input a_position, a_normal, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_color0, v_normal

#include "bgfx_shader.sh"

void main() {
    mat4 modelMatrix = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
    vec4 worldPos = mul(modelMatrix, vec4(a_position, 1.0));
    
    vec3 instanceCenter = i_data3.xyz; 
    vec3 bhPos = vec3(0.0, 0.0, 0.0);  
    float rs = 0.6f;                    
    
    vec3 camPos = mul(u_invView, vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    
    float distCamToBH = length(bhPos - camPos);
    float distCamToAst = length(instanceCenter - camPos);
    
    vec4 clipBH = mul(u_viewProj, vec4(bhPos, 1.0));
    
    vec2 offsetNDC = vec2(0.0f);

    // Evaluate gravitational lensing displacement exclusively when the primary 
    // attractor resides within the forward camera frustum hemisphere (clip.w > 0).
    if (clipBH.w > 0.0f) {
        vec2 ndcBH = clipBH.xy / clipBH.w;
        
        vec4 clipCenterAst = mul(u_viewProj, vec4(instanceCenter, 1.0));
        vec2 ndcCenterAst = clipCenterAst.xy / clipCenterAst.w;
        
        vec2 deltaNDC = ndcCenterAst - ndcBH;
        float aspect = 1.777f; 
        deltaNDC.x *= aspect; 
        
        float rNDC = length(deltaNDC) + 0.0001f; 
        float apparentRs = (rs * 1.5f) / (distCamToBH + 0.001f); 
        
        // Gravitational Deflection Approximation
        // Formula: alpha = (r_app^2) / (r_ndc^2 + epsilon)
        float deflection = (apparentRs * apparentRs) / (rNDC * rNDC + 0.1f);

        // Attenuate displacement to prioritize occluded geometry along line of sight
        float depthFade = (distCamToAst > distCamToBH - rs) ? 1.0f : 0.0f;
        
        offsetNDC = normalize(deltaNDC) * deflection * 2.0f * depthFade;
        offsetNDC.x /= aspect;
    }
    
    gl_Position = mul(u_viewProj, worldPos);
    if (clipBH.w > 0.0f) {
        gl_Position.xy += offsetNDC * gl_Position.w;
    }
    
    vec3 worldNormal = mul(modelMatrix, vec4(a_normal, 0.0)).xyz;
    v_normal = normalize(worldNormal);
    
    vec4 agentColor = i_data4;
    
    // Anti-NaN Trap: Inverting the greater-than operator safely catches NaN 
    // memory corruption because (NaN > 0.05f) evaluates to false. !(false) = true.
    if (!(length(agentColor.rgb) > 0.05f)) {
        agentColor = vec4(0.0f, 0.85f, 1.0f, 1.0f); 
    }
    v_color0 = agentColor;
}