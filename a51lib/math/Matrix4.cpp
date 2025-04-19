#include "Matrix4.h"

#define M(column, row) cells[column][row]

bool Matrix4::Invert()
{
    float Scratch[4][8];
    float a;
    int i, j, k, jr, Pivot;
    int Row[4];

    //
    // Initialize.
    //

    for (j = 0; j < 4; j++) {
        for (k = 0; k < 4; k++) {
            Scratch[j][k] = M(j, k);
            Scratch[j][4 + k] = 0.0f;
        }

        Scratch[j][4 + j] = 1.0f;
        Row[j] = j;
    }

    //
    // Eliminate columns.
    //

    for (i = 0; i < 4; i++) {
        // Find pivot.
        k = i;

        a = abs(Scratch[Row[k]][k]);

        for (j = i + 1; j < 4; j++) {
            jr = Row[j];

            if (a < abs(Scratch[jr][i])) {
                k = j;
                a = abs(Scratch[jr][i]);
            }
        }

        // Swap the pivot row (Row[k]) with the i'th row.
        Pivot = Row[k];
        Row[k] = Row[i];
        Row[i] = Pivot;

        // Normalize pivot row.
        a = Scratch[Pivot][i];

        if (a == 0.0f) {
            return false;
        }

        Scratch[Pivot][i] = 1.0f;

        for (k = i + 1; k < 8; k++) {
            Scratch[Pivot][k] /= a;
        }

        // Eliminate pivot from all remaining rows.
        for (j = i + 1; j < 4; j++) {
            jr = Row[j];
            a = -Scratch[jr][i];

            if (a == 0.0f) {
                continue;
            }

            Scratch[jr][i] = 0.0f;

            for (k = i + 1; k < 8; k++) {
                Scratch[jr][k] += (a * Scratch[Pivot][k]);
            }
        }
    }

    //
    // Back solve.
    //

    for (i = 3; i > 0; i--) {
        Pivot = Row[i];
        for (j = i - 1; j >= 0; j--) {
            jr = Row[j];
            a = Scratch[jr][i];

            for (k = i; k < 8; k++) {
                Scratch[jr][k] -= (a * Scratch[Pivot][k]);
            }
        }
    }

    //
    // Copy inverse back into the matrix.
    //

    for (j = 0; j < 4; j++) {
        jr = Row[j];
        for (k = 0; k < 4; k++) {
            M(j, k) = Scratch[jr][k + 4];
        }
    }
    // Success!
    return true;
}

bool Matrix4::InvertSRT()
{
    Matrix4 Src = *this;

    //
    // Calculate the determinant.
    //

    float Determinant = (Src.M(0, 0) * (Src.M(1, 1) * Src.M(2, 2) - Src.M(1, 2) * Src.M(2, 1)) -
                         Src.M(0, 1) * (Src.M(1, 0) * Src.M(2, 2) - Src.M(1, 2) * Src.M(2, 0)) +
                         Src.M(0, 2) * (Src.M(1, 0) * Src.M(2, 1) - Src.M(1, 1) * Src.M(2, 0)));

    if (abs(Determinant) < 0.00001f) {
        return false;
    }

    Determinant = 1.0f / Determinant;

    //
    // Find the inverse of the matrix.
    //

    M(0, 0) = Determinant * (Src.M(1, 1) * Src.M(2, 2) - Src.M(1, 2) * Src.M(2, 1));
    M(0, 1) = -Determinant * (Src.M(0, 1) * Src.M(2, 2) - Src.M(0, 2) * Src.M(2, 1));
    M(0, 2) = Determinant * (Src.M(0, 1) * Src.M(1, 2) - Src.M(0, 2) * Src.M(1, 1));
    M(0, 3) = 0.0f;

    M(1, 0) = -Determinant * (Src.M(1, 0) * Src.M(2, 2) - Src.M(1, 2) * Src.M(2, 0));
    M(1, 1) = Determinant * (Src.M(0, 0) * Src.M(2, 2) - Src.M(0, 2) * Src.M(2, 0));
    M(1, 2) = -Determinant * (Src.M(0, 0) * Src.M(1, 2) - Src.M(0, 2) * Src.M(1, 0));
    M(1, 3) = 0.0f;

    M(2, 0) = Determinant * (Src.M(1, 0) * Src.M(2, 1) - Src.M(1, 1) * Src.M(2, 0));
    M(2, 1) = -Determinant * (Src.M(0, 0) * Src.M(2, 1) - Src.M(0, 1) * Src.M(2, 0));
    M(2, 2) = Determinant * (Src.M(0, 0) * Src.M(1, 1) - Src.M(0, 1) * Src.M(1, 0));
    M(2, 3) = 0.0f;

    M(3, 0) = -(Src.M(3, 0) * M(0, 0) + Src.M(3, 1) * M(1, 0) + Src.M(3, 2) * M(2, 0));
    M(3, 1) = -(Src.M(3, 0) * M(0, 1) + Src.M(3, 1) * M(1, 1) + Src.M(3, 2) * M(2, 1));
    M(3, 2) = -(Src.M(3, 0) * M(0, 2) + Src.M(3, 1) * M(1, 2) + Src.M(3, 2) * M(2, 2));
    M(3, 3) = 1.0f;

    // Success!
    return true;
}
