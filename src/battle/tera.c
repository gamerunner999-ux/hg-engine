#include "nitro.h"
#include "battle.h"
#include "poke_party.h"
#include "pokemon.h"
#include "battle_system.h"
#include "global.h"

// ============================================================================
// Terastallization UI Constants
// ============================================================================
#define TERA_ICON_FIGHT_GFX      (143) 
#define TERA_ICON_SELECTED_GFX   (144) 
#define TERA_ICON_BLANK_GFX      (146) 

#define TERA_ICON_SPRITE_TAG     (22060)
#define TERA_ICON_PAL_TAG        (22061)
#define TERA_ICON_CELL_TAG       (22062)
#define TERA_ICON_CELL_ANIM_TAG  (22063)
#define TERA_BUTTON_SPRITE_TAG   (22064)
#define TERA_BUTTON_PAL_TAG      (22065)

/**
 * @brief Checks if the active Pokémon is eligible to Terastallize this turn.
 */
BOOL CheckCanDrawTeraButton(struct BI_PARAM *bip)
{
    void *pp;
    u16 item;
    u16 mon;
    u16 moves[4];

#ifndef DEBUG_ENABLE_ALL_GIMMICKS
    // Ensure the story flag for the Tera Orb is active
    if (!CheckScriptFlag(FLAG_TERASTALLIZATION_ENABLED)) {
        return FALSE;
    }
#endif

    // Double battle safety: if player already selected Tera for the first slot
    if (bip->client_no && newBS.playerWantTera) 
    {
        return FALSE;
    }

    // Single-use check: has the player already used their Tera charge this battle?
    if (newBS.PlayerTeraed)
    {
        return FALSE;
    }

    // Hardware/Emulator safety check
    if (IS_NOT_VALID_EWRAM_POINTER(&bip->bw->opponentData[bip->client_no])) 
    {
        return FALSE;
    }

    pp = BattleWorkPokemonParamGet(bip->bw, bip->client_no, bip->sel_mons_no);
    mon = GetMonData(pp, MON_DATA_SPECIES, NULL);
    item = GetMonData(pp, MON_DATA_HELD_ITEM, NULL);
    
    for (int i = 0; i < 4; i++) {
        moves[i] = GetMonData(pp, MON_DATA_MOVE1+i, NULL);
    }

    // --- GIMMICK CONFLICT CHECK ---
    // If holding a valid Mega Stone or knowing Dragon Ascent, hide the Tera button
    if (CheckMegaData(mon, item) || CheckMegaMoveData(mon, moves))
    {
        return FALSE;
    }

    // Prevent Terastallization if transformed or if already Terastallized
    if ((bip->bw->sp->battlemon[bip->client_no].condition2 & STATUS2_TRANSFORMED) 
     || bip->bw->sp->battlemon[bip->client_no].is_currently_terastallized)
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Processes the touch screen input when the player interacts with the Tera button.
 */
BOOL CheckTeraButton(struct BI_PARAM *bip, int tp_ret)
{
    void *csp;
    void *crp;
    void *pfd;
    int iconindex = TERA_ICON_SELECTED_GFX;
    int palindex = TERA_ICON_SELECTED_GFX + 1;

    if (tp_ret != 5) 
        return 0;
    if (newBS.ChangeBgFlag) 
        return 0;
    if (!newBS.CanTera) 
        return 0;
    if (newBS.PlayerTeraed) 
        return 0;

    csp = BattleWorkCATS_SYS_PTRGet(bip->bw);
    crp = BattleWorkCATS_RES_PTRGet(bip->bw);
    pfd = BattleWorkPfdGet(bip->bw);

    // Wipe old assets from Sub-VRAM before re-drawing
    OAM_FreeResourcePltt(crp, TERA_BUTTON_PAL_TAG);
    OAM_FreeResourceChar(crp, TERA_BUTTON_SPRITE_TAG);

    // Toggle selected state
    if (newBS.TeraIconLight)
    {
        iconindex = TERA_ICON_BLANK_GFX;
        palindex = TERA_ICON_BLANK_GFX + 1;
        newBS.TeraIconLight = 0;
    }
    else
    {
        newBS.TeraIconLight = 1;
    }

    // Load the updated asset configuration into memory
    OAM_LoadResourceCharArc(csp, crp, ARC_BATTLE_GFX, iconindex, 0, NNS_G2D_VRAM_TYPE_2DSUB, TERA_BUTTON_SPRITE_TAG);
    OAM_LoadResourcePlttWorkArc(pfd, FADE_SUB_OBJ, csp, crp, ARC_BATTLE_GFX, palindex, 0, 1, NNS_G2D_VRAM_TYPE_2DSUB, TERA_BUTTON_PAL_TAG);
    
    OAM_ObjectUpdate(newBS.TeraButton->act);
    Snd_SePlay(1501); // Play menu click sound

    // Force menu interface alignment
    bip->scrn_offset = MoveSelectScreenOffsets[0];
    bip->scrn_range = &MoveSelectButtonScreenRectangle[0];
    bip->scrnbuf_no = 3;
    bip->tp_ret = RECT_HIT_NONE;
    bip->obj_del = FALSE;
    newBS.ChangeBgFlag = 1;

    return 1;
}