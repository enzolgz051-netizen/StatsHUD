#include "pch.h"
#include "StatsHUD.h"

BAKKESMOD_PLUGIN(StatsHUD, "Stats HUD", "1.0.0", 0)

void StatsHUD::onLoad()
{
    cvarEnabled  = std::make_shared<bool>(true);
    cvarScale    = std::make_shared<float>(1.0f);
    cvarOffsetX  = std::make_shared<float>(590.f);  // px depuis bord droit
    cvarOffsetY  = std::make_shared<float>(360.f);  // px depuis bord bas
    cvarBoostR   = std::make_shared<float>(72.f);   // rayon cercle boost

    cvarManager->registerCvar("statshud_enabled", "1", "Enable Stats HUD", true, true, 0, true, 1).bindTo(cvarEnabled);
    cvarManager->registerCvar("statshud_scale",   "1.0", "HUD scale", true, true, 0.5f, true, 3.0f).bindTo(cvarScale);
    cvarManager->registerCvar("statshud_offset_x","590", "Boost center X from right", true, true, 0, true, 1920).bindTo(cvarOffsetX);
    cvarManager->registerCvar("statshud_offset_y","360", "Boost center Y from bottom", true, true, 0, true, 1080).bindTo(cvarOffsetY);
    cvarManager->registerCvar("statshud_boost_r", "72",  "Boost circle radius", true, true, 10, true, 300).bindTo(cvarBoostR);

    gameWrapper->RegisterDrawable(std::bind(&StatsHUD::Render, this, std::placeholders::_1));

    gameWrapper->HookEvent("Function TAGame.Team_TA.ScorePoint",         [this](std::string n){ OnGoalScored(n); });
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", [this](std::string n){ OnMatchEnded(n); });
    gameWrapper->HookEvent("Function GameEvent_TA.Countdown.BeginState", [this](std::string n){ OnMatchStarted(n); });

    cvarManager->log("StatsHUD loaded! Tune position with: statshud_offset_x / statshud_offset_y / statshud_boost_r");
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

// ── Shape : gauche arrondi, droite concave (suit le cercle boost) ─────────────
static void DrawHUDShape(CanvasWrapper& canvas,
                          int px, int py, int pw, int ph,
                          int bcx, int bcy, int br, int lr,
                          int cr, int cg, int cb, int ca)
{
    canvas.SetColor(cr, cg, cb, ca);
    for (int i = 0; i < ph; i++) {
        int y = py + i;

        // Gauche arrondi convexe
        int xL = px;
        if (i < lr) {
            int d = lr - i;
            xL = px + lr - (int)sqrtf((float)(lr*lr - d*d));
        } else if (i >= ph - lr) {
            int d = lr - (ph - 1 - i);
            xL = px + lr - (int)sqrtf((float)(lr*lr - d*d));
        }

        // Droite concave : suit le bord gauche du cercle boost
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

void StatsHUD::Render(CanvasWrapper canvas)
{
    if (!*cvarEnabled) return;

    float s  = *cvarScale;
    auto  sz = canvas.GetSize();
    float W  = (sz.X > 100) ? (float)sz.X : 1920.f;
    float H  = (sz.Y > 100) ? (float)sz.Y : 1080.f;

    // Coordonnées du boost depuis les CVars (ajustables en jeu via F6)
    int bcx = (int)(W - *cvarOffsetX * s);
    int bcy = (int)(H - *cvarOffsetY * s);
    int br  = (int)(*cvarBoostR * s);

    int pw   = (int)(210.f * s);
    int ph   = (int)(118.f * s);
    int lr   = (int)(14.f  * s);
    int px   = bcx - br - pw;
    int py   = bcy - ph / 2;

    // Ombre
    DrawHUDShape(canvas, px+3,py+5, pw,ph, bcx,bcy,br,lr, 0,0,0,80);
    // Fond
    DrawHUDShape(canvas, px,  py,   pw,ph, bcx,bcy,br,lr, 10,13,20,230);
    // Bordure
    DrawHUDShape(canvas, px,  py,   pw,ph, bcx,bcy,br,lr, 70,75,100,110);
    DrawHUDShape(canvas, px+1,py+1, pw-2,ph-2, bcx,bcy,br,lr, 10,13,20,230);
    // Barre orange top
    DrawHUDShape(canvas, px,py, pw,(int)(4.f*s), bcx,bcy,br,lr, 255,155,15,255);

    // Texte
    int pad=15*s, rowH=33*s;
    float lS=1.05f*s, vS=1.55f*s;
    int r1=py+(int)(12*s), r2=r1+rowH, r3=r2+rowH;
    int lx=px+pad, vx=px+pw-(int)(28*s);

    canvas.SetColor(38,44,60,200);
    canvas.SetPosition(Vector2{lx,r2-(int)(2*s)}); canvas.FillBox(Vector2{(int)(pw*0.68f),1});
    canvas.SetPosition(Vector2{lx,r3-(int)(2*s)}); canvas.FillBox(Vector2{(int)(pw*0.68f),1});

    // WINS
    canvas.SetColor(190,195,215,255); canvas.SetPosition(Vector2{lx,r1}); canvas.DrawString("WINS",lS,lS,false);
    canvas.SetColor(50,230,110,255);
    std::string w=std::to_string(totalWins);
    canvas.SetPosition(Vector2{vx-(int)(w.size()*14*s),r1-(int)(3*s)}); canvas.DrawString(w,vS,vS,false);

    // LOSSES
    canvas.SetColor(190,195,215,255); canvas.SetPosition(Vector2{lx,r2}); canvas.DrawString("LOSSES",lS,lS,false);
    canvas.SetColor(255,60,60,255);
    std::string l=std::to_string(totalLosses);
    canvas.SetPosition(Vector2{vx-(int)(l.size()*14*s),r2-(int)(3*s)}); canvas.DrawString(l,vS,vS,false);

    // STREAK
    canvas.SetColor(190,195,215,255); canvas.SetPosition(Vector2{lx,r3}); canvas.DrawString("STREAK",lS,lS,false);
    std::string ss; int sr,sg,sb;
    if (winStreak>0)       {ss="+"+std::to_string(winStreak);  sr=255;sg=205;sb=25;}
    else if (lossStreak>0) {ss="-"+std::to_string(lossStreak); sr=255;sg=60; sb=60;}
    else                   {ss="-"; sr=95;sg=100;sb=118;}
    canvas.SetColor(sr,sg,sb,255);
    canvas.SetPosition(Vector2{vx-(int)(ss.size()*14*s),r3-(int)(3*s)}); canvas.DrawString(ss,vS,vS,false);

    // by MielCarbo
    canvas.SetColor(50,55,70,180);
    canvas.SetPosition(Vector2{px+pw-(int)(76*s),py+ph+(int)(4*s)});
    canvas.DrawString("by MielCarbo",0.65f*s,0.65f*s,false);
}
