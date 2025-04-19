#pragma once

#include "navigation/NavMap.h"
#include "LevelTemplate.h"
#include "VariableManager.h"
#include "../BinLevel.h"

class Level
{
public:
    NavMap navMap;
    VariableManager varMgr;
    BinLevel binLevel;
    LevelTemplate templateMgr;
};
