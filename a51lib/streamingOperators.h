#pragma once
#include <iostream>
#include "VectorMath.h"

std::ostream& operator<<(std::ostream& os, const Vector3& obj);
std::ostream& operator<<(std::ostream& os, const Quaternion& obj);
std::ostream& operator<<(std::ostream& os, const BBox& aBBox);
