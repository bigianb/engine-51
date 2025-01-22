#include "streamingOperators.h"

std::ostream& operator<<(std::ostream& os, const Vector3& obj)
{
    os << "[ " << obj.x << ", " << obj.y << ", " << obj.z << " ]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Vector4& obj)
{
    os << "[ " << obj.x << ", " << obj.y << ", " << obj.z << ", " << obj.w << " ]";
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

std::ostream& operator<<(std::ostream& os, const Matrix4& mtx)
{
    os << mtx.vCol0 << mtx.vCol1 << mtx.vCol2 << mtx.vCol3;
    return os;
}
