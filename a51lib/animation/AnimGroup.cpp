

#include "animData.h"
#include "../xfiles/x_plus.h"
#include "../math/Random.h"

#include <cassert>

int AnimGroup::GetBoneIndex(const char* pBoneName, bool FindAnywhere) const
{
    // If bone name is empty then just return
    if (pBoneName[0] == 0) {
        return -1;
    }

    // Check if pBoneName is found anywhere in the string
    if (FindAnywhere) {
        for (int i = 0; i < numBones; i++) {
            if (x_stristr(bones[i].name.c_str(), pBoneName) != NULL) {
                return i;
            }
        }
    } else {
        for (int i = 0; i < numBones; i++) {
            if (x_stricmp(bones[i].name.c_str(), pBoneName) == 0) {
                return i;
            }
        }
    }

    return -1;
}

//=========================================================================

void AnimGroup::GetL2W(const Matrix4& L2W, float Frame, int iAnim, Matrix4* pBoneL2W) const
{
    // Allocate location for keys
    AnimKey* pKey = (AnimKey*)malloc(sizeof(AnimKey) * numBones);

    // Get keys from anim
    anims[iAnim].GetInterpKeys(Frame, pKey);

    // Build matrices
    ComputeBonesL2W(L2W, pKey, numBones, pBoneL2W);

    // Free key array
    free(pKey);
}

//=========================================================================

int AnimGroup::GetAnimIndex(const char* pAnimName) const
{
    
    for (int i = 0; i < numAnims; i++) {
        if (x_stricmp(anims[i].GetName(), pAnimName) == 0) {
            return i;
        }
    }
    
    return -1;
}

//=========================================================================

int AnimGroup::GetRandomAnimIndex(const char* pAnimName, int iSkipAnim /*= -1*/) const
{
    // Lookup start of animations with this name
    int iStartAnim = GetAnimIndex(pAnimName);
    if (iStartAnim == -1) {
        return -1;
    }

    // Now select a random anim
    return GetRandomAnimIndex(iStartAnim, iSkipAnim);
}

//=========================================================================

int AnimGroup::GetRandomAnimIndex(int iStartAnim, int iSkipAnim /*= -1*/) const
{
    // If there is just one anim of this type, then use it
    const AnimInfo& StartAnimInfo = GetAnimInfo(iStartAnim);

    // Get # of anims of the same name
    int nAnims = StartAnimInfo.GetNAnims();

    // If there is just one anim, then use it
    if (nAnims == 1) {
        return iStartAnim;
    }

    // Compute total weight
    float TotalWeight = StartAnimInfo.GetAnimsWeight();

    // If skip anim has the same name as the requested anim, then skip it so
    // it's not chosen twice in a row
    if ((iSkipAnim >= iStartAnim) && (iSkipAnim < (iStartAnim + nAnims))) {
        TotalWeight -= GetAnimInfo(iSkipAnim).GetWeight();
    }

    // Choose a random weight
    float WeightChosen = x_frand(0.0f, TotalWeight);

    // Search for anim with chosen weight
    int   iAnim = iStartAnim;
    float WeightSum = 0.0f;
    while (1) {
        // Skip?
        if (iAnim != iSkipAnim) {
            // Update sum
            WeightSum += GetAnimInfo(iAnim).GetWeight();

            // Found?
            if (WeightSum >= WeightChosen) {
                break;
            }
        }

        // Check next
        iAnim++;
    }

    // Make sure this logic works!
    assert(iAnim != iSkipAnim);
    assert((iAnim >= iStartAnim) && (iAnim < (iStartAnim + nAnims)));
    assert((iAnim >= 0) && (iAnim < GetNAnims()));

    return iAnim;
}

//=========================================================================

void AnimGroup::ComputeBonesL2W(const Matrix4& L2W, AnimKey* pKey, int nBones, Matrix4* pBoneL2W, bool bApplyTheBindPose) const
{
    int i;

    assert(nBones <= numBones);

    // Convert all keys to matrices and put into world space
    for (i = 0; i < nBones; i++) {
        // Setup L2W
        pKey[i].Setup(pBoneL2W[i]);

        // Concatenate with parent or L2W
        const Matrix4* PM = (bones[i].iParent == -1) ? (&L2W) : (&pBoneL2W[bones[i].iParent]);
        pBoneL2W[i] = (*PM) * pBoneL2W[i];
    }

    if (bApplyTheBindPose == false) {
        return;
    }

    // Apply bind matrices
    for (i = 0; i < nBones; i++) {
        pBoneL2W[i] = pBoneL2W[i] * bones[i].bindMatrixInv;
    }
}

//=========================================================================

void AnimGroup::ComputeBoneL2W(int iBone, const Matrix4& L2W, AnimKey* pKey, Matrix4& BoneL2W) const
{
    assert(iBone < numBones);

    // Clear bone matrix
    BoneL2W.Identity();

    // Run hierarchy from bone to root node
    int I = iBone;
    while (I != -1) {
        Matrix4 LM;
        pKey[I].Setup(LM);
        BoneL2W = LM * BoneL2W;
        I = bones[I].iParent;
    }

    // Concatenate L2W
    BoneL2W = L2W * BoneL2W;

    // Apply bind matrix
    BoneL2W = BoneL2W * bones[iBone].bindMatrixInv;
}

//=========================================================================

Vector3 AnimGroup::GetEventPos(int iBone, const Vector3& Offset, AnimKey* pKey) const
{
    Matrix4 BoneM;
    Matrix4 IdentM;
    IdentM.Identity();

    ComputeBoneL2W(iBone, IdentM, pKey, BoneM);

    Vector3 P = BoneM * Offset;
    return P;
}

//=========================================================================

Radian3 AnimGroup::GetEventRot(int iBone, const Vector3& Offset, AnimKey* pKey) const
{
    Matrix4 BoneM;
    Matrix4 IdentM;
    IdentM.Identity();

    ComputeBoneL2W(iBone, IdentM, pKey, BoneM);

    // Offset is an angular offset
    Radian3 Rot(Offset.GetX(), Offset.GetY(), Offset.GetZ());

    Matrix4 RotM;
    RotM.Identity();
    RotM.Rotate(Rot);
    RotM = BoneM * RotM;
    return RotM.GetRotation();
}
