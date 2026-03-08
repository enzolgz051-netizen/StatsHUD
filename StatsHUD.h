#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"

class StatsHUD : public BakkesMod::Plugin::BakkesModPlugin
{
public:
    void onLoad() override;
    void onUnload() override;

private:
    void Render(CanvasWrapper canvas);
    void OnGoalScored(std::string eventName);
    void OnMatchEnded(std::string eventName);
    void OnMatchStarted(std::string eventName);

    int winStreak   = 0;
    int lossStreak  = 0;
    int totalWins   = 0;
    int totalLosses = 0;
    bool lastGameWon = false;
    bool inMatch  = false;
    bool isOrange = false;

    std::shared_ptr<bool>  cvarEnabled;
    std::shared_ptr<float> cvarScale;
    std::shared_ptr<float> cvarOffsetX;  // décalage X depuis bord droit
    std::shared_ptr<float> cvarOffsetY;  // décalage Y depuis bord bas
    std::shared_ptr<float> cvarBoostR;   // rayon du cercle boost
};
