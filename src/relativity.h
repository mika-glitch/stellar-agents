#pragma once
#include "raylib.h"

namespace Relativity {
  
    Vector3 GetApparentPosition(Vector3 realPos, Vector3 camPos, Vector3 holePos, float mass, float eventHorizon);
}