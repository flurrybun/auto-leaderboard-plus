#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/PlayLayer.hpp>

class $modify(GJGameLevel) {
    void savePercentage(int percent, bool isPracticeMode, int clicks, int attempts, bool isChkValid) {
        auto oldPercent = m_normalPercent.value();
        GJGameLevel::savePercentage(percent, isPracticeMode, clicks, attempts, isChkValid);

        if (percent <= oldPercent) return;

        auto gm = GameManager::sharedState();
        auto glm = GameLevelManager::sharedState();

        auto leaderboardType = gm->getIntGameVariable("0098");
        auto leaderboardMode = gm->getIntGameVariable("0164");

        // using friends and points should cut down the amount of data fetched
        // might help mobile players on cell service but probably doesn't matter
        glm->getLevelLeaderboard(static_cast<GJGameLevel*>(this), LevelLeaderboardType::Weekly, LevelLeaderboardMode::Points);

        // getLevelLeaderboard automatically sets the mode and type to whatever is passed into it, so we have to reset it
        gm->setIntGameVariable("0098", leaderboardType);
        gm->setIntGameVariable("0164", leaderboardMode);
    }
};

class $modify(PlayLayer) {
    // platformer levels don't use GJGameLevel::savePercentage
    void levelComplete() {
        auto oldBestTime = m_level->m_bestTime;
        PlayLayer::levelComplete();

        if (!m_level->isPlatformer() || m_level->m_bestTime >= oldBestTime) return;

        auto gm = GameManager::sharedState();
        auto glm = GameLevelManager::sharedState();

        auto leaderboardType = gm->getIntGameVariable("0098");
        auto leaderboardMode = gm->getIntGameVariable("0164");

        glm->getLevelLeaderboard(static_cast<GJGameLevel*>(m_level), LevelLeaderboardType::Weekly, LevelLeaderboardMode::Time);

        gm->setIntGameVariable("0098", leaderboardType);
        gm->setIntGameVariable("0164", leaderboardMode);
    }
};

class $modify(GameLevelManager) {
    void handleIt(bool idk, gd::string fetchedData, gd::string tag, GJHttpType httpType) {
        GameLevelManager::handleIt(idk, fetchedData, tag, httpType);
        if (httpType != GJHttpType::GetLevelLeaderboard) return;

        // m_storedLevels holds all cached level-related data (levels, comments, leaderboards, etc)
        // m_timerDict holds timestamps for when the data was last fetched
        // deleting these forces a refetch

        m_storedLevels->removeObjectForKey(tag);
        m_timerDict->removeObjectForKey(tag);
    }
};