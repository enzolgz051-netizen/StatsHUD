#include "pch.h"
#include "StatsHUD.h"

BAKKESMOD_PLUGIN(StatsHUD, "Stats HUD", "1.0.0", 0)

void StatsHUD::onLoad()
{
    cvarEnabled = std::make_shared<bool>(true);
    cvarScale   = std::make_shared<float>(1.0f);

    cvarManager->registerCvar("statshud_enabled", "1",   "Enable Stats HUD", true, true, 0, true, 1).bindTo(cvarEnabled);
    cvarManager->registerCvar("statshud_scale",   "1.0", "HUD scale", true, true, 0.5f, true, 3.0f).bindTo(cvarScale);

    gameWrapper->RegisterDrawable(std::bind(&StatsHUD::Render, this, std::placeholders::_1));

    gameWrapper->HookEvent("Function TAGame.Team_TA.ScorePoint",                  [this](std::string n){ OnGoalScored(n); });
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", [this](std::string n){ OnMatchEnded(n); });
    gameWrapper->HookEvent("Function GameEvent_TA.Countdown.BeginState",          [this](std::string n){ OnMatchStarted(n); });

    cvarManager->log("StatsHUD loaded!");
}

void StatsHUD::onUnload() { cvarManager->log("StatsHUD unloaded."); }

void StatsHUD::OnMatchStarted(std::string eventName)
{
    inMatch = true;
    if (!gameWrapper->IsInOnlineGame()) return;
    auto game = gameWrapper->GetOnlineGame(); if (game.IsNull()) return;
    auto car  = gameWrapper->GetLocalCar();   if (car.IsNull())  return;
    auto pri  = car.GetPRI();                 if (pri.IsNull())  return;
    isOrange  = (pri.GetTeamNum() == 1);
}

void StatsHUD::OnGoalScored(std::string eventName) {}

void StatsHUD::OnMatchEnded(std::string eventName)
{
    if (!inMatch) return;
    inMatch = false;
    if (!gameWrapper->IsInOnlineGame()) return;
    auto game  = gameWrapper->GetOnlineGame(); if (game.IsNull()) return;
    auto teams = game.GetTeams();              if (teams.Count() < 2) return;

    int myScore    = isOrange ? teams.Get(1).GetScore() : teams.Get(0).GetScore();
    int theirScore = isOrange ? teams.Get(0).GetScore() : teams.Get(1).GetScore();
    bool won = (myScore > theirScore);

    if (won) { winStreak++; lossStreak = 0; totalWins++; }
    else     { lossStreak++; winStreak = 0; totalLosses++; }
    lastGameWon = won;
}

// ── Shape : gauche arrondi, droite concave (suit cercle boost) ─────────────────
static void DrawHUDShape(CanvasWrapper& canvas,
                          int px, int py, int pw, int ph,
                          int bcx, int bcy, int br, int lr,
                          unsigned char cr, unsigned char cg,
                          unsigned char cb, unsigned char ca)
{
    canvas.SetColor(cr, cg, cb, ca);
    for (int i = 0; i < ph; i++) {
        int y = py + i;
        int xL = px;
        if (i < lr) {
            int d = lr - i;
            xL = px + lr - (int)sqrtf((float)(lr*lr - d*d));
        } else if (i >= ph - lr) {
            int d = lr - (ph - 1 - i);
            xL = px + lr - (int)sqrtf((float)(lr*lr - d*d));
        }
        float dy = (float)(y - bcy);
        int xR = px + pw;
        if (fabsf(dy) < (float)br) {
            float dx = sqrtf((float)(br*br) - dy*dy);
            int edge = bcx - (int)dx;
            if (edge < xR) xR = edge;
        }
        int lw = xR - xL;
        if (lw > 0) {
            canvas.SetPosition(Vector2{ xL, y });
            canvas.FillBox(Vector2{ lw, 1 });
        }
    }
}

// ── Texte avec ombre portée ────────────────────────────────────────────────────
static void DrawTextShadow(CanvasWrapper& canvas,
                            const std::string& text,
                            int x, int y, float sx, float sy,
                            unsigned char r, unsigned char g,
                            unsigned char b, unsigned char a)
{
    canvas.SetColor((unsigned char)0,(unsigned char)0,(unsigned char)0,(unsigned char)180);
    canvas.SetPosition(Vector2{ x+2, y+2 });
    canvas.DrawString(text, sx, sy, false);
    canvas.SetColor(r, g, b, a);
    canvas.SetPosition(Vector2{ x, y });
    canvas.DrawString(text, sx, sy, false);
}

// ── Render ────────────────────────────────────────────────────────────────────
void StatsHUD::Render(CanvasWrapper canvas)
{
    if (!*cvarEnabled) return;

    float s  = *cvarScale;
    auto  sz = canvas.GetSize();
    float W  = (sz.X > 100) ? (float)sz.X : 1920.f;
    float H  = (sz.Y > 100) ? (float)sz.Y : 1080.f;
    // Debug : affiche la taille écran
    static int debugCounter = 0;
    if (debugCounter++ % 300 == 0) {
        cvarManager->log("Screen: " + std::to_string((int)W) + "x" + std::to_string((int)H));
    }

    // ── Position automatique du boost natif RL (ratio fixe peu importe la résolution) ──
    // Le boost RL est toujours à 30.73% depuis la droite et 33.33% depuis le bas
    // Le rayon est toujours ~6.67% de la hauteur de l'écran
    int bcx = (int)(W - 0.3135f * W);
    int bcy = (int)(H - 0.3630f * H);
    int br  = (int)(0.0880f * H);

    int pw  = (int)(215.f * s);
    int ph  = (int)(125.f * s);
    int lr  = (int)(14.f  * s);
    int px  = bcx - br - pw;
    int py  = bcy - ph / 2;

    // Ombre
    DrawHUDShape(canvas, px+3,py+5, pw,ph, bcx,bcy,br,lr, 0,0,0,80);
    // Fond
    DrawHUDShape(canvas, px,py, pw,ph, bcx,bcy,br,lr, 10,13,20,230);
    // Bordure
    DrawHUDShape(canvas, px,  py,   pw,  ph,   bcx,bcy,br,lr, 70,75,100,110);
    DrawHUDShape(canvas, px+1,py+1, pw-2,ph-2, bcx,bcy,br,lr, 10,13,20, 230);
    // Barre orange top
    DrawHUDShape(canvas, px,py, pw,(int)(4.f*s), bcx,bcy,br,lr, 255,155,15,255);

    // ── Layout ────────────────────────────────────────────────────────────────
    int pad  = (int)(15.f * s);
    int rowH = (int)(35.f * s);
    float lS = 1.15f * s;
    float vS = 1.65f * s;

    int r1 = py + (int)(13.f * s);
    int r2 = r1 + rowH;
    int r3 = r2 + rowH;
    int lx = px + pad;
    int vx = px + pw - (int)(30.f * s);

    // Dividers
    canvas.SetColor((unsigned char)45,(unsigned char)50,(unsigned char)68,(unsigned char)220);
    canvas.SetPosition(Vector2{ lx, r2-(int)(3.f*s) }); canvas.FillBox(Vector2{ (int)(pw*0.68f), 1 });
    canvas.SetPosition(Vector2{ lx, r3-(int)(3.f*s) }); canvas.FillBox(Vector2{ (int)(pw*0.68f), 1 });

    // WINS
    DrawTextShadow(canvas, "WINS", lx, r1, lS, lS, 210,215,230,255);
    std::string wStr = std::to_string(totalWins);
    DrawTextShadow(canvas, wStr, vx-(int)(wStr.size()*15.f*s), r1-(int)(3.f*s), vS,vS, 50,235,110,255);

    // LOSSES
    DrawTextShadow(canvas, "LOSSES", lx, r2, lS, lS, 210,215,230,255);
    std::string lStr = std::to_string(totalLosses);
    DrawTextShadow(canvas, lStr, vx-(int)(lStr.size()*15.f*s), r2-(int)(3.f*s), vS,vS, 255,60,60,255);

    // STREAK
    DrawTextShadow(canvas, "STREAK", lx, r3, lS, lS, 210,215,230,255);
    std::string sStr; unsigned char sr,sg,sb;
    if      (winStreak  > 0) { sStr="+"+std::to_string(winStreak);  sr=255;sg=210;sb=25;  }
    else if (lossStreak > 0) { sStr="-"+std::to_string(lossStreak); sr=255;sg=60; sb=60;  }
    else                     { sStr="-"; sr=100;sg=105;sb=120; }
    DrawTextShadow(canvas, sStr, vx-(int)(sStr.size()*15.f*s), r3-(int)(3.f*s), vS,vS, sr,sg,sb,255);

    // by MielCarbo
    DrawTextShadow(canvas, "by MielCarbo", px+(int)(8.f*s), py+ph+(int)(5.f*s),
                   0.85f*s, 0.85f*s, 180,185,210,230);
}
