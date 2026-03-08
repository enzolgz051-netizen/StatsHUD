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

    // ── Dimensions du panel ──
    // Aligné visuellement juste au-dessus du boost natif RL (bas droite)
    // Le boost natif est ~220px depuis la droite, ~160px depuis le bas
    float panelW  = 200.f * scale;
    float panelH  = 80.f  * scale;
    float marginR = 155.f * scale;   // aligne avec le boost natif RL
    float marginB = 175.f * scale;   // juste au-dessus du boost

    float px = screenW - panelW - marginR;
    float py = screenH - panelH - marginB;

    // ── Background semi-transparent ──
    canvas.SetColor(8, 12, 20, 200);
    canvas.SetPosition(Vector2{ (int)px, (int)py });
    canvas.FillBox(Vector2{ (int)panelW, (int)panelH });

    // ── Barre latérale gauche (accent RL style) ──
    canvas.SetColor(0, 180, 255, 255);
    canvas.SetPosition(Vector2{ (int)px, (int)py });
    canvas.FillBox(Vector2{ (int)(3.f * scale), (int)panelH });

    // ── Ligne diagonale décorative haut-droite (style tech RL) ──
    // Simule un angle en dessinant un triangle coupé en haut à droite
    canvas.SetColor(0, 180, 255, 180);
    canvas.SetPosition(Vector2{ (int)(px + panelW - 18.f * scale), (int)py });
    canvas.FillBox(Vector2{ (int)(18.f * scale), (int)(3.f * scale) });
    canvas.SetPosition(Vector2{ (int)(px + panelW - 3.f * scale), (int)py });
    canvas.FillBox(Vector2{ (int)(3.f * scale), (int)(18.f * scale) });

    // ── Ligne diagonale décorative bas-gauche ──
    canvas.SetColor(0, 180, 255, 100);
    canvas.SetPosition(Vector2{ (int)(px + 3.f * scale), (int)(py + panelH - 3.f * scale) });
    canvas.FillBox(Vector2{ (int)(18.f * scale), (int)(3.f * scale) });

    float padding = 12.f * scale;

    // ── WINS ──
    float col1X = px + padding + 3.f * scale;
    float col2X = px + panelW * 0.52f;

    // Label W
    canvas.SetColor(120, 140, 160, 255);
    canvas.SetPosition(Vector2{ (int)col1X, (int)(py + padding) });
    canvas.DrawString("W", 0.85f * scale, 0.85f * scale, false);

    // Valeur wins (vert)
    canvas.SetColor(80, 255, 160, 255);
    canvas.SetPosition(Vector2{ (int)(col1X + 16.f * scale), (int)(py + padding - 2.f * scale) });
    canvas.DrawString(std::to_string(totalWins), 1.3f * scale, 1.3f * scale, false);

    // Label L
    canvas.SetColor(120, 140, 160, 255);
    canvas.SetPosition(Vector2{ (int)col2X, (int)(py + padding) });
    canvas.DrawString("L", 0.85f * scale, 0.85f * scale, false);

    // Valeur losses (rouge)
    canvas.SetColor(255, 80, 80, 255);
    canvas.SetPosition(Vector2{ (int)(col2X + 16.f * scale), (int)(py + padding - 2.f * scale) });
    canvas.DrawString(std::to_string(totalLosses), 1.3f * scale, 1.3f * scale, false);

    // ── Séparateur ──
    canvas.SetColor(0, 180, 255, 60);
    canvas.SetPosition(Vector2{ (int)(px + 3.f * scale), (int)(py + panelH * 0.52f) });
    canvas.FillBox(Vector2{ (int)(panelW - 3.f * scale), (int)(1.f * scale) });

    // ── STREAK ──
    float streakY = py + panelH * 0.55f;

    if (winStreak > 0) {
        // Label
        canvas.SetColor(120, 140, 160, 255);
        canvas.SetPosition(Vector2{ (int)col1X, (int)streakY });
        canvas.DrawString("STREAK", 0.8f * scale, 0.8f * scale, false);
        // Valeur gold
        canvas.SetColor(255, 210, 30, 255);
        canvas.SetPosition(Vector2{ (int)(col1X + 58.f * scale), (int)(streakY - 1.f * scale) });
        canvas.DrawString("+" + std::to_string(winStreak) + "W", 1.1f * scale, 1.1f * scale, false);

    } else if (lossStreak > 0) {
        canvas.SetColor(120, 140, 160, 255);
        canvas.SetPosition(Vector2{ (int)col1X, (int)streakY });
        canvas.DrawString("STREAK", 0.8f * scale, 0.8f * scale, false);
        // Valeur rouge
        canvas.SetColor(255, 80, 80, 255);
        canvas.SetPosition(Vector2{ (int)(col1X + 58.f * scale), (int)(streakY - 1.f * scale) });
        canvas.DrawString("-" + std::to_string(lossStreak) + "L", 1.1f * scale, 1.1f * scale, false);

    } else {
        canvas.SetColor(70, 85, 100, 255);
        canvas.SetPosition(Vector2{ (int)col1X, (int)streakY });
        canvas.DrawString("NO STREAK", 0.8f * scale, 0.8f * scale, false);
    }
}
