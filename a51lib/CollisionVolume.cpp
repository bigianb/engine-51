#include "CollisionVolume.h"
#include "InevFile.h"

void CollisionData::MatInfo::read(InevFile& inevFile)
{
    inevFile.read(soundType);
    inevFile.read(flags);
}

void CollisionData::HighCluster::read(InevFile& inevFile)
{
    inevFile.read(bbox);
    inevFile.read(nTris);
    inevFile.read(iMesh);
    inevFile.read(iBone);
    inevFile.read(iDList);
    inevFile.read(iOffset);
    materialInfo.read(inevFile);
}

void CollisionData::LowQuad::read(InevFile& inevFile)
{
    inevFile.read(iP[0]);
    inevFile.read(iP[1]);
    inevFile.read(iP[2]);
    inevFile.read(iP[3]);
    inevFile.read(iN);
    inevFile.read(flags);
}

void CollisionData::LowCluster::read(InevFile& inevFile)
{
    inevFile.read(bbox);
    inevFile.read(iVectorOffset);
    inevFile.read(nPoints);
    inevFile.read(nNormals);
    inevFile.read(iQuadOffset);
    inevFile.read(nQuads);
    inevFile.read(iMesh);
    inevFile.read(iBone);
}

void CollisionData::read(InevFile& inevFile)
{
    inevFile.read(bbox);
    inevFile.read(numHighClusters);
    inevFile.readArray(highClusters, numHighClusters);
    inevFile.read(numHighIndices);
    inevFile.readNativeArray(highIndexToVert0, numHighIndices);
    inevFile.read(numLowClusters);
    inevFile.read(numLowVectors);
    inevFile.read(numLowQuads);
    inevFile.readArray(lowClusters, numLowClusters);
    inevFile.readNativeArray(lowVectors, numLowVectors);
    inevFile.readArray(lowQuads, numLowQuads);
}

CollisionData::CollisionData()
{
    highIndexToVert0 = nullptr;
    highClusters = nullptr;
    lowClusters = nullptr;
    lowVectors = nullptr;
    lowQuads = nullptr;
}

CollisionData::~CollisionData()
{
    delete[] highIndexToVert0;
    delete[] highClusters;
    delete[] lowClusters;
    delete[] lowVectors;
    delete[] lowQuads;
}
