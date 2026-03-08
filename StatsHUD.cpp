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

// ── Shape : gauche arrondi convexe, droite arrondi CONCAVE (suit le cercle boost) ──
// Dessine ligne par ligne
static void DrawHUDShape(CanvasWrapper& canvas,
                          int px, int py, int pw, int ph,
                          int boostCX, int boostCY, int boostR,
                          int leftR,
                          int cr, int cg, int cb, int ca)
{
    canvas.SetColor(cr, cg, cb, ca);

    for (int i = 0; i < ph; i++) {
        int y = py + i;

        // Bord gauche arrondi (convexe)
        int xL = px;
        if (i < leftR) {
            int d = leftR - i;
            xL = px + leftR - (int)sqrtf((float)(leftR*leftR - d*d));
        } else if (i >= ph - leftR) {
            int d = leftR - (ph - 1 - i);
            xL = px + leftR - (int)sqrtf((float)(leftR*leftR - d*d));
        }

        // Bord droit concave : suit le bord gauche du cercle boost
        float dy = (float)(y - boostCY);
        int xR = px + pw; // défaut : plat
        if (fabsf(dy) < (float)boostR) {
            float dx = sqrtf((float)((float)boostR * (float)boostR) - dy * dy);
            int circleEdge = boostCX - (int)dx;
            if (circleEdge < xR) xR = circleEdge;
        }

        int lw = xR - xL;
        if (lw > 0) {
            canvas.SetPosition(Vector2{ xL, y });
            canvas.FillBox(Vector2{ lw, 1 });
        }
    }
}

// ── Render ───────────────────────────────────────────────────────────────────
void StatsHUD::Render(CanvasWrapper canvas)
{
    if (!*cvarEnabled) return;

    float s = *cvarScale;
    auto sz = canvas.GetSize();
    float W = (sz.X > 100) ? (float)sz.X : 1920.f;
    float H = (sz.Y > 100) ? (float)sz.Y : 1080.f;

    // Centre et rayon du cercle boost natif RL (1920x1080)
    int boostCX = (int)(W - 118.f * s);
    int boostCY = (int)(H - 100.f * s);
    int boostR  = (int)(100.f * s);

    int panelW  = (int)(220.f * s);
    int panelH  = (int)(120.f * s);
    int leftR   = (int)(16.f  * s);

    // Panel collé au cercle — son bord droit COMMENCE là où le cercle commence
    int panelX = boostCX - boostR - panelW;
    int panelY = boostCY - panelH / 2;

    // Ombre
    DrawHUDShape(canvas, panelX+3, panelY+5, panelW, panelH,
                 boostCX, boostCY, boostR, leftR,
                 0, 0, 0, 75);

    // Fond
    DrawHUDShape(canvas, panelX, panelY, panelW, panelH,
                 boostCX, boostCY, boostR, leftR,
                 10, 13, 20, 228);

    // Bordure subtile
    DrawHUDShape(canvas, panelX,   panelY,   panelW,   panelH,   boostCX, boostCY, boostR, leftR, 70, 75, 100, 100);
    DrawHUDShape(canvas, panelX+1, panelY+1, panelW-2, panelH-2, boostCX, boostCY, boostR, leftR, 10, 13,  20, 228);

    // Barre orange top
    DrawHUDShape(canvas, panelX, panelY, panelW, (int)(4.f*s),
                 boostCX, boostCY, boostR, leftR,
                 255, 155, 15, 255);

    // ── Texte ─────────────────────────────────────────────────────────────────
    int pad    = (int)(16.f * s);
    int rowH   = (int)(33.f * s);
    float lS   = 1.05f * s;
    float vS   = 1.55f * s;

    int row1Y  = panelY + (int)(12.f * s);
    int row2Y  = row1Y + rowH;
    int row3Y  = row2Y + rowH;
    int labelX = panelX + pad;
    int valueX = panelX + panelW - (int)(30.f * s);

    // Dividers
    canvas.SetColor(38, 44, 60, 200);
    canvas.SetPosition(Vector2{ labelX, row2Y - (int)(2.f*s) });
    canvas.FillBox(Vector2{ (int)(panelW * 0.7f), 1 });
    canvas.SetPosition(Vector2{ labelX, row3Y - (int)(2.f*s) });
    canvas.FillBox(Vector2{ (int)(panelW * 0.7f), 1 });

    // WINS
    canvas.SetColor(190, 195, 215, 255);
    canvas.SetPosition(Vector2{ labelX, row1Y });
    canvas.DrawString("WINS", lS, lS, false);
    canvas.SetColor(50, 230, 110, 255);
    std::string wStr = std::to_string(totalWins);
    canvas.SetPosition(Vector2{ valueX - (int)(wStr.size() * 14.f * s), row1Y - (int)(3.f*s) });
    canvas.DrawString(wStr, vS, vS, false);

    // LOSSES
    canvas.SetColor(190, 195, 215, 255);
    canvas.SetPosition(Vector2{ labelX, row2Y });
    canvas.DrawString("LOSSES", lS, lS, false);
    canvas.SetColor(255, 60, 60, 255);
    std::string lStr = std::to_string(totalLosses);
    canvas.SetPosition(Vector2{ valueX - (int)(lStr.size() * 14.f * s), row2Y - (int)(3.f*s) });
    canvas.DrawString(lStr, vS, vS, false);

    // STREAK
    canvas.SetColor(190, 195, 215, 255);
    canvas.SetPosition(Vector2{ labelX, row3Y });
    canvas.DrawString("STREAK", lS, lS, false);

    std::string sStr;
    int sr, sg, sb;
    if (winStreak > 0)       { sStr = "+" + std::to_string(winStreak);  sr=255; sg=205; sb=25; }
    else if (lossStreak > 0) { sStr = "-" + std::to_string(lossStreak); sr=255; sg=60;  sb=60; }
    else                     { sStr = "-"; sr=95; sg=100; sb=118; }

    canvas.SetColor(sr, sg, sb, 255);
    canvas.SetPosition(Vector2{ valueX - (int)(sStr.size() * 14.f * s), row3Y - (int)(3.f*s) });
    canvas.DrawString(sStr, vS, vS, false);

    // by MielCarbo
    canvas.SetColor(50, 55, 70, 180);
    canvas.SetPosition(Vector2{ panelX + panelW - (int)(76.f*s), panelY + panelH + (int)(4.f*s) });
    canvas.DrawString("by MielCarbo", 0.65f*s, 0.65f*s, false);
}
