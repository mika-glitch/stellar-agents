#include "relativity.h"
#include "raymath.h"
#include <cmath>

Vector3 Relativity::GetApparentPosition(Vector3 realPos, Vector3 camPos, Vector3 holePos, float mass, float eventHorizon) {
    Vector3 relCam = Vector3Subtract(camPos, holePos);
    Vector3 relObj = Vector3Subtract(realPos, holePos);

    float distObj = Vector3Length(relObj);
    if (distObj < eventHorizon) return realPos; // Im Abgrund gibt es keine optische Rettung

    Vector3 viewDir = Vector3Normalize(Vector3Subtract(realPos, camPos));

    float t = Vector3DotProduct(Vector3Negate(relCam), viewDir);
    if (t < 0.0f) return realPos;

    Vector3 closestPoint = Vector3Add(camPos, Vector3Scale(viewDir, t));
    float b = Vector3Distance(closestPoint, holePos);

    if (b < eventHorizon * 1.05f) b = eventHorizon * 1.05f;

    // Einstein-Ablenkungs-Winkel berechnen
    float deflectionAngle = (4.0f * (mass * 0.0015f)) / b;

    Vector3 radialVector = Vector3Normalize(Vector3Subtract(closestPoint, holePos));
    Vector3 apparentClosestPoint = Vector3Add(holePos, Vector3Scale(radialVector, b * (1.0f + deflectionAngle)));

    return Vector3Add(apparentClosestPoint, Vector3Scale(viewDir, distObj - b));
}
