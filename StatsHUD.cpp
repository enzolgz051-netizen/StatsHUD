#include "pch.h"
#include "StatsHUD.h"

BAKKESMOD_PLUGIN(StatsHUD, "Stats HUD", "1.0.0", PLUGINTYPE_FREEPLAY | PLUGINTYPE_CUSTOM_TRAINING | PLUGINTYPE_ONLINE)

void StatsHUD::onLoad()
{
    cvarEnabled = std::make_shared<bool>(true);
    cvarScale   = std::make_shared<float>(1.0f);

    cvarManager->registerCvar("statshud_enabled", "1", "Enable Stats HUD", true, true, 0, true, 1)
        .bindTo(cvarEnabled);
    cvarManager->registerCvar("statshud_scale", "1.0", "HUD scale", true, true, 0.5f, true, 3.0f)
        .bindTo(cvarScale);

    gameWrapper->RegisterDrawable([this](CanvasWrapper canvas) {
        if (*cvarEnabled) Render(canvas);
    });

    gameWrapper->HookEvent("Function TAGame.Team_TA.ScorePoint",
        [this](std::string name) { OnGoalScored(name); });
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
        [this](std::string name) { OnMatchEnded(name); });
    gameWrapper->HookEvent("Function GameEvent_TA.Countdown.BeginState",
        [this](std::string name) { OnMatchStarted(name); });

    LOG("StatsHUD loaded!");
}

void StatsHUD::onUnload()
{
    LOG("StatsHUD unloaded.");
}

float StatsHUD::GetBoostAmount()
{
    if (!gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame()) return 0.f;
    auto car = gameWrapper->GetLocalCar();
    if (car.IsNull()) return 0.f;
    auto boost = car.GetBoostComponent();
    if (boost.IsNull()) return 0.f;
    return boost.GetCurrentBoostAmount() * 100.f;
}

void StatsHUD::OnMatchStarted(std::string eventName)
{
    inMatch = true;
    if (!gameWrapper->IsInOnlineGame()) return;
    auto car = gameWrapper->GetLocalCar();
    if (car.IsNull()) return;
    auto pri = car.GetPRI();
    if (pri.IsNull()) return;
    isOrange = (pri.GetTeamNum() == 1);
}

void StatsHUD::OnGoalScored(std::string eventName) {}

void StatsHUD::OnMatchEnded(std::string eventName)
{
    if (!inMatch) return;
    inMatch = false;
    if (!gameWrapper->IsInOnlineGame()) return;
    auto game = gameWrapper->GetOnlineGame();
    if (game.IsNull()) return;
    auto teams = game.GetTeams();
    if (teams.Count() < 2) return;

    int myScore    = isOrange ? teams.Get(1).GetScore() : teams.Get(0).GetScore();
    int theirScore = isOrange ? teams.Get(0).GetScore() : teams.Get(1).GetScore();
    bool won = (myScore > theirScore);

    if (won) {
        winStreak++;
        lossStreak = 0;
        totalWins++;
    } else {
        lossStreak++;
        winStreak = 0;
        totalLosses++;
    }
    lastGameWon = won;
}

void StatsHUD::Render(CanvasWrapper canvas)
{
    if (!gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame()) return;

    float scale = *cvarScale;
    auto screenSize = canvas.GetSize();

    float panelW  = 230.f * scale;
    float panelH  = 175.f * scale;
    float margin  = 20.f  * scale;
    float padding = 14.f  * scale;

    float panelX = screenSize.X - panelW - margin;
    float panelY = screenSize.Y - panelH - margin;

    // Shadow
    canvas.SetColor(LinearColor{0.f, 0.f, 0.f, 0.5f});
    canvas.SetPosition(Vector2F{panelX + 4.f, panelY + 4.f});
    canvas.FillBox(Vector2F{panelW, panelH});

    // Background
    canvas.SetColor(LinearColor{0.06f, 0.06f, 0.10f, 0.92f});
    canvas.SetPosition(Vector2F{panelX, panelY});
    canvas.FillBox(Vector2F{panelW, panelH});

    // Top accent bar
    canvas.SetColor(LinearColor{0.3f, 0.55f, 1.0f, 1.f});
    canvas.SetPosition(Vector2F{panelX, panelY});
    canvas.FillBox(Vector2F{panelW, 3.f * scale});

    // Boost
    float boost = GetBoostAmount();

    canvas.SetColor(LinearColor{0.55f, 0.6f, 0.75f, 1.f});
    canvas.SetPosition(Vector2F{panelX + padding, panelY + padding + 2.f * scale});
    canvas.DrawString("BOOST", 1.05f * scale, 1.05f * scale, false);

    LinearColor boostColor;
    if      (boost > 50.f) boostColor = LinearColor{0.2f,  1.0f,  0.5f,  1.f};
    else if (boost > 20.f) boostColor = LinearColor{1.0f,  0.82f, 0.1f,  1.f};
    else                   boostColor = LinearColor{1.0f,  0.25f, 0.25f, 1.f};

    canvas.SetColor(boostColor);
    canvas.SetPosition(Vector2F{panelX + padding, panelY + padding + 17.f * scale});
    canvas.DrawString(std::to_string((int)boost) + "%", 2.1f * scale, 2.1f * scale, false);

    float barY = panelY + padding + 52.f * scale;
    float barW = panelW - padding * 2.f;
    float barH = 7.f * scale;

    canvas.SetColor(LinearColor{0.18f, 0.18f, 0.22f, 1.f});
    canvas.SetPosition(Vector2F{panelX + padding, barY});
    canvas.FillBox(Vector2F{barW, barH});

    canvas.SetColor(boostColor);
    canvas.SetPosition(Vector2F{panelX + padding, barY});
    canvas.FillBox(Vector2F{barW * (boost / 100.f), barH});

    // Divider 1
    float div1Y = barY + barH + 10.f * scale;
    canvas.SetColor(LinearColor{0.25f, 0.25f, 0.35f, 0.7f});
    canvas.SetPosition(Vector2F{panelX + padding, div1Y});
    canvas.FillBox(Vector2F{panelW - padding * 2.f, 1.f});

    // Wins / Losses
    float wlY = div1Y + 9.f * scale;

    canvas.SetColor(LinearColor{0.55f, 0.6f, 0.75f, 1.f});
    canvas.SetPosition(Vector2F{panelX + padding, wlY});
    canvas.DrawString("WINS", 0.95f * scale, 0.95f * scale, false);

    canvas.SetColor(LinearColor{0.25f, 1.0f, 0.55f, 1.f});
    canvas.SetPosition(Vector2F{panelX + padding + 44.f * scale, wlY - 1.f * scale});
    canvas.DrawString(std::to_string(totalWins), 1.4f * scale, 1.4f * scale, false);

    canvas.SetColor(LinearColor{0.35f, 0.35f, 0.45f, 1.f});
    canvas.SetPosition(Vector2F{panelX + panelW * 0.5f - 4.f * scale, wlY});
    canvas.DrawString("|", 1.2f * scale, 1.2f * scale, false);

    canvas.SetColor(LinearColor{0.55f, 0.6f, 0.75f, 1.f});
    canvas.SetPosition(Vector2F{panelX + panelW * 0.5f + 6.f * scale, wlY});
    canvas.DrawString("LOSSES", 0.95f * scale, 0.95f * scale, false);

    canvas.SetColor(LinearColor{1.0f, 0.3f, 0.3f, 1.f});
    canvas.SetPosition(Vector2F{panelX + panelW - padding - 28.f * scale, wlY - 1.f * scale});
    canvas.DrawString(std::to_string(totalLosses), 1.4f * scale, 1.4f * scale, false);

    // Divider 2
    float div2Y = wlY + 22.f * scale;
    canvas.SetColor(LinearColor{0.25f, 0.25f, 0.35f, 0.7f});
    canvas.SetPosition(Vector2F{panelX + padding, div2Y});
    canvas.FillBox(Vector2F{panelW - padding * 2.f, 1.f});

    // Streak
    float streakY = div2Y + 9.f * scale;

    if (winStreak > 0) {
        canvas.SetColor(LinearColor{0.55f, 0.6f, 0.75f, 1.f});
        canvas.SetPosition(Vector2F{panelX + padding, streakY});
        canvas.DrawString("WIN STREAK", 0.95f * scale, 0.95f * scale, false);
        canvas.SetColor(LinearColor{1.0f, 0.82f, 0.1f, 1.f});
        canvas.SetPosition(Vector2F{panelX + panelW - padding - 36.f * scale, streakY - 2.f * scale});
        canvas.DrawString(std::to_string(winStreak) + "W", 1.45f * scale, 1.45f * scale, false);
    } else if (lossStreak > 0) {
        canvas.SetColor(LinearColor{0.55f, 0.6f, 0.75f, 1.f});
        canvas.SetPosition(Vector2F{panelX + padding, streakY});
        canvas.DrawString("LOSS STREAK", 0.95f * scale, 0.95f * scale, false);
        canvas.SetColor(LinearColor{1.0f, 0.3f, 0.3f, 1.f});
        canvas.SetPosition(Vector2F{panelX + panelW - padding - 36.f * scale, streakY - 2.f * scale});
        canvas.DrawString(std::to_string(lossStreak) + "L", 1.45f * scale, 1.45f * scale, false);
    } else {
        canvas.SetColor(LinearColor{0.45f, 0.45f, 0.5f, 1.f});
        canvas.SetPosition(Vector2F{panelX + padding, streakY});
        canvas.DrawString("No streak yet", 0.9f * scale, 0.9f * scale, false);
    }

    // by MielCarbo
    canvas.SetColor(LinearColor{0.35f, 0.38f, 0.5f, 0.9f});
    canvas.SetPosition(Vector2F{panelX + panelW - padding - 74.f * scale, panelY + panelH - 14.f * scale});
    canvas.DrawString("by MielCarbo", 0.75f * scale, 0.75f * scale, false);
}
