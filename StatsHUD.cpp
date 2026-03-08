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

// ── Rounded box helper ──────────────────────────────────────────────────────
static void DrawRoundedBox(CanvasWrapper& canvas, int x, int y, int w, int h, int r,
                           int cr, int cg, int cb, int ca)
{
    canvas.SetColor(cr, cg, cb, ca);

    // Centre + croix
    canvas.SetPosition(Vector2{ x + r, y });
    canvas.FillBox(Vector2{ w - r * 2, h });
    canvas.SetPosition(Vector2{ x, y + r });
    canvas.FillBox(Vector2{ w, h - r * 2 });

    // 4 coins avec algo cercle
    for (int i = 0; i < r; i++) {
        int hw = (int)sqrtf((float)(r * r - (r - i - 1) * (r - i - 1)));
        canvas.SetPosition(Vector2{ x + r - hw,         y + i });           canvas.FillBox(Vector2{ hw, 1 });
        canvas.SetPosition(Vector2{ x + w - r,          y + i });           canvas.FillBox(Vector2{ hw, 1 });
        canvas.SetPosition(Vector2{ x + r - hw,         y + h - 1 - i });   canvas.FillBox(Vector2{ hw, 1 });
        canvas.SetPosition(Vector2{ x + w - r,          y + h - 1 - i });   canvas.FillBox(Vector2{ hw, 1 });
    }
}

// ── Render ──────────────────────────────────────────────────────────────────
void StatsHUD::Render(CanvasWrapper canvas)
{
    if (!*cvarEnabled) return;

    float s = *cvarScale;
    auto sz = canvas.GetSize();
    float W = (sz.X > 100) ? (float)sz.X : 1920.f;
    float H = (sz.Y > 100) ? (float)sz.Y : 1080.f;

    // ── Positioning ─────────────────────────────────────────────────────────
    // Le boost natif RL est dans le coin bas-droit, environ 210px de large, centré à ~W-105 depuis la droite
    // On se place juste à sa gauche, aligné verticalement

    int boostCenterX = (int)(W - 105.f * s);  // centre approximatif du boost natif
    int boostCenterY = (int)(H - 105.f * s);  // centre vertical du boost natif

    int rowH    = (int)(28.f * s);   // hauteur d'une ligne
    int rowGap  = (int)(6.f  * s);   // espace entre lignes
    int panelW  = (int)(170.f * s);
    int panelH  = rowH * 3 + rowGap * 2 + (int)(16.f * s) * 2; // 3 rows + padding
    int radius  = (int)(10.f * s);

    // Centré verticalement sur le boost, collé à sa gauche
    int panelX = boostCenterX - panelW - (int)(18.f * s);
    int panelY = boostCenterY - panelH / 2;

    // ── Background arrondi ──────────────────────────────────────────────────
    // Ombre portée
    DrawRoundedBox(canvas, panelX + 3, panelY + 4, panelW, panelH, radius,
                   0, 0, 0, 100);
    // Fond principal noir translucide
    DrawRoundedBox(canvas, panelX, panelY, panelW, panelH, radius,
                   12, 14, 18, 215);

    // Fine bordure lumineuse
    DrawRoundedBox(canvas, panelX,     panelY,     panelW,     panelH,     radius, 255, 255, 255, 18);
    DrawRoundedBox(canvas, panelX + 1, panelY + 1, panelW - 2, panelH - 2, radius, 12, 14, 18, 215);

    // ── Données ─────────────────────────────────────────────────────────────
    int pad   = (int)(16.f * s);
    int labelX = panelX + pad;
    int valX   = panelX + panelW - pad;

    int games = totalWins + totalLosses;

    // Ligne 1 : WINS
    int row1Y = panelY + (int)(14.f * s);
    // Ligne 2 : LOSSES
    int row2Y = row1Y + rowH + rowGap;
    // Ligne 3 : STREAK
    int row3Y = row2Y + rowH + rowGap;

    float lScale = 0.82f * s;   // label scale
    float vScale = 1.15f * s;   // value scale

    // ── Accent bar gauche (comme dans l'image de référence) ──
    canvas.SetColor(255, 165, 0, 200);
    canvas.SetPosition(Vector2{ panelX, panelY + radius });
    canvas.FillBox(Vector2{ (int)(3.f * s), panelH - radius * 2 });

    // ── WINS ────────────────────────────────────────────────────────────────
    canvas.SetColor(180, 180, 180, 255);
    canvas.SetPosition(Vector2{ labelX + (int)(4.f * s), row1Y + (int)(4.f * s) });
    canvas.DrawString("WINS", lScale, lScale, false);

    canvas.SetColor(80, 255, 140, 255);
    std::string wStr = std::to_string(totalWins);
    canvas.SetPosition(Vector2{ valX - (int)(wStr.size() * 10.f * s), row1Y + (int)(2.f * s) });
    canvas.DrawString(wStr, vScale, vScale, false);

    // Divider
    canvas.SetColor(50, 55, 65, 200);
    canvas.SetPosition(Vector2{ labelX, row1Y + rowH + (int)(1.f * s) });
    canvas.FillBox(Vector2{ panelW - pad * 2, 1 });

    // ── LOSSES ──────────────────────────────────────────────────────────────
    canvas.SetColor(180, 180, 180, 255);
    canvas.SetPosition(Vector2{ labelX + (int)(4.f * s), row2Y + (int)(4.f * s) });
    canvas.DrawString("LOSSES", lScale, lScale, false);

    canvas.SetColor(255, 75, 75, 255);
    std::string lStr = std::to_string(totalLosses);
    canvas.SetPosition(Vector2{ valX - (int)(lStr.size() * 10.f * s), row2Y + (int)(2.f * s) });
    canvas.DrawString(lStr, vScale, vScale, false);

    // Divider
    canvas.SetColor(50, 55, 65, 200);
    canvas.SetPosition(Vector2{ labelX, row2Y + rowH + (int)(1.f * s) });
    canvas.FillBox(Vector2{ panelW - pad * 2, 1 });

    // ── STREAK ──────────────────────────────────────────────────────────────
    canvas.SetColor(180, 180, 180, 255);
    canvas.SetPosition(Vector2{ labelX + (int)(4.f * s), row3Y + (int)(4.f * s) });
    canvas.DrawString("STREAK", lScale, lScale, false);

    std::string sStr;
    int sr, sg, sb;
    if (winStreak > 0) {
        sStr = "+" + std::to_string(winStreak);
        sr = 255; sg = 205; sb = 30;   // gold
    } else if (lossStreak > 0) {
        sStr = "-" + std::to_string(lossStreak);
        sr = 255; sg = 75; sb = 75;    // rouge
    } else {
        sStr = "0";
        sr = 120; sg = 120; sb = 120;  // gris
    }
    canvas.SetColor(sr, sg, sb, 255);
    canvas.SetPosition(Vector2{ valX - (int)(sStr.size() * 10.f * s), row3Y + (int)(2.f * s) });
    canvas.DrawString(sStr, vScale, vScale, false);

    // ── GAMES (petit, sous tout) ─────────────────────────────────────────────
    if (games > 0) {
        canvas.SetColor(90, 95, 110, 200);
        std::string gStr = std::to_string(games) + " games";
        canvas.SetPosition(Vector2{ panelX + panelW / 2 - (int)(gStr.size() * 4.f * s),
                                    panelY + panelH - (int)(11.f * s) });
        canvas.DrawString(gStr, 0.65f * s, 0.65f * s, false);
    }

    // ── by MielCarbo ─────────────────────────────────────────────────────────
    canvas.SetColor(60, 65, 80, 200);
    canvas.SetPosition(Vector2{ panelX + panelW - (int)(72.f * s), panelY + panelH + (int)(3.f * s) });
    canvas.DrawString("by MielCarbo", 0.6f * s, 0.6f * s, false);
}
