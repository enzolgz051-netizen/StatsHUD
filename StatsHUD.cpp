#include "pch.h"
#include "StatsHUD.h"

BAKKESMOD_PLUGIN(StatsHUD, "Stats HUD", "1.0.0", 0)

void StatsHUD::onLoad()
{
    cvarEnabled = std::make_shared<bool>(true);
    cvarScale   = std::make_shared<float>(1.0f);

    cvarManager->registerCvar("statshud_enabled", "1", "Enable Stats HUD", true, true, 0, true, 1)
        .bindTo(cvarEnabled);
    cvarManager->registerCvar("statshud_scale", "1.0", "HUD scale", true, true, 0.5f, true, 3.0f)
        .bindTo(cvarScale);

    gameWrapper->RegisterDrawable(std::bind(&StatsHUD::Render, this, std::placeholders::_1));

    gameWrapper->HookEvent("Function TAGame.Team_TA.ScorePoint",
        [this](std::string name) { OnGoalScored(name); });
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
        [this](std::string name) { OnMatchEnded(name); });
    gameWrapper->HookEvent("Function GameEvent_TA.Countdown.BeginState",
        [this](std::string name) { OnMatchStarted(name); });

    cvarManager->log("StatsHUD loaded!");
}

void StatsHUD::onUnload()
{
    cvarManager->log("StatsHUD unloaded.");
}

float StatsHUD::GetBoostAmount()
{
    if (!gameWrapper->IsInGame() && !gameWrapper->IsInOnlineGame() && !gameWrapper->IsInFreeplay()) return 0.f;
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
    auto game = gameWrapper->GetOnlineGame();
    if (game.IsNull()) return;
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

    if (won) { winStreak++; lossStreak = 0; totalWins++; }
    else     { lossStreak++; winStreak = 0; totalLosses++; }
    lastGameWon = won;
}

// ─────────────────────────────────────────────
//  RENDER
// ─────────────────────────────────────────────
void StatsHUD::Render(CanvasWrapper canvas)
{
    if (!*cvarEnabled) return;

    float scale = *cvarScale;
    auto screenSize = canvas.GetSize();

    float screenW = (screenSize.X > 100) ? (float)screenSize.X : 1920.f;
    float screenH = (screenSize.Y > 100) ? (float)screenSize.Y : 1080.f;

    float panelW  = 230.f * scale;
    float panelH  = 175.f * scale;
    float margin  = 20.f  * scale;
    float padding = 14.f  * scale;

    float panelX = screenW - panelW - margin;
    float panelY = screenH - panelH - margin;

    // ── Shadow
    canvas.SetColor(0, 0, 0, 128);
    canvas.SetPosition(Vector2{ (int)(panelX + 4.f), (int)(panelY + 4.f) });
    canvas.FillBox(Vector2{ (int)panelW, (int)panelH });

    // ── Background
    canvas.SetColor(15, 15, 26, 235);
    canvas.SetPosition(Vector2{ (int)panelX, (int)panelY });
    canvas.FillBox(Vector2{ (int)panelW, (int)panelH });

    // ── Top accent bar
    canvas.SetColor(77, 140, 255, 255);
    canvas.SetPosition(Vector2{ (int)panelX, (int)panelY });
    canvas.FillBox(Vector2{ (int)panelW, (int)(3.f * scale) });

    // ── BOOST label
    float boost = GetBoostAmount();

    canvas.SetColor(140, 153, 191, 255);
    canvas.SetPosition(Vector2{ (int)(panelX + padding), (int)(panelY + padding + 2.f * scale) });
    canvas.DrawString("BOOST", 1.05f * scale, 1.05f * scale, false);

    // Boost value color
    int br, bg, bb;
    if      (boost > 50.f) { br = 51;  bg = 255; bb = 128; }
    else if (boost > 20.f) { br = 255; bg = 209; bb = 26;  }
    else                   { br = 255; bg = 64;  bb = 64;  }

    canvas.SetColor(br, bg, bb, 255);
    canvas.SetPosition(Vector2{ (int)(panelX + padding), (int)(panelY + padding + 17.f * scale) });
    canvas.DrawString(std::to_string((int)boost) + "%", 2.1f * scale, 2.1f * scale, false);

    // Boost bar track
    float barY = panelY + padding + 52.f * scale;
    float barW = panelW - padding * 2.f;
    float barH = 7.f * scale;

    canvas.SetColor(46, 46, 56, 255);
    canvas.SetPosition(Vector2{ (int)(panelX + padding), (int)barY });
    canvas.FillBox(Vector2{ (int)barW, (int)barH });

    canvas.SetColor(br, bg, bb, 255);
    canvas.SetPosition(Vector2{ (int)(panelX + padding), (int)barY });
    canvas.FillBox(Vector2{ (int)(barW * (boost / 100.f)), (int)barH });

    // ── Divider 1
    float div1Y = barY + barH + 10.f * scale;
    canvas.SetColor(64, 64, 89, 178);
    canvas.SetPosition(Vector2{ (int)(panelX + padding), (int)div1Y });
    canvas.FillBox(Vector2{ (int)(panelW - padding * 2.f), 1 });

    // ── WINS / LOSSES
    float wlY = div1Y + 9.f * scale;

    canvas.SetColor(140, 153, 191, 255);
    canvas.SetPosition(Vector2{ (int)(panelX + padding), (int)wlY });
    canvas.DrawString("WINS", 0.95f * scale, 0.95f * scale, false);

    canvas.SetColor(64, 255, 140, 255);
    canvas.SetPosition(Vector2{ (int)(panelX + padding + 44.f * scale), (int)(wlY - 1.f * scale) });
    canvas.DrawString(std::to_string(totalWins), 1.4f * scale, 1.4f * scale, false);

    canvas.SetColor(89, 89, 115, 255);
    canvas.SetPosition(Vector2{ (int)(panelX + panelW * 0.5f - 4.f * scale), (int)wlY });
    canvas.DrawString("|", 1.2f * scale, 1.2f * scale, false);

    canvas.SetColor(140, 153, 191, 255);
    canvas.SetPosition(Vector2{ (int)(panelX + panelW * 0.5f + 6.f * scale), (int)wlY });
    canvas.DrawString("LOSSES", 0.95f * scale, 0.95f * scale, false);

    canvas.SetColor(255, 77, 77, 255);
    canvas.SetPosition(Vector2{ (int)(panelX + panelW - padding - 28.f * scale), (int)(wlY - 1.f * scale) });
    canvas.DrawString(std::to_string(totalLosses), 1.4f * scale, 1.4f * scale, false);

    // ── Divider 2
    float div2Y = wlY + 22.f * scale;
    canvas.SetColor(64, 64, 89, 178);
    canvas.SetPosition(Vector2{ (int)(panelX + padding), (int)div2Y });
    canvas.FillBox(Vector2{ (int)(panelW - padding * 2.f), 1 });

    // ── STREAK
    float streakY = div2Y + 9.f * scale;

    if (winStreak > 0) {
        canvas.SetColor(140, 153, 191, 255);
        canvas.SetPosition(Vector2{ (int)(panelX + padding), (int)streakY });
        canvas.DrawString("WIN STREAK", 0.95f * scale, 0.95f * scale, false);

        canvas.SetColor(255, 209, 26, 255);
        canvas.SetPosition(Vector2{ (int)(panelX + panelW - padding - 36.f * scale), (int)(streakY - 2.f * scale) });
        canvas.DrawString(std::to_string(winStreak) + "W", 1.45f * scale, 1.45f * scale, false);

    } else if (lossStreak > 0) {
        canvas.SetColor(140, 153, 191, 255);
        canvas.SetPosition(Vector2{ (int)(panelX + padding), (int)streakY });
        canvas.DrawString("LOSS STREAK", 0.95f * scale, 0.95f * scale, false);

        canvas.SetColor(255, 77, 77, 255);
        canvas.SetPosition(Vector2{ (int)(panelX + panelW - padding - 36.f * scale), (int)(streakY - 2.f * scale) });
        canvas.DrawString(std::to_string(lossStreak) + "L", 1.45f * scale, 1.45f * scale, false);

    } else {
        canvas.SetColor(115, 115, 128, 255);
        canvas.SetPosition(Vector2{ (int)(panelX + padding), (int)streakY });
        canvas.DrawString("No streak yet", 0.9f * scale, 0.9f * scale, false);
    }

    // ── Branding
    canvas.SetColor(89, 97, 128, 230);
    canvas.SetPosition(Vector2{ (int)(panelX + panelW - padding - 74.f * scale), (int)(panelY + panelH - 14.f * scale) });
    canvas.DrawString("by MielCarbo", 0.75f * scale, 0.75f * scale, false);
}
