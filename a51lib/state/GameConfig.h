#pragma once

class GameConfig
{
public:
    void SetLevelID(int id) { levelId = id; }
    int  GetLevelID() const { return levelId; }

    void commit(const GameConfig& source)
    {
        levelId = source.GetLevelID();
    }

private:
    int levelId;
};
