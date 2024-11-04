#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/PlayLayer.hpp>

class $modify(ModGJGameLevel, GJGameLevel) {
    void updateLeaderboard() {
        auto gm = GameManager::sharedState();
        auto glm = GameLevelManager::sharedState();

        auto leaderboardType = gm->getIntGameVariable("0098");
        auto leaderboardMode = gm->getIntGameVariable("0164");

        // using weekly should cut down the amount of data fetched
        // might help mobile players on cell service but probably doesn't matter

        // if you use LevelLeaderboardMode::Points and beat a platformer level that doesn't have points,
        // gd's servers don't upload your time to the leaderboard
        glm->getLevelLeaderboard(static_cast<GJGameLevel*>(this), LevelLeaderboardType::Weekly, LevelLeaderboardMode::Time);

        // getLevelLeaderboard automatically sets the mode and type to whatever is passed into it, so we have to reset it
        gm->setIntGameVariable("0098", leaderboardType);
        gm->setIntGameVariable("0164", leaderboardMode);
    }
    
    $override
    void savePercentage(int percent, bool isPracticeMode, int clicks, int attempts, bool isChkValid) {
        auto oldPercent = m_normalPercent.value();
        GJGameLevel::savePercentage(percent, isPracticeMode, clicks, attempts, isChkValid);

        if (percent <= oldPercent) return;

        updateLeaderboard();
    }
};

class $modify(PlayLayer) {
    // platformer levels don't use GJGameLevel::savePercentage
    // i wasn't able to find which method saves platformer time, but PlayLayer::levelComplete works
    $override
    void levelComplete() {
        auto oldBestTime = m_level->m_bestTime;
        PlayLayer::levelComplete();

        bool isNewBestTime = m_level->m_bestTime < oldBestTime;
        if (oldBestTime == 0) isNewBestTime = true;

        if (!m_level->isPlatformer() || !isNewBestTime) return;

        static_cast<ModGJGameLevel*>(m_level)->updateLeaderboard();
    }
};

class $modify(GameLevelManager) {
    $override
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