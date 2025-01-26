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

std::ostream& operator<<(std::ostream& os, const PropertyType& pt)
{
    switch (pt & PROP_TYPE_BASIC_MASK) {
    case PROP_TYPE_NULL:
        os << "null";
        break;
    case PROP_TYPE_FLOAT:
        os << "flost";
        break;
    case PROP_TYPE_INT:
        os << "int";
        break;
    case PROP_TYPE_BOOL:
        os << "bool";
        break;
    case PROP_TYPE_VECTOR2:
        os << "vector2";
        break;
    case PROP_TYPE_VECTOR3:
        os << "vector3";
        break;
    case PROP_TYPE_ROTATION:
        os << "rotation";
        break;
    case PROP_TYPE_ANGLE:
        os << "angle";
        break;
    case PROP_TYPE_BBOX:
        os << "bbox";
        break;
    case PROP_TYPE_GUID:
        os << "GUID";
        break;
    case PROP_TYPE_COLOR:
        os << "colour";
        break;
    case PROP_TYPE_STRING:
        os << "string";
        break;
    case PROP_TYPE_ENUM:
        os << "enum";
        break;
    case PROP_TYPE_BUTTON:
        os << "button";
        break;
    case PROP_TYPE_EXTERNAL:
        os << "external";
        break;
    case PROP_TYPE_FILENAME:
        os << "filename";
        break;
    }
    if (pt & ~PROP_TYPE_BASIC_MASK) {
        const int flags = pt & ~PROP_TYPE_BASIC_MASK;
        if (flags & PROP_TYPE_HEADER) {
            os << " | header";
        }
        if (flags & PROP_TYPE_READ_ONLY) {
            os << " | read only";
        }
        if (flags & PROP_TYPE_MUST_ENUM) {
            os << " | must enum";
        }
        if (flags & PROP_TYPE_DONT_SAVE) {
            os << " | don't save";
        }
        if (flags & PROP_TYPE_DONT_SHOW) {
            os << " | don't show";
        }
        if (flags & PROP_TYPE_DONT_EXPORT) {
            os << " | don't export";
        }
        if (flags & PROP_TYPE_EXPOSE) {
            os << " | expose";
        }
        if (flags & PROP_TYPE_DONT_SAVE_MEMCARD) {
            os << " | don't save on memcard";
        }
        if (flags & PROP_TYPE_DONT_COPY) {
            os << " | don't copy";
        }
    }
    return os;
}
