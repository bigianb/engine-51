#include "streamingOperators.h"

std::ostream& operator<<(std::ostream& os, const Vector3& obj)
{
    os << "[ " << obj.x << ", " << obj.y << ", " << obj.z << " ]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Quaternion& obj)
{
    os << "[ " << obj.x << ", " << obj.y << ", " << obj.z << ", " << obj.w << " ]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const BBox& aBBox)
{
    os << aBBox.min << aBBox.max;
    return os;
}
