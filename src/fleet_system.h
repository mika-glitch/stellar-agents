#pragma once
#include "raylib.h"
#include "black_hole.h"
#include <vector>
#include <string>

class Universe; // Forward declaration

enum class ShipType {
    LIGHT_FIGHTER,
    HEAVY_FIGHTER,
    CAPITAL_SHIP
};

struct SpaceShip {
    Vector3 position;
    Vector3 velocity;
    ShipType type;
    std::string name;
    float thrustPower;
    Color engineGlowColor;
    bool active;
};

class FleetSystem {
private:
    const BlackHole& blackHole;
    const Universe& universe; // FIXED: Keep a permanent live link to your active universe!
    std::vector<SpaceShip> ships;

public:
    // Constructor matches your main.cpp initialization signature
    FleetSystem(const BlackHole& hole, const Universe& uni);

    void Update(float dt);

    void Draw3D(Camera3D camera) const;
    void DrawHUD(Camera3D camera) const;

    void SpawnVessel(Vector3 pos, ShipType type, const std::string& vesselName);
    const std::vector<SpaceShip>& GetActiveShips() const { return ships; }
};
