#include "BoundingBox.h"

bool BBox::Intersect(float& t, const Vector3& P0, const Vector3& P1) const
{
    float   PlaneD[3];
    bool    PlaneUsed[3] = {true, true, true};
    float   T[3] = {-1, -1, -1};
    Vector3 Direction = P1 - P0;
    int     MaxPlane;
    int     i;
    float   Component;

    // Set a value until we have something better.
    t = 0.0f;

    // Consider relationship of each component of P0 to the box.
    for (i = 0; i < 3; i++) {
        if (P0[i] > max[i]) {
            PlaneD[i] = max[i];
        } else if (P0[i] < min[i]) {
            PlaneD[i] = min[i];
        } else {
            PlaneUsed[i] = false;
        }
    }

    // Is the starting point in the box?
    if (!PlaneUsed[0] && !PlaneUsed[1] && !PlaneUsed[2]) {
        return true;
    }

    // For each plane to be used, compute the distance to the plane.
    for (i = 0; i < 3; i++) {
        if (PlaneUsed[i] && (Direction[i] != 0.0f)) {
            T[i] = (PlaneD[i] - P0[i]) / Direction[i];
        }
    }

    // We need to know which plane had the largest distance.
    if (T[0] > T[1]) {
        MaxPlane = ((T[0] > T[2]) ? 0 : 2);
    } else {
        MaxPlane = ((T[1] > T[2]) ? 1 : 2);
    }

    // If the largest plane distance is less than zero, then there is no hit.
    if (T[MaxPlane] < 0.0f) {
        return false;
    }

    // See if the point we think is the hit point is a real hit.
    for (i = 0; i < 3; i++) {
        // See if component 'i' of the hit point is on the box.
        if (i != MaxPlane) {
            Component = P0[i] + T[MaxPlane] * Direction[i];
            if ((Component < min[i]) || (Component > max[i])) {
                // We missed!  Hit point was not on the box.
                return false;
            }
        }
    }

    // We have a verified hit.  Set t and we're done.
    t = T[MaxPlane];
    return true;
}
