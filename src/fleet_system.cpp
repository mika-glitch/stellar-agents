#include "universe.h"
#include "fleet_system.h"
#include "relativity.h"
#include "rlgl.h"
#include "raymath.h"
#include <cmath>

// Constructor: Binds the live references cleanly via initializer list bounds
FleetSystem::FleetSystem(const BlackHole& hole, const Universe& uni)
    : blackHole(hole), universe(uni) {

    // Tamsa II is located at index 1. We spawn them around its initial position at frame zero.
    Vector3 tamsaTwoPos = universe.GetPlanetPosition(1);
    if (tamsaTwoPos.x == 0.0f && tamsaTwoPos.y == 0.0f && tamsaTwoPos.z == 0.0f) {
        tamsaTwoPos = Vector3{ 0.0f, 0.0f, -12.0f }; // Standard fallback
    }

    SpawnVessel(Vector3Add(tamsaTwoPos, Vector3{ 0.0f, 0.3f, 1.2f }), ShipType::LIGHT_FIGHTER, "Anvil Arrow (Patrol Alpha)");
    SpawnVessel(Vector3Add(tamsaTwoPos, Vector3{ -1.5f, -0.1f, -1.5f }), ShipType::HEAVY_FIGHTER, "Aegis Vanguard (Deep Space Recon)");
    SpawnVessel(Vector3Add(tamsaTwoPos, Vector3{ 2.5f, 0.0f, 0.5f }), ShipType::CAPITAL_SHIP, "UEE Idris-M (Battle Group Tamsa)");
}

void FleetSystem::SpawnVessel(Vector3 pos, ShipType type, const std::string& vesselName) {
    float massFactor = 1.0f;
    float thrust = 4.2f;
    Color glow = SKYBLUE;

    if (type == ShipType::HEAVY_FIGHTER) {
        massFactor = 1.1f;
        thrust = 7.5f;
        glow = ORANGE;
    }
    else if (type == ShipType::CAPITAL_SHIP) {
        massFactor = 2.5f;
        thrust = 24.0f;
        glow = Color{ 0, 255, 180, 255 };
    }

    float radius = Vector3Length(pos);
    float speed = sqrtf(blackHole.GetMass() / (radius > 0.1f ? radius : 1.0f)) * massFactor;

    Vector3 vel = { -pos.z, 0.0f, pos.x };
    if (Vector3Length(vel) > 0.001f) {
        vel = Vector3Scale(Vector3Normalize(vel), speed);
    }

    ships.push_back(SpaceShip{ pos, vel, type, vesselName, thrust, glow, true });
}

// Tick processing frame updates: ADVANCED LIVE ORBITAL TRACKING AI
void FleetSystem::Update(float dt) {
    float currentTime = GetTime();

    // LIVE SCANNING: Extract the active dynamic target position of the gas giant (Index 1) every frame!
    Vector3 tamsaTwoLivePos = universe.GetPlanetPosition(1);

    for (auto& ship : ships) {
        if (!ship.active) continue;

        if (blackHole.HasCrossedPointOfNoReturn(ship.position)) {
            ship.active = false;
            continue;
        }

        // 1. EXTRACT CENTRAL Gravitational acceleration pull
        Vector3 gravity = blackHole.CalculateGravity(ship.position);
        float currentDistanceToBH = Vector3Distance(ship.position, blackHole.GetPosition());

        // 2. COMPUTE LIVE TRACKING THICK COUPLING VECTOR TO TAMSA II
        // The pilot computer evaluates its position relative to the moving gas giant
        Vector3 vecToPlanet = Vector3Subtract(tamsaTwoLivePos, ship.position);
        float distToPlanet = Vector3Length(vecToPlanet);
        Vector3 dirToPlanet = (distToPlanet > 0.01f) ? Vector3Scale(vecToPlanet, 1.0f / distToPlanet) : Vector3Zero();

        Vector3 outwardDirection = Vector3Normalize(Vector3Scale(gravity, -1.0f));
        Vector3 flightDirection = Vector3Normalize(ship.velocity);
        Vector3 combinedThrust = Vector3Zero();

        // 3. KI-DECISION MATRIX: EMERGENCY AFTERBURNER SENSOR
        float emergencyThreshold = blackHole.GetEventHorizon() * 5.0f;
        bool emergencyAfterburner = (currentDistanceToBH < emergencyThreshold);

        if (emergencyAfterburner) {
            float afterburnerThrust = ship.thrustPower * 4.0f;
            combinedThrust = Vector3Add(
                Vector3Scale(flightDirection, ship.thrustPower * 0.3f),
                Vector3Scale(outwardDirection, afterburnerThrust)
            );
        }
        else {
            // HIGH-PERFORMANCE COUPLING PATHS:
            // Adjust thrust steering vectors to actively dynamically bind to Tamsa II's current sector coordinates!
            float orbitHoldForce = 2.5f;
            Vector3 planetAttractionThrust = Vector3Scale(dirToPlanet, ship.thrustPower * orbitHoldForce / (distToPlanet + 0.5f));

            float evasionFrequency = (ship.type == ShipType::LIGHT_FIGHTER) ? 6.0f : 2.5f;
            float evasionMagnitude = (ship.type == ShipType::LIGHT_FIGHTER) ? 2.0f : 0.5f;
            float weaveOffset = sinf(currentTime * evasionFrequency + currentDistanceToBH) * evasionMagnitude;

            Vector3 perpendicularEvasionVector = { -ship.velocity.z, ship.velocity.y, ship.velocity.x };
            if (Vector3Length(perpendicularEvasionVector) > 0.001f) {
                perpendicularEvasionVector = Vector3Normalize(perpendicularEvasionVector);
            }

            combinedThrust = Vector3Add(
                Vector3Scale(flightDirection, ship.thrustPower * 0.8f),
                Vector3Scale(outwardDirection, ship.thrustPower * 1.2f)
            );

            // Inject planet binding fields and asteroid weave motions simultaneously
            combinedThrust = Vector3Add(combinedThrust, planetAttractionThrust);
            combinedThrust = Vector3Add(combinedThrust, Vector3Scale(perpendicularEvasionVector, weaveOffset));
        }

        Vector3 totalAcceleration = Vector3Add(gravity, combinedThrust);
        ship.velocity = Vector3Add(ship.velocity, Vector3Scale(totalAcceleration, dt));
        ship.position = Vector3Add(ship.position, Vector3Scale(ship.velocity, dt));
    }
}

void FleetSystem::Draw3D(Camera3D camera) const {
    float currentTime = GetTime();
    BeginMode3D(camera);
    for (const auto& ship : ships) {
        if (!ship.active) continue;

        Vector3 apparentPos = Relativity::GetApparentPosition(
            ship.position, camera.position, blackHole.GetPosition(), blackHole.GetMass(), blackHole.GetEventHorizon()
        );

        float hullRadius = 0.08f;
        if (ship.type == ShipType::HEAVY_FIGHTER) hullRadius = 0.14f;
        else if (ship.type == ShipType::CAPITAL_SHIP) hullRadius = 0.45f;

        float currentDistance = Vector3Distance(ship.position, blackHole.GetPosition());
        bool afterburnerActive = (currentDistance < blackHole.GetEventHorizon() * 5.0f);

        Color dynamicGlow = ship.engineGlowColor;
        float flareScale = 1.0f + (sinf(currentTime * 30.0f) * 0.15f);

        if (afterburnerActive) {
            dynamicGlow = RED;
            flareScale *= 3.0f;
        }

        DrawSphere(apparentPos, hullRadius, WHITE);
        DrawSphereWires(apparentPos, hullRadius + (0.03f * flareScale), 4, 4, dynamicGlow);
    }
    EndMode3D();
}

void FleetSystem::DrawHUD(Camera3D camera) const {
    for (const auto& ship : ships) {
        if (!ship.active) continue;

        Vector3 apparentPos = Relativity::GetApparentPosition(
            ship.position, camera.position, blackHole.GetPosition(), blackHole.GetMass(), blackHole.GetEventHorizon()
        );
        Vector2 screenPos = GetWorldToScreen(apparentPos, camera);

        if (screenPos.x > 0 && screenPos.x < GetScreenWidth() && screenPos.y > 0 && screenPos.y < GetScreenHeight()) {
            int size = (ship.type == ShipType::CAPITAL_SHIP) ? 14 : 8;
            float currentDistance = Vector3Distance(ship.position, blackHole.GetPosition());
            bool afterburnerActive = (currentDistance < blackHole.GetEventHorizon() * 5.0f);

            Color alertColor = ship.engineGlowColor;
            std::string telemetryLabel = ship.name;

            if (afterburnerActive) {
                alertColor = RED;
                telemetryLabel += " [AFTERBURNER ACTIVE]";
            }

            DrawCircleLines((int)screenPos.x, (int)screenPos.y, (float)size, alertColor);
            DrawCircleLines((int)screenPos.x, (int)screenPos.y, (float)size + 3.0f, alertColor);
            DrawText(telemetryLabel.c_str(), (int)screenPos.x + size + 8, (int)screenPos.y - 6, 12, alertColor);
        }
    }
}
