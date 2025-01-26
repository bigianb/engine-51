#pragma once
#include <iostream>
#include "VectorMath.h"
#include "PropertyDefs.h"

std::ostream& operator<<(std::ostream& os, const Vector3& obj);
std::ostream& operator<<(std::ostream& os, const Quaternion& obj);
std::ostream& operator<<(std::ostream& os, const BBox& aBBox);
std::ostream& operator<<(std::ostream& os, const Matrix4& mtx);
std::ostream& operator<<(std::ostream& os, const PropertyType& pt);
