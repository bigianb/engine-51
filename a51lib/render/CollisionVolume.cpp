#include "CollisionVolume.h"
#include "../InevFile.h"

#include "../streamingOperators.h"

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

    if(inevFile.isPC()){
        // PC version 41 has an extra word here.
        int pad;
        inevFile.read(pad);
    }

    inevFile.readArray(highClusters, numHighClusters);
    inevFile.read(numHighIndices);
    inevFile.readNativeArray(highIndexToVert0, numHighIndices);
    inevFile.read(numLowClusters);
    inevFile.read(numLowVectors);
    inevFile.read(numLowQuads);
    uint16_t pad;
    inevFile.read(pad);
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

void CollisionData::describe(std::ostringstream& ss)
{
    ss << "CollisionData:" << std::endl;
    ss << "--------------" << std::endl;
    ss << "  bbox: " << bbox << std::endl;
    ss << "  numHighClusters: " << numHighClusters << std::endl;
    ss << "  numHighIndices: " << numHighIndices << std::endl;
    ss << "  numLowClusters: " << numLowClusters << std::endl;
    ss << "  numLowVectors: " << numLowVectors << std::endl;
    ss << "  numLowQuads: " << numLowQuads << std::endl;

    for (int i = 0; i < numHighClusters; ++i) {
        ss << "  HighCluster[" << i << "]:" << std::endl;
        highClusters[i].describe(ss);
    }

    for (int i = 0; i < numLowClusters; ++i) {
        ss << "  LowCluster[" << i << "]:" << std::endl;
        lowClusters[i].describe(ss);
    }
}

std::ostream& operator<<(std::ostream& os, const CollisionData::MatInfo& matInfo)
{
    os << "MatInfo: soundType=" << matInfo.soundType
       << ", flags=" << matInfo.flags;
    return os;
}

void CollisionData::HighCluster::describe(std::ostringstream& ss)
{
    ss << "    bbox: " << bbox << std::endl;
    ss << "    nTris: " << nTris << std::endl;
    ss << "    iMesh: " << iMesh << std::endl;
    ss << "    iBone: " << iBone << std::endl;
    ss << "    iDList: " << iDList << std::endl;
    ss << "    iOffset: " << iOffset << std::endl;
    ss << "    materialInfo: " << materialInfo << std::endl;
}

void CollisionData::LowCluster::describe(std::ostringstream& ss)
{
    ss << "    bbox: " << bbox << std::endl;
    ss << "    iVectorOffset: " << iVectorOffset << std::endl;
    ss << "    nPoints: " << nPoints << std::endl;
    ss << "    nNormals: " << nNormals << std::endl;
    ss << "    iQuadOffset: " << iQuadOffset << std::endl;
    ss << "    nQuads: " << nQuads << std::endl;
    ss << "    iMesh: " << iMesh << std::endl;
    ss << "    iBone: " << iBone << std::endl;
}

