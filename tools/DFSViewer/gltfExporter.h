#pragma once

#include <QString>

class RigidGeom;
class SkinGeom;

void exportGLTF(RigidGeom& rigidGeom, QString fileName);
void exportGLTF(SkinGeom& geom, QString fileName);
