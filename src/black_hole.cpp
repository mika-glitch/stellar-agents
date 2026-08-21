#include "black_hole.h"
#include "raymath.h"
#include <cmath>

Shader accretionShader;
int timeLoc;
int resolutionLoc;
int camPosLoc;
int camTargetLoc;

// MASTER SHADER WITH DIRECT MATRIX COUPLING 
const char* shaderSource = R"(
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform float time;
uniform vec2 resolution;

uniform vec3 camPos;
uniform vec3 camTarget;

// High-frequency 3D hash for perfectly distributed pinpoint stars
float starHash3D(vec3 p) {
    float dotProduct = dot(p, vec3(127.1, 311.7, 74.7));
    return fract(sin(dotProduct) * 43758.5453123);
}

float hash2d(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise2d(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash2d(i + vec2(0.0, 0.0)), hash2d(i + vec2(1.0, 0.0)), u.x),
               mix(hash2d(i + vec2(0.0, 1.0)), hash2d(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    vec2 shift = vec2(100.0);
    mat2 rot = mat2(cos(0.6), sin(0.6), -sin(0.6), cos(0.6));
    for (int i = 0; i < 4; ++i) {
        v += a * noise2d(p);
        p = rot * p * 2.5 + shift;
        a *= 0.46;
    }
    return v;
}

void main() {
    vec2 uv = (gl_FragCoord.xy * 2.0 - resolution.xy) / resolution.y;
    
    // 1. STABLE INTERNAL 3D CAMERA SYSTEM
    vec3 forward = normalize(camTarget - camPos);
    
    // We stabilize the up vector mathematically continuously!
    // This prevents the axes from flipping and completely eliminates the seam line.
    vec3 upProjected = vec3(0.0, 1.0, 0.0);
    if (abs(forward.y) > 0.99) {
        upProjected = vec3(0.0, 0.0, sign(forward.y));
    }
    
    vec3 right = normalize(cross(upProjected, forward));
    vec3 up = cross(forward, right);
    
    vec3 rayDir = normalize(forward - uv.x * right * 0.5 + uv.y * up * 0.5);
    vec3 rayPos = camPos;
    
    float mass = 0.75;          
    float eventHorizon = 0.95;  
    
    vec3 totalColor = vec3(0.0);
    bool hitSingularity = false;
    
    float steepness = abs(forward.y);
    float dynamicThickness = 0.25 + (steepness * 0.65);

    // 2. ADAPTIVE 3D RAYMARCHING
    for (int step = 0; step < 160; step++) {
        float r2 = dot(rayPos, rayPos);
        float r = sqrt(r2);
        
        if (r < eventHorizon) {
            hitSingularity = true;
            break;
        }
        
        vec3 gravityDir = -rayPos / r;
        float deflection = (3.0 * mass) / (r2 * r); 
        float stepSize = 0.04 + (r * 0.012); 
        
        rayDir = normalize(rayDir + gravityDir * deflection * stepSize * 0.4);
        rayPos += rayDir * stepSize; 
        
        // A) VOLUMETRIC GAS DISK SAMPLING
        if (abs(rayPos.y) < dynamicThickness) {
            float distToCenter = length(rayPos.xz);
            
            if (distToCenter > 1.05 && distToCenter < 60.0) {
                // Stetige Phasenberechnung verhindert Kanten im Rauschen
                float angle = atan(rayPos.z, rayPos.x);
                float speed = time * 0.25;
                float logRadius = log(distToCenter);
                
                float spiralSog = 4.0 * logRadius - speed;
                float centralVortex = angle * 4.0 + (14.0 / (distToCenter + 0.05)) - speed * 0.5;
                
                vec2 vortexUV = vec2(spiralSog, centralVortex);
                float gasStructure = pow(fbm(vortexUV * 1.3), 1.5) * 2.5;
                float glow = 1.2 / (distToCenter - eventHorizon + 0.03);
                
                vec3 gasColor = vec3(0.15, 0.45, 1.0) * glow * 0.7; 
                gasColor += vec3(0.5, 0.1, 0.9) * (gasStructure * glow * 1.5); 
                
                float coreGlow = smoothstep(3.5, 1.05, distToCenter);
                gasColor += vec3(1.0, 1.0, 1.0) * (coreGlow * glow * 2.8); 
                
                // Doppler-Effekt perfekt ausgerichtet auf das lokale Koordinatensystem
                if (rayPos.x < 0.0) gasColor *= 1.6; 
                else gasColor *= 0.55; 
                
                float edgeFade = smoothstep(60.0, 12.0, distToCenter);
                float angleDamping = mix(1.0, 0.35, steepness);
                
                float volumeFade = smoothstep(dynamicThickness, 0.0, abs(rayPos.y));
                totalColor += gasColor * 0.045 * 3.5 * edgeFade * angleDamping * volumeFade;
            }
        }
        
        // B) RELATIVISTIC SPACETIME GRID SAMPLING
        float gridY = -3.2;
        if ((rayPos.y - gridY) * ((rayPos.y - rayDir.y * stepSize) - gridY) < 0.0) {
            float t = (gridY - (rayPos.y - rayDir.y * stepSize)) / rayDir.y;
            vec3 gridIntersection = (rayPos - rayDir * stepSize) + rayDir * t;
            float distToGridCenter = length(gridIntersection.xz);
            
            if (distToGridCenter < 80.0) {
                vec2 gridCoord = fract(gridIntersection.xz * 0.8); 
                float lineThickness = 0.03; 
                
                if (gridCoord.x < lineThickness || gridCoord.y < lineThickness) {
                    float fadeWithDistance = smoothstep(80.0, 8.0, distToGridCenter);
                    totalColor += vec3(0.3, 0.45, 0.6) * fadeWithDistance * 0.18;
                }
            }
        }
    }
    
    // 2. STEP: RELATIVISTIC CHROMATIC LENSING OF BACKGROUND STARS AT THE END
    if (!hitSingularity) {
        vec3 finalStarDir = floor(rayDir * 500.0); 
        float starNoise = starHash3D(finalStarDir);
        
        if (starNoise > 0.9935) { 
            float starIntensity = smoothstep(0.9935, 1.0, starNoise);
            float twinkle = sin(time * 3.5 + starNoise * 1000.0) * 0.4 + 0.6;
            totalColor += vec3(starIntensity * twinkle * 2.2); 
        }
    }
    
    // 3. RENDERING-FINALE
    if (hitSingularity) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0); 
    } else {
        totalColor = totalColor / (totalColor + vec3(1.0));
        totalColor = pow(totalColor, vec3(0.85)); 
        finalColor = vec4(totalColor, 1.0); 
    }
}
)";

BlackHole::BlackHole(Vector3 pos, float m, float r) {
    position = pos;
    mass = m;
    eventHorizon = r;
    accretionShader = LoadShaderFromMemory(0, shaderSource);
    timeLoc = GetShaderLocation(accretionShader, "time");
    resolutionLoc = GetShaderLocation(accretionShader, "resolution");
    camPosLoc = GetShaderLocation(accretionShader, "camPos");
    camTargetLoc = GetShaderLocation(accretionShader, "camTarget");
}

void BlackHole::Update(float deltaTime) { (void)deltaTime; }

void BlackHole::Draw(Camera3D camera) const {
    float w = (float)GetScreenWidth();
    float h = (float)GetScreenHeight();
    Vector2 res = { w, h };
    float currentTime = GetTime();
    Vector3 cPos = camera.position;
    Vector3 cTarget = camera.target;

    SetShaderValue(accretionShader, timeLoc, &currentTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(accretionShader, resolutionLoc, &res, SHADER_UNIFORM_VEC2);
    SetShaderValue(accretionShader, camPosLoc, &cPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(accretionShader, camTargetLoc, &cTarget, SHADER_UNIFORM_VEC3);

    BeginBlendMode(BLEND_ALPHA);
    BeginShaderMode(accretionShader);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();
    EndBlendMode();
}

Vector3 BlackHole::CalculateGravity(Vector3 objectPos) const {
    Vector3 direction = {
        position.x - objectPos.x,
        position.y - objectPos.y,
        position.z - objectPos.z
    };
    float distanceSq = (direction.x * direction.x) + (direction.y * direction.y) + (direction.z * direction.z);
    float distance = std::sqrt(distanceSq);
    if (distance < 0.1f) return Vector3Zero();
    float accelerationMagnitude = mass / distanceSq;
    Vector3 normalizedDirection = { direction.x / distance, direction.y / distance, direction.z / distance };
    return Vector3Scale(normalizedDirection, accelerationMagnitude);
}

bool BlackHole::HasCrossedPointOfNoReturn(Vector3 objectPos) const {
    float distance = Vector3Distance(objectPos, position);
    return (distance <= eventHorizon);
}