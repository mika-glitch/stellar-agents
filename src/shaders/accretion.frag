/*
 * ============================================================================
 * [Core/Graphics] VULKAN/BGFX RENDERING PIPELINE
 * Shader Module: Relativistic Accretion Disk & Raymarching Fragment Pipeline
 * Description: Evaluates volumetric rendering, gravitational lensing, and
 *              Signed Distance Field (SDF) intersections.
 * ============================================================================
 */

$input v_color0, v_texcoord0

#include "bgfx_shader.sh"

uniform vec4 u_cameraPositionAndTime; // xyz: camPos, w: time
uniform vec4 u_resolutionAndMass;     // xy: resolution, z: mass, w: eventHorizon
uniform vec4 u_singularityPosition;   // xyz: holePos
uniform vec4 u_cameraTarget;          // xyz: camTarget
uniform vec4 u_object1;               // xyz: pos, w: radius (Primary Compact Mass Node)
uniform vec4 u_object2;               // xyz: pos, w: radius (Secondary Volumetric Gas Mass)
uniform vec4 u_object3;               // xyz: pos, w: radius (Trans-Orbital Portal Manifold)

// Generates pseudo-random scalar noise across 3D spatial domains.
float starHash3D(vec3 p) {
    float dotProduct = dot(p, vec3(127.1, 311.7, 74.7));
    return fract(sin(dotProduct) * 43758.5453123);
}

// Generates pseudo-random scalar values for 2D coordinate fields.
float hash2d(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// Computes quintic smoothstep interpolation for value noise gradient generation.
float smoothNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    float a = hash2d(i);
    float b = hash2d(i + vec2(1.0, 0.0));
    float c = hash2d(i + vec2(0.0, 1.0));
    float d = hash2d(i + vec2(1.0, 1.0));

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Evaluates multi-octave fractional Brownian motion (fBm) using rotated domain sampling.
float smoothFbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    mat2 rot = mat2(0.8, 0.6, -0.6, 0.8);
    for (int i = 0; i < 4; ++i) {
        v += a * smoothNoise(p);
        p = rot * p * 2.0 + vec2(15.2);
        a *= 0.5;
    }
    return v;
}

// ============================================================================
// UNIFIED SCENE DISTANCE ESTIMATOR (SDF)
// ============================================================================

float sdSphere(vec3 p, vec3 center, float radius) {
    return length(p - center) - radius;
}

float sdDisk(vec3 p, vec3 center, float radius, float thickness) {
    vec3 localP = p - center;
    vec2 d = abs(vec2(length(localP.xy), localP.z)) - vec2(radius, thickness);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

float map_scene(vec3 p, out int hitID) {
    float d = 1e5;
    hitID = 0;

    if (u_object1.w > 0.001) {
        float d1 = sdSphere(p, u_object1.xyz, u_object1.w);
        if (d1 < d) {
            d = d1;
            hitID = 1;
        }
    }

    if (u_object2.w > 0.001) {
        float d2 = sdSphere(p, u_object2.xyz, u_object2.w);
        if (d2 < d) {
            d = d2;
            hitID = 2;
        }
    }

    if (u_object3.w > 0.001 && length(u_object3.xyz) > 2.0) {
        float portalRadius = u_object3.w;
        float throatRadius = portalRadius * 0.12;
        float throatThickness = 0.01;
        float d3 = sdDisk(p, u_object3.xyz, throatRadius, throatThickness);
        if (d3 < d) {
            d = d3;
            hitID = 3;
        }
    }

    return d;
}

void main() {
    if (v_color0.a > 0.001) {
        gl_FragColor = v_color0;
        return;
    }

    vec3 camPos         = u_cameraPositionAndTime.xyz;
    float time          = u_cameraPositionAndTime.w;
    vec2 resolution     = u_resolutionAndMass.xy;
    float mass          = u_resolutionAndMass.z;
    float configHorizon = u_resolutionAndMass.w;
    vec3 camTarget      = u_cameraTarget.xyz;

    float visualHorizon = 0.8;

    // Camera basis — only for disk Doppler / steepness, NOT for the primary ray
    vec3 forward = normalize(camTarget - camPos);
    vec3 upProjected = vec3(0.0, 1.0, 0.0);
    if (abs(forward.y) > 0.99) {
        upProjected = vec3(0.0, 0.0, sign(forward.y));
    }
    vec3 right = normalize(cross(upProjected, forward));
    vec3 up = cross(forward, right);

    // Same frustum as View 1 (mtxLookAt + 80° mtxProj via u_invViewProj)
    vec2 ndc = (gl_FragCoord.xy - u_viewRect.xy) / u_viewRect.zw * 2.0 - 1.0;
#if !BGFX_SHADER_LANGUAGE_GLSL
    ndc.y = -ndc.y;
#endif
    vec4 worldFar = mul(u_invViewProj, vec4(ndc, 1.0, 1.0));
    worldFar /= worldFar.w;
    vec3 rayDir = normalize(worldFar.xyz - camPos);

    float dither = hash2d(gl_FragCoord.xy + fract(time * 0.05));
    vec3 rayPos = camPos + rayDir * (dither * 0.03);

    vec3 totalColor = vec3(0.0);
    bool hitSingularity = false;
    float steepness = abs(forward.y);
    float dynamicThickness = visualHorizon * (0.4 + steepness * 0.5);

    // ========================================================================
    // CORE RAYMARCHING ENGINE LOOP
    // ========================================================================
    for (int step = 0; step < 1000; step++) {
        float r2 = dot(rayPos, rayPos);
        float r = sqrt(r2);

        if (r <= visualHorizon) {
            hitSingularity = true;
            break;
        }

        int hitID = 0;
        float distToScene = map_scene(rayPos, hitID);

        if (distToScene <= 0.01) {
            if (hitID == 1) {
                vec3 localP = rayPos - u_object1.xyz;
                vec3 baseNormal = normalize(localP);
                float surfaceCrust = smoothFbm(baseNormal.xy * 3.5 + baseNormal.zz * 2.0);
                vec3 chthonianNormal = normalize(baseNormal + vec3(
                    smoothNoise(localP.yz * 6.0),
                    smoothNoise(localP.zx * 6.0),
                    smoothNoise(localP.xy * 6.0)
                ) * 0.3 * surfaceCrust);

                vec3 lightDir = normalize(vec3(1.0, 1.5, 1.0));
                float diff = max(dot(chthonianNormal, lightDir), 0.15);
                float rim = pow(1.0 - max(dot(chthonianNormal, -rayDir), 0.0), 3.0);

                vec3 rockColor = vec3(0.1, 0.12, 0.18);
                vec3 crystalVeins = vec3(0.5, 0.8, 1.0) * smoothstep(0.45, 0.75, surfaceCrust);
                vec3 surfaceColor = mix(rockColor, vec3(0.75, 0.9, 1.0), surfaceCrust * 0.4) + crystalVeins;

                totalColor = (surfaceColor * diff + vec3(0.2, 0.5, 0.9) * rim * 2.2) * 5.0;
            }
            else if (hitID == 2) {
                vec3 normal = normalize(rayPos - u_object2.xyz);
                float uCoord = atan2(normal.z, normal.x);
                float vCoord = normal.y;
                float planetTime = time * 0.02;

                vec2 planetUV = vec2(uCoord * 1.5 + planetTime, vCoord * 2.0);
                float cloudPattern = smoothFbm(planetUV * 1.5);

                vec3 deepColor = vec3(0.15, 0.03, 0.28);
                vec3 brightColor = vec3(0.6, 0.25, 0.8);
                vec3 stormColor = mix(deepColor, brightColor, cloudPattern);

                vec3 lightDir = normalize(vec3(1.0, 1.5, 1.0));
                float diff = max(dot(normal, lightDir), 0.15);
                float rim = pow(1.0 - max(dot(normal, -rayDir), 0.0), 2.5);

                totalColor = (stormColor * diff + vec3(0.3, 0.15, 0.5) * rim * 1.5) * 5.0;
            }
            else if (hitID == 3) {
                totalColor *= 0.0;
            }
            hitSingularity = true;
            break;
        }

        if (r > 600.0) {
            break;
        }

        float distToPortalBounding = 1e5;
        if (u_object3.w > 0.001) {
            distToPortalBounding = max(0.0, length(rayPos - u_object3.xyz) - (u_object3.w * 1.5));
        }

        float safeStepDist = min(distToScene, max(distToPortalBounding, 0.05));
        float stepSize = clamp(min(r * 0.025, max(safeStepDist * 0.5, 0.005)), 0.005, 2.5);

        vec3 gravityDir = -rayPos / r;
        float deflection = (5.5 * visualHorizon * (mass / 600.0)) / (r2 * r + 0.0001);

        rayDir = normalize(rayDir + gravityDir * deflection * stepSize * 0.45);
        rayPos += rayDir * stepSize;

        // ====================================================================
        // VOLUMETRIC INTEGRATION: Trans-Orbital Portal Vortex (Violet Glow)
        // ====================================================================
        if (u_object3.w > 0.001) {
            vec3 localP = rayPos - u_object3.xyz;
            float distToPortalCenter = length(localP);
            float portalRadius = u_object3.w;

            if (distToPortalCenter < portalRadius * 1.2) {
                float distXY = length(localP.xy);
                float distZ = abs(localP.z);

                if (distZ < portalRadius * 0.5) {
                    float angle = atan2(localP.y, localP.x);

                    float spiral = angle - distXY * 1.2 - time * 3.0;
                    float turbulence = smoothFbm(vec2(spiral * 3.0, distXY * 4.0 - time * 1.5));

                    float radialGlow = smoothstep(portalRadius, 0.0, distXY);
                    float axialFade = smoothstep(portalRadius * 0.5, 0.0, distZ);

                    float throatRadius = portalRadius * 0.12;
                    float edgeGlow = 0.05 / (abs(distXY - throatRadius) + 0.02);

                    vec3 deepPlasma = vec3(0.08, 0.01, 0.25);
                    vec3 brightPlasma = vec3(0.6, 0.15, 1.0);
                    vec3 coreEnergy = vec3(1.0, 0.6, 1.0);

                    vec3 emission = mix(deepPlasma, brightPlasma, turbulence);
                    emission += coreEnergy * edgeGlow * (turbulence * 0.5 + 0.5);

                    float density = (turbulence * 0.6 + 0.2) * radialGlow * axialFade;
                    totalColor += emission * density * stepSize * 14.0;
                }
            }
        }

        // ====================================================================
        // VOLUMETRIC INTEGRATION: Relativistic Accretion Disk
        // ====================================================================
        if (abs(rayPos.y) < dynamicThickness) {
            float distToCenter = length(rayPos.xz);
            float innerDisk = visualHorizon * 1.002;
            float outerDisk = max(length(u_object1.xz) * 1.02, 12.0);

            if (distToCenter > innerDisk && distToCenter < outerDisk) {
                vec2 dirXZ = rayPos.xz / (distToCenter + 0.0001);
                float speed = time * 0.15;
                float normDist = distToCenter / visualHorizon;
                float logRadius = log(normDist * 0.3);

                float rotAngle = 2.5 * logRadius - speed;
                float c = cos(rotAngle);
                float s = sin(rotAngle);
                vec2 rotatedXZ = vec2(dirXZ.x * c - dirXZ.y * s, dirXZ.x * s + dirXZ.y * c);

                vec2 vortexUV = vec2(logRadius * 1.5 - speed, rotatedXZ.x * 2.0 + rotatedXZ.y * 2.0);
                float gasStructure = smoothFbm(vortexUV * 0.5) * 3.0;

                float photonRing = 0.06 / abs(distToCenter - visualHorizon * 1.01 + 0.003);
                float glow = 1.6 / (distToCenter - visualHorizon + 0.02) + photonRing * 2.0;

                vec3 gasColor = vec3(0.2, 0.6, 1.8) * glow * 2.0;
                gasColor += vec3(0.45, 0.85, 3.0) * (gasStructure * glow * 3.5);

                float distToPlanet = length(rayPos - u_object1.xyz);
                float suctionInfluence = smoothstep(u_object1.w * 4.0, 0.0, distToPlanet);
                if (suctionInfluence > 0.0) {
                    float streamer = smoothFbm(rayPos.xz * 2.0 - time * 0.5) * suctionInfluence;
                    gasColor += vec3(0.5, 0.8, 2.5) * streamer * 10.0;
                }

                float coreGlow = smoothstep(visualHorizon * 2.5, visualHorizon * 1.005, distToCenter);
                gasColor += vec3(2.5, 3.0, 5.0) * (coreGlow * glow * 10.0);

                vec2 gasVelocityDir = vec2(-rayPos.z, rayPos.x) / (distToCenter + 0.0001);
                float viewAlignment = dot(gasVelocityDir, right.xz);
                float doppler = mix(0.5, 2.2, clamp(0.5 + 0.5 * viewAlignment, 0.0, 1.0));
                gasColor *= doppler;

                float edgeFade = smoothstep(outerDisk, outerDisk - 4.0, distToCenter);
                float angleDamping = mix(1.0, 0.3, steepness);
                float volumeFade = smoothstep(dynamicThickness, 0.0, abs(rayPos.y));

                totalColor += gasColor * 0.025 * 7.0 * edgeFade * angleDamping * volumeFade;
            }
        }
    }
if (!hitSingularity) {
        // ====================================================================
        // 1. GALAKTISCHER STAUB (Nebula)
        // ====================================================================
        vec2 nebUV = vec2(asin(rayDir.y), atan2(rayDir.z, rayDir.x));
        float dust = smoothFbm(nebUV * 2.5) * 0.5;
        float dust2 = smoothFbm(nebUV * 4.0 + vec2(1.2, 3.4)) * 0.5;
        
        vec3 nebulaColor = vec3(0.04, 0.01, 0.08) * dust + vec3(0.01, 0.04, 0.09) * dust2;
        totalColor += nebulaColor;

        // ====================================================================
        // 2. HINTERGRUND-STERNE (Millionen ferne, winzige Punkte)
        // ====================================================================
        float hashA = starHash3D(floor(rayDir * 800.0));
        float starA = smoothstep(0.995, 1.0, hashA);
        totalColor += vec3(0.5, 0.6, 0.9) * starA * 0.4; 

        // ====================================================================
        // 3. VORDERGRUND-STERNE (Farbige, helle Punkte)
        // ====================================================================
        vec3 gridB = floor(rayDir * 300.0);
        float hashB = starHash3D(gridB + vec3(42.0, 17.0, 99.0));
        
        if (hashB > 0.985) {
            vec3 fractB = fract(rayDir * 300.0) - 0.5;
            float dotSize = max(0.0, 1.0 - length(fractB) * 2.5); 
            float intensity = smoothstep(0.985, 1.0, hashB) * dotSize;
            float twinkle = sin(time * 4.0 + hashB * 1000.0) * 0.4 + 0.6;
            vec3 starColor = mix(vec3(1.0, 0.4, 0.2), vec3(0.4, 0.8, 1.0), fract(hashB * 123.456));
            totalColor += starColor * intensity * twinkle * 3.5;
        }

        // ====================================================================
        // 4. CONSTELLATION: "Tachikoma Major" EASTER EGG
        // ====================================================================
        if (rayDir.y > 0.4) {
            vec2 p = rayDir.xz / rayDir.y; 
            
            // Rotation und Skalierung der Konstellation am Himmel
            float angle = -0.4;
            float s = sin(angle), c = cos(angle);
            p = vec2(p.x * c - p.y * s, p.x * s + p.y * c);
            p = p * 15.0 + vec2(0.0, 5.0); 
            
            // Die Sterne der Konstellation (Knotenpunkte des Tachikoma)
            vec2 s1 = vec2(0.0, 1.5);   // Abdomen Spitze
            vec2 s2 = vec2(0.0, 0.5);   // Abdomen Basis / Körper
            vec2 s3 = vec2(0.0, -0.2);  // Kopf Zentrum
            vec2 s4 = vec2(-0.4, -0.5); // Linkes Auge
            vec2 s5 = vec2(0.4, -0.5);  // Rechtes Auge
            vec2 s6 = vec2(0.0, -0.8);  // Mittleres Auge
            vec2 s7 = vec2(-1.2, 0.2);  // Knie Vorne Links
            vec2 s8 = vec2(-1.5, -0.8); // Fuß Vorne Links
            vec2 s9 = vec2(1.2, 0.2);   // Knie Vorne Rechts
            vec2 s10 = vec2(1.5, -0.8); // Fuß Vorne Rechts
            vec2 s11 = vec2(-1.0, 1.2); // Knie Hinten Links
            vec2 s12 = vec2(-1.8, 1.6); // Fuß Hinten Links
            vec2 s13 = vec2(1.0, 1.2);  // Knie Hinten Rechts
            vec2 s14 = vec2(1.8, 1.6);  // Fuß Hinten Rechts
            
            // Berechne Abstand zu den Sternen
            float dStar = 100.0;
            dStar = min(dStar, length(p - s1)); dStar = min(dStar, length(p - s2));
            dStar = min(dStar, length(p - s3)); dStar = min(dStar, length(p - s4));
            dStar = min(dStar, length(p - s5)); dStar = min(dStar, length(p - s6));
            dStar = min(dStar, length(p - s7)); dStar = min(dStar, length(p - s8));
            dStar = min(dStar, length(p - s9)); dStar = min(dStar, length(p - s10));
            dStar = min(dStar, length(p - s11)); dStar = min(dStar, length(p - s12));
            dStar = min(dStar, length(p - s13)); dStar = min(dStar, length(p - s14));
            
            // Sterne Zeichnen (Hell & Funkelnd)
            float starInt = smoothstep(0.12, 0.02, dStar);
            float starGlow = smoothstep(0.4, 0.0, dStar) * 0.5;
            float twinkle = sin(time * 6.0 + p.x * 10.0) * 0.4 + 0.6;
            
            vec3 constColor = vec3(0.5, 0.8, 1.0); // Blau-Weiße Riesen
            totalColor += constColor * starInt * 4.0 * twinkle;
            totalColor += constColor * starGlow;

            // Feine Linien der Sternkarte (verbindet die Sterne)
            #define L(a, b) length(p - a - (b - a) * clamp(dot(p - a, b - a) / dot(b - a, b - a), 0.0, 1.0))
            float dLine = 100.0;
            dLine = min(dLine, L(s1, s2)); dLine = min(dLine, L(s2, s3));
            dLine = min(dLine, L(s3, s4)); dLine = min(dLine, L(s3, s5));
            dLine = min(dLine, L(s3, s6)); dLine = min(dLine, L(s2, s7));
            dLine = min(dLine, L(s7, s8)); dLine = min(dLine, L(s2, s9));
            dLine = min(dLine, L(s9, s10)); dLine = min(dLine, L(s2, s11));
            dLine = min(dLine, L(s11, s12)); dLine = min(dLine, L(s2, s13));
            dLine = min(dLine, L(s13, s14));
            
            // Zarte Einblendung der Linien
            float lineInt = smoothstep(0.03, 0.01, dLine) * 0.15; 
            totalColor += constColor * lineInt;
        }
    }

    totalColor = totalColor / (totalColor + vec3(1.0));
    totalColor = pow(totalColor, vec3(0.85));
    gl_FragColor = vec4(totalColor, 1.0);
}