#pragma once

#include <QString>

class RigidGeom;
class SkinGeom;
class DFSFile;

void exportGLTF(RigidGeom& rigidGeom, QString fileName, DFSFile* dfsFile = nullptr);
void exportGLTF(SkinGeom& geom, QString fileName, DFSFile* dfsFile = nullptr);
