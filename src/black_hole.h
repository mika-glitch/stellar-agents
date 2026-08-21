#pragma once
#include "raylib.h"

class BlackHole {
private:
    Vector3 position;
    float mass;
    float eventHorizon;

public:
    BlackHole(Vector3 pos, float m, float r);
    void Update(float deltaTime);
    void Draw(Camera3D camera) const;

    
    Vector3 CalculateGravity(Vector3 objectPos) const;
    bool HasCrossedPointOfNoReturn(Vector3 objectPos) const;

    // small getter
    Vector3 GetPosition() const { return position; }
    float GetMass() const { return mass; }
    float GetEventHorizon() const { return eventHorizon; }
};