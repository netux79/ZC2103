//--------------------------------------------------------
//  Zelda Classic
//  by Jeremy Craner, 1999-2000
//
//  zc_sys.cpp
//
//  System functions, input handlers, GUI stuff, etc.
//  for Zelda Classic.
//
//--------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef ALLEGRO_DOS
#include <unistd.h>
#endif

#include "zdefs.h"
#include "zelda.h"
#include "tiles.h"
#include "pal.h"
#include "qst.h"
#include "zc_sys.h"
#include "subscr.h"
#include "maps.h"
#include "sprite.h"
#include "guys.h"
#include "link.h"
#include "title.h"
#include "particles.h"

extern LinkClass Link;
extern sprite_list  guys, items, Ewpns, Lwpns, Sitems, chainlinks, decorations,
       particles;

/**********************************/
/******** System functions ********/
/**********************************/

static char cfg_sect[] = "zeldadx";

void load_game_configs()
{
   Akey = get_config_int(cfg_sect, "key_a", KEY_Z);
   Bkey = get_config_int(cfg_sect, "key_b", KEY_X);
   Lkey = get_config_int(cfg_sect, "key_l", KEY_A);
   Rkey = get_config_int(cfg_sect, "key_r", KEY_S);
   Ekey = get_config_int(cfg_sect, "key_select", KEY_ESC);
   Skey = get_config_int(cfg_sect, "key_start", KEY_ENTER);
   Mkey = get_config_int(cfg_sect, "key_map", KEY_SPACE);

   DUkey = get_config_int(cfg_sect, "key_up",   KEY_UP);
   DDkey = get_config_int(cfg_sect, "key_down", KEY_DOWN);
   DLkey = get_config_int(cfg_sect, "key_left", KEY_LEFT);
   DRkey = get_config_int(cfg_sect, "key_right", KEY_RIGHT);

   JoyN = get_config_int(cfg_sect, "joy_idx", 0);
   Abtn = get_config_int(cfg_sect, "joy_a", 0);
   Bbtn = get_config_int(cfg_sect, "joy_b", 1);
   Lbtn = get_config_int(cfg_sect, "joy_l", 6);
   Rbtn = get_config_int(cfg_sect, "joy_r", 7);
   Ebtn = get_config_int(cfg_sect, "joy_select", 10);
   Sbtn = get_config_int(cfg_sect, "joy_start", 11);
   Mbtn = get_config_int(cfg_sect, "joy_map", 3);

   digi_volume = get_config_int(cfg_sect, "sfx", 255);
   midi_volume = get_config_int(cfg_sect, "music", 255);
   pan_style = get_config_int(cfg_sect, "pan", 1);

   Capfps = (bool)get_config_int(cfg_sect, "capfps", 1);
   TransLayers = (bool)get_config_int(cfg_sect, "translayers", 1);
   ShowFPS = (bool)get_config_int(cfg_sect, "showfps", 0);

#ifdef ALLEGRO_DOS
   VidMode = get_config_int(cfg_sect, "vid_mode", GFX_AUTODETECT);
#else
   VidMode = get_config_int(cfg_sect, "vid_mode", GFX_AUTODETECT_FULLSCREEN);
#endif
   resx = get_config_int(cfg_sect, "resx", 320);
   resy = get_config_int(cfg_sect, "resy", 240);
   sbig = get_config_int(cfg_sect, "sbig", 0);
   scanlines = get_config_int(cfg_sect, "scanlines", 0);
   HeartBeep = get_config_int(cfg_sect, "heartbeep", 1);

   strcpy(qstpath, get_config_string(cfg_sect, "qst_path", ""));
}

void save_game_configs()
{
   set_config_int(cfg_sect, "key_a", Akey);
   set_config_int(cfg_sect, "key_b", Bkey);
   set_config_int(cfg_sect, "key_l", Lkey);
   set_config_int(cfg_sect, "key_r", Rkey);
   set_config_int(cfg_sect, "key_select", Ekey);
   set_config_int(cfg_sect, "key_start", Skey);
   set_config_int(cfg_sect, "key_map", Mkey);

   set_config_int(cfg_sect, "key_up",   DUkey);
   set_config_int(cfg_sect, "key_down", DDkey);
   set_config_int(cfg_sect, "key_left", DLkey);
   set_config_int(cfg_sect, "key_right", DRkey);

   set_config_int(cfg_sect, "joy_idx", JoyN);
   set_config_int(cfg_sect, "joy_a", Abtn);
   set_config_int(cfg_sect, "joy_b", Bbtn);
   set_config_int(cfg_sect, "joy_l", Lbtn);
   set_config_int(cfg_sect, "joy_r", Rbtn);
   set_config_int(cfg_sect, "joy_select", Ebtn);
   set_config_int(cfg_sect, "joy_start", Sbtn);
   set_config_int(cfg_sect, "joy_map", Mbtn);

   set_config_int(cfg_sect, "sfx", digi_volume);
   set_config_int(cfg_sect, "music", midi_volume);
   set_config_int(cfg_sect, "pan", pan_style);
   set_config_int(cfg_sect, "capfps", (int)Capfps);
   set_config_int(cfg_sect, "translayers", (int)TransLayers);
   set_config_int(cfg_sect, "showfps", (int)ShowFPS);
   set_config_int(cfg_sect, "vid_mode", VidMode);
   set_config_int(cfg_sect, "resx", resx);
   set_config_int(cfg_sect, "resy", resy);
   set_config_int(cfg_sect, "sbig", sbig);
   set_config_int(cfg_sect, "scanlines", scanlines);
   set_config_string(cfg_sect, "qst_path", qstpath);
   set_config_int(cfg_sect, "heartbeep", HeartBeep);
}

//----------------------------------------------------------------

// Timers

void fps_callback()
{
   lastfps = framecnt;
   avgfps = ((long double)avgfps * fps_secs + lastfps) / (fps_secs + 1);
   ++fps_secs;
   framecnt = 0;
}

END_OF_FUNCTION(fps_callback)

void myvsync_callback()
{
   ++myvsync;
}

END_OF_FUNCTION(myvsync_callback)

void Z_init_timers()
{
   const char *err_str = "Couldn't install timer";

   if (install_timer() < 0)
      Z_error(err_str);

   LOCK_VARIABLE(lastfps);
   LOCK_VARIABLE(framecnt);
   LOCK_FUNCTION(fps_callback);
   if (install_int_ex(fps_callback, SECS_TO_TIMER(1)))
      Z_error(err_str);

   LOCK_VARIABLE(myvsync);
   LOCK_FUNCTION(myvsync_callback);
   if (install_int_ex(myvsync_callback, BPS_TO_TIMER(60)))
      Z_error(err_str);
}

//----------------------------------------------------------------

void show_fps()
{
   char buf[16];

   sprintf(buf, "%2d/60", lastfps);

   if (sbig)
      textout_ex(screen, zfont, buf, scrx + 40 - 120, scry + 216 + 104, 1, -1);

   else
      textout_ex(screen, zfont, buf, scrx + 40, scry + 216, 1, -1);
}

//----------------------------------------------------------------

// sets the video mode and initializes the palette
bool game_vid_mode(int mode, int wait)
{

   request_refresh_rate(60);

#ifdef ALLEGRO_DOS
   switch (mode)
   {
      case GFX_AUTODETECT:
      case GFX_VESA3:
         if (set_gfx_mode(GFX_VESA3, resx, resy, 0, 0) == 0)
         {
            VidMode = GFX_VESA3;
            break;
         }
      case GFX_VESA2L:
         if (set_gfx_mode(GFX_VESA2L, resx, resy, 0, 0) == 0)
         {
            VidMode = GFX_VESA2L;
            break;
         }
      case GFX_VESA2B:
         if (set_gfx_mode(GFX_VESA2B, resx, resy, 0, 0) == 0)
         {
            VidMode = GFX_VESA2B;
            break;
         }
      case GFX_VESA1:
         if (set_gfx_mode(GFX_VESA1, resx, resy, 0, 0) == 0)
         {
            VidMode = GFX_VESA1;
            break;
         }
      case GFX_MODEX:
         if (set_gfx_mode(GFX_MODEX, 320, 240, 0, 0) == 0)
         {
            VidMode = GFX_MODEX;
            resx = 320;
            resy = 240;
            sbig = false;
            break;
         }
      default:
         return false;
         break;
   }
#else
   switch (mode)
   {
      case GFX_AUTODETECT_FULLSCREEN:
         if (set_gfx_mode(GFX_AUTODETECT_FULLSCREEN, resx, resy, 0, 0) == 0)
            VidMode = GFX_AUTODETECT_FULLSCREEN;
         break;
      case GFX_AUTODETECT_WINDOWED:
         if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, resx, resy, 0, 0) == 0)
            VidMode = GFX_AUTODETECT_WINDOWED;
         break;
      default:
         return false;
         break;
   }
#endif
   // wait a bit after setting the video mode
   rest(wait);

   scrx = (resx - 320) >> 1;
   scry = (resy - 240) >> 1;

   rgb_map = &rgb_table;

   clear_to_color(screen, BLACK);

   return true;
}

//----------------------------------------------------------------

int black_opening_count = 0;
int black_opening_x, black_opening_y;

void close_black_opening(int x, int y, bool wait)
{
   black_opening_count = 66;
   black_opening_x = x;
   black_opening_y = y;
   lensclk = 0;

   if (wait)
   {
      for (int i = 0; i < 66; i++)
      {
         draw_screen(tmpscr, 0, 0);
         putsubscr(framebuf, 0, 0);
         syskeys();
         advanceframe();
         if (Status)
            break;
      }
   }
}

void open_black_opening(int x, int y, bool wait)
{
   black_opening_count = -66;
   black_opening_x = x;
   black_opening_y = y;
   lensclk = 0;

   if (wait)
   {
      for (int i = 0; i < 66; i++)
      {
         draw_screen(tmpscr, 0, 0);
         putsubscr(framebuf, 0, 0);
         syskeys();
         advanceframe();
         if (Status)
            break;
      }
   }
}

void black_opening(BITMAP *dest, int x, int y, int a, int max_a)
{
   clear_to_color(tmp_scr, BLACK);
   int w = 256, h = 224;

   double new_w = (w / 2) + abs(w / 2 - x);
   double new_h = (h / 2) + abs(h / 2 - y);
   int r = int(sqrt((new_w * new_w) + (new_h * new_h)) * a / max_a);
   circlefill(tmp_scr, x, y, r, 0);

   masked_blit(tmp_scr, dest, 0, 0, 0, 0, w, h);
}

//----------------------------------------------------------------

bool item_disabled(int item_type,
                   int item)                 //is this item disabled?
{
   return false;
}

bool can_use_item(int item_type,
                  int item)                  //can Link use this item?
{
   if (has_item(item_type, item) && !item_disabled(item_type, item))
      return true;
   return false;
}

bool has_item(int item_type,
              int it)                        //does Link possess this item?
{
   switch (item_type)
   {
      case itype_bomb:
      case itype_sbomb:
         return (game.items[item_type] > 0);
         break;
      case itype_clock:
         return Link.getClock() ? 1 : 0;
      case itype_key:
         return (game.keys > 0);
      case itype_magiccontainer:
         return (game.maxmagic >= MAGICPERBLOCK);
      case itype_triforcepiece:
         //it: -2=any, -1=current level, other=that level
      {
         switch (it)
         {
            case -2:
            {
               for (int i = 0; i < MAXLEVELS; i++)
               {
                  if (game.lvlitems[i] | liTRIFORCE)

                     return true;
               }
               return false;
               break;
            }
            case -1:
               return (game.lvlitems[dlevel] | liTRIFORCE);
               break;
            default:
               if (it >= 0 && it < MAXLEVELS)
                  return (game.lvlitems[it] | liTRIFORCE);
               break;
         }
         return 0;
      }
      case itype_map:
         //it: -2=any, -1=current level, other=that level
      {
         switch (it)
         {
            case -2:
            {
               for (int i = 0; i < MAXLEVELS; i++)
               {
                  if (game.lvlitems[i] | liMAP)
                     return true;
               }
               return false;
            }
            break;
            case -1:
               return (game.lvlitems[dlevel] | liMAP);
               break;
            default:
               if (it >= 0 && it < MAXLEVELS)
                  return (game.lvlitems[it] | liMAP);
               break;
         }
         return 0;
      }
      case itype_compass:
         //it: -2=any, -1=current level, other=that level
      {
         switch (it)
         {
            case -2:
            {
               for (int i = 0; i < MAXLEVELS; i++)
               {
                  if (game.lvlitems[i] | liCOMPASS)
                     return true;
               }
               return false;
               break;
            }
            case -1:
               return (game.lvlitems[dlevel] | liCOMPASS);
               break;
            default:
               if (it >= 0 && it < MAXLEVELS)
                  return (game.lvlitems[it] | liCOMPASS);
               break;
         }
         return 0;
      }
      case itype_bosskey:
         //it: -2=any, -1=current level, other=that level
      {
         switch (it)
         {
            case -2:
            {
               for (int i = 0; i < MAXLEVELS; i++)
               {
                  if (game.lvlitems[i] | liBOSSKEY)
                     return true;
               }
               return false;
               break;
            }
            case -1:
               return (game.lvlitems[dlevel] | liBOSSKEY) ? 1 : 0;
               break;
            default:
               if (it >= 0 && it < MAXLEVELS)
                  return (game.lvlitems[it] | liBOSSKEY) ? 1 : 0;
               break;
         }
         return 0;
      }
      default:
         it = (1 << (it - 1));
         if (item_type >= itype_max)
         {
            Z_message("Error - has_item() exception.");
            return false;
         }
         if (game.items[item_type]&it)
            return true;
         break;
   }
   return false;
}

int high_item(int jmax, int item_type, bool consecutive, int itemcluster,
              bool usecluster)
{

   if (usecluster)
   {
      for (int j = jmax - 1; j > 0; j--)
      {
         if (itemcluster & (1 << (j - 1)))
            return consecutive ? j : (1 << (j - 1));
      }
   }
   else
   {
      for (int j = jmax - 1; j > 0; j--)
      {
         if (can_use_item(item_type, j))

            return consecutive ? j : (1 << (j - 1));
      }
   }

   return 0;
}

int current_item(int item_type,
                 bool consecutive)           //item currently being used
{
   int jmax = 0;
   switch (item_type)
   {
      case itype_sword:
         jmax = imax_sword;
         break;
      case itype_brang:
         jmax = imax_brang;
         break;
      case itype_arrow:
         jmax = imax_arrow;
         break;
      case itype_candle:
         jmax = imax_candle;
         break;
      case itype_whistle:
         jmax = imax_whistle;
         break;
      case itype_bait:
         jmax = imax_bait;
         break;
      case itype_letter:
         jmax = imax_letter;
         break;
      case itype_potion:
         jmax = imax_potion;
         break;
      case itype_wand:
         jmax = imax_wand;
         break;
      case itype_ring:
         jmax = imax_ring;
         break;
      case itype_wallet:
         jmax = imax_wallet;
         break;
      case itype_amulet:
         jmax = imax_amulet;
         break;
      case itype_shield:
         jmax = imax_shield;
         break;
      case itype_bow:
         jmax = imax_bow;
         break;
      case itype_raft:
         jmax = imax_raft;
         break;
      case itype_ladder:
         jmax = imax_ladder;
         break;
      case itype_book:
         jmax = imax_book;
         break;
      case itype_magickey:
         jmax = imax_magickey;
         break;
      case itype_bracelet:
         jmax = imax_bracelet;
         break;
      case itype_flippers:
         jmax = imax_flippers;
         break;
      case itype_boots:
         jmax = imax_boots;
         break;
      case itype_hookshot:
         jmax = imax_hookshot;
         break;
      case itype_lens:
         jmax = imax_lens;
         break;
      case itype_hammer:
         jmax = imax_hammer;
         break;
      case itype_dinsfire:
         jmax = imax_dinsfire;
         break;
      case itype_faroreswind:
         jmax = imax_faroreswind;
         break;
      case itype_nayruslove:
         jmax = imax_nayruslove;
         break;
      case itype_bomb:
      case itype_sbomb:
         return can_use_item(item_type, 1) ? game.items[item_type] : 0;
         break;
      case itype_clock:
         return has_item(itype_clock, 1) ? 1 : 0;
         break;
      case itype_key:
         return game.keys;
      case itype_magiccontainer:
         return game.maxmagic / MAGICPERBLOCK;
      case itype_triforcepiece:
      {
         int count = 0;
         for (int i = 0; i < MAXLEVELS; i++)
            count += (game.lvlitems[i] | liTRIFORCE) ? 1 : 0;
         return 0;
         break;
      }
      case itype_map:
      {
         int count = 0;
         for (int i = 0; i < MAXLEVELS; i++)
            count += (game.lvlitems[i] | liMAP) ? 1 : 0;
         return count;
         break;
      }
      case itype_compass:
      {
         int count = 0;
         for (int i = 0; i < MAXLEVELS; i++)
            count += (game.lvlitems[i] | liCOMPASS) ? 1 : 0;
         return count;
         break;
      }
      case itype_bosskey:
      {
         int count = 0;
         for (int i = 0; i < MAXLEVELS; i++)
            count += (game.lvlitems[i] | liBOSSKEY) ? 1 : 0;
         return count;
         break;
      }
      default:
         return 0;
         break;
   }
   return high_item(jmax, item_type, consecutive, 0, false);
}

int item_tile_mod()
{
   long tile = 0;
   int ret = 0;
   ret = current_item(itype_sword, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iSword].ltm;
         break;
      case 2:
         ret = itemsbuf[iWSword].ltm;
         break;
      case 3:
         ret = itemsbuf[iMSword].ltm;
         break;
      case 4:
         ret = itemsbuf[iXSword].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_brang, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iBrang].ltm;
         break;
      case 2:
         ret = itemsbuf[iMBrang].ltm;
         break;
      case 3:
         ret = itemsbuf[iFBrang].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_arrow, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iArrow].ltm;
         break;
      case 2:
         ret = itemsbuf[iSArrow].ltm;
         break;
      case 3:
         ret = itemsbuf[iGArrow].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_candle, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iBCandle].ltm;
         break;
      case 2:
         ret = itemsbuf[iRCandle].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_whistle, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iWhistle].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;
   ret = current_item(itype_bait, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iBait].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_letter, true);
   switch (ret)
   {
      case 1:
      case 2:
         ret = itemsbuf[iLetter].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_potion, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iBPotion].ltm;
         break;
      case 2:
         ret = itemsbuf[iRPotion].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_wand, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iWand].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_ring, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iBRing].ltm;
         break;
      case 2:
         ret = itemsbuf[iRRing].ltm;
         break;
      case 3:
         ret = itemsbuf[iGRing].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_wallet, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iWallet500].ltm;
         break;
      case 2:
         ret = itemsbuf[iWallet999].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;
   ret = current_item(itype_amulet, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iAmulet].ltm;
         break;
      case 2:
         ret = itemsbuf[iL2Amulet].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_shield, true);
   switch (ret)
   {
      case 1:
         ret = 0;
         break;
      case 2:
         ret = itemsbuf[iShield].ltm;
         break;
      case 3:
         ret = itemsbuf[iMShield].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_bow, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iBow].ltm;
         break;
      case 2:
         ret = itemsbuf[iBow2].ltm;
         break;
      default:
         ret = 0;
         break;

   }
   tile += ret;

   ret = current_item(itype_raft, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iRaft].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_ladder, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iLadder].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_book, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iBook].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;
   ret = current_item(itype_magickey, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iMKey].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_bracelet, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iBracelet].ltm;
         break;
      case 2:
         ret = itemsbuf[iL2Bracelet].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_flippers, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iFlippers].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_boots, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iBoots].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_hookshot, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iHookshot].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_lens, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iLens].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;
   ret = current_item(itype_hammer, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iHammer].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_dinsfire, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iDinsFire].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_faroreswind, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iFaroresWind].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_nayruslove, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iNayrusLove].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_bomb, true);
   switch (ret)
   {
      case 0:
         ret = 0;
         break;
      default:
         ret = itemsbuf[iBombs].ltm;
         break;
   }
   tile += ret;

   ret = current_item(itype_sbomb, true);
   switch (ret)
   {
      case 0:
         ret = 0;
         break;
      default:
         ret = itemsbuf[iSBomb].ltm;
         break;
   }
   tile += ret;

   ret = current_item(itype_clock, true);
   switch (ret)
   {
      case 1:
         ret = itemsbuf[iClock].ltm;
         break;
      default:
         ret = 0;
         break;
   }
   tile += ret;

   ret = current_item(itype_key, true);
   switch (ret)
   {
      case 0:
         ret = 0;
         break;
      default:
         ret = itemsbuf[iKey].ltm;
         break;
   }
   tile += ret;

   ret = current_item(itype_map, true);
   switch (ret)
   {
      case 0:
         ret = 0;
         break;
      default:
         ret = itemsbuf[iMap].ltm;
         break;
   }
   tile += ret;

   ret = current_item(itype_compass, true);
   switch (ret)
   {
      case 0:
         ret = 0;
         break;
      default:
         ret = itemsbuf[iCompass].ltm;
         break;
   }
   tile += ret;

   ret = current_item(itype_bosskey, true);
   switch (ret)
   {
      case 0:
         ret = 0;
         break;
      default:
         ret = itemsbuf[iBossKey].ltm;
         break;
   }
   tile += ret;

   ret = current_item(itype_magiccontainer, true);
   switch (ret)
   {
      case 0:
         ret = 0;
         break;
      default:
         ret = itemsbuf[iMagicC].ltm;
         break;
   }
   tile += ret;

   ret = current_item(itype_triforcepiece, true);
   switch (ret)
   {
      case 0:
         ret = 0;
         break;
      default:
         ret = itemsbuf[iTriforce].ltm;
         break;
   }
   tile += ret;
   return tile;
}

int dmap_tile_mod()
{
   return 0;
}

void draw_lens_under()
{
   int strike_hint_table[11] =
   {
      mfARROW, mfBOMB, mfBRANG, mfWANDMAGIC,
      mfSWORD, mfREFMAGIC, mfHOOKSHOT,
      mfREFFIREBALL, mfHAMMER, mfSWORDBEAM, mfWAND
   };

   {
      int blink_rate = 1;
      int tempitem, tempweapon;
      strike_hint = strike_hint_table[strike_hint_counter];
      if (strike_hint_timer > 32)
      {
         strike_hint_timer = 0;
         strike_hint_counter = ((strike_hint_counter + 1) % 11);
      }
      ++strike_hint_timer;

      for (int i = 0; i < 176; i++)
      {
         int x = (i & 15) << 4;
         int y = (i & 0xF0) + 56;
         int tempitemx = -16, tempitemy = -16;
         int tempweaponx = -16, tempweapony = -16;

         switch (tmpscr->sflag[i])
         {
            case 0:
            case mfZELDA:
            case mfPUSHED:
            case mfENEMY0:
            case mfENEMY1:
            case mfENEMY2:
            case mfENEMY3:
            case mfENEMY4:
            case mfENEMY5:
            case mfENEMY6:

            case mfENEMY7:
            case mfENEMY8:
            case mfENEMY9:
               break;

            case mfPUSHUD:
            case mfPUSHLR:
            case mfPUSH4:
            case mfPUSHU:
            case mfPUSHD:
            case mfPUSHL:
            case mfPUSHR:
            case mfPUSHUDNS:
            case mfPUSHLRNS:
            case mfPUSH4NS:
            case mfPUSHUNS:
            case mfPUSHDNS:
            case mfPUSHLNS:
            case mfPUSHRNS:
            case mfPUSHUDINS:
            case mfPUSHLRINS:
            case mfPUSH4INS:

            case mfPUSHUINS:
            case mfPUSHDINS:
            case mfPUSHLINS:
            case mfPUSHRINS:
               if (lensclk & 16)
                  putcombo(framebuf, x, y, tmpscr->undercombo, tmpscr->undercset);
               if (lensclk & blink_rate)
               {
                  if (get_bit(quest_rules, qr_LENSHINTS))
                  {
                     switch (combobuf[tmpscr->data[i]].type)
                     {
                        case cPUSH_HEAVY:
                        case cPUSH_HW:
                           tempitem = iBracelet;
                           tempitemx = x, tempitemy = y;
                           putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                                    lens_hint_item[tempitem][1], 0);
                           break;
                        case cPUSH_HEAVY2:
                        case cPUSH_HW2:
                           tempitem = iL2Bracelet;
                           tempitemx = x, tempitemy = y;
                           putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                                    lens_hint_item[tempitem][1], 0);
                           break;
                     }
                  }
               }
               break;

            case mfWHISTLE:
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iWhistle;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfFAIRY:
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iFairyMoving;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfBCANDLE:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sBCANDLE],
                        tmpscr->secretcset[sBCANDLE]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iBCandle;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfRCANDLE:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sRCANDLE],
                        tmpscr->secretcset[sRCANDLE]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iRCandle;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfWANDFIRE:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sWANDFIRE],
                        tmpscr->secretcset[sWANDFIRE]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iWand;
                  tempweapon = wFire;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  else
                  {
                     tempweaponx = x;
                     tempweapony = y;
                  }
                  putweapon(framebuf, tempweaponx, tempweapony, tempweapon, 0, up,
                            lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfDINSFIRE:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sDINSFIRE],
                        tmpscr->secretcset[sDINSFIRE]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iDinsFire;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfARROW:
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  putcombo(framebuf, x, y, tmpscr->secretcombo[sARROW],
                           tmpscr->secretcset[sARROW]);
                  tempitem = iArrow;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfSARROW:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sSARROW],
                        tmpscr->secretcset[sSARROW]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iSArrow;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfGARROW:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sGARROW],
                        tmpscr->secretcset[sGARROW]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iGArrow;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfBOMB:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sBOMB], tmpscr->secretcset[sBOMB]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempweapon = wBomb;
                  if (lensclk & blink_rate)
                  {
                     tempweaponx = x;
                     tempweapony = y;
                  }
                  putweapon(framebuf, tempweaponx, tempweapony + lens_hint_weapon[tempweapon][4],
                            tempweapon, 0, up, lens_hint_weapon[tempweapon][0],
                            lens_hint_weapon[tempweapon][1]);
               }
               break;

            case mfSBOMB:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sSBOMB],
                        tmpscr->secretcset[sSBOMB]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempweapon = wSBomb;
                  if (lensclk & blink_rate)
                  {
                     tempweaponx = x;
                     tempweapony = y;
                  }
                  putweapon(framebuf, tempweaponx, tempweapony + lens_hint_weapon[tempweapon][4],
                            tempweapon, 0, up, lens_hint_weapon[tempweapon][0],
                            lens_hint_weapon[tempweapon][1]);
               }
               break;

            case mfARMOS_SECRET:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sSTAIRS],
                        tmpscr->secretcset[sSTAIRS]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
               }
               break;

            case mfBRANG:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sBRANG],
                        tmpscr->secretcset[sBRANG]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iBrang;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfMBRANG:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sMBRANG],
                        tmpscr->secretcset[sMBRANG]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iMBrang;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfFBRANG:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sFBRANG],
                        tmpscr->secretcset[sFBRANG]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iFBrang;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfWANDMAGIC:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sWANDMAGIC],
                        tmpscr->secretcset[sWANDMAGIC]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iWand;
                  tempweapon = wMagic;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  else
                  {
                     tempweaponx = x;
                     tempweapony = y;
                     --lens_hint_weapon[wMagic][4];
                     if (lens_hint_weapon[wMagic][4] < -8)
                        lens_hint_weapon[wMagic][4] = 8;
                  }
                  putweapon(framebuf, tempweaponx, tempweapony + lens_hint_weapon[tempweapon][4],
                            tempweapon, 0, up, lens_hint_weapon[tempweapon][0],
                            lens_hint_weapon[tempweapon][1]);
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfREFMAGIC:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sREFMAGIC],
                        tmpscr->secretcset[sREFMAGIC]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iMShield;
                  tempweapon = ewMagic;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  else
                  {
                     tempweaponx = x;
                     tempweapony = y;
                     if (lens_hint_weapon[ewMagic][2] == up)
                        --lens_hint_weapon[ewMagic][4];

                     else
                        ++lens_hint_weapon[ewMagic][4];
                     if (lens_hint_weapon[ewMagic][4] > 8)
                        lens_hint_weapon[ewMagic][2] = up;
                     if (lens_hint_weapon[ewMagic][4] <= 0)
                        lens_hint_weapon[ewMagic][2] = down;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
                  putweapon(framebuf, tempweaponx, tempweapony + lens_hint_weapon[tempweapon][4],
                            tempweapon, 0, lens_hint_weapon[ewMagic][2], lens_hint_weapon[tempweapon][0],
                            lens_hint_weapon[tempweapon][1]);
               }
               break;

            case mfREFFIREBALL:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sREFFIREBALL],
                        tmpscr->secretcset[sREFFIREBALL]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iMShield;
                  tempweapon = ewFireball;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                     tempweaponx = x;
                     tempweapony = y;
                     ++lens_hint_weapon[ewFireball][3];
                     if (lens_hint_weapon[ewFireball][3] > 8)
                     {
                        lens_hint_weapon[ewFireball][3] = -8;
                        lens_hint_weapon[ewFireball][4] = 8;
                     }
                     if (lens_hint_weapon[ewFireball][3] > 0)
                        ++lens_hint_weapon[ewFireball][4];

                     else
                        --lens_hint_weapon[ewFireball][4];
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
                  putweapon(framebuf, tempweaponx + lens_hint_weapon[tempweapon][3],
                            tempweapony + lens_hint_weapon[ewFireball][4], tempweapon, 0, up,
                            lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
               }
               break;

            case mfSWORD:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sSWORD],
                        tmpscr->secretcset[sSWORD]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iSword;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfWSWORD:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sWSWORD],
                        tmpscr->secretcset[sWSWORD]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iWSword;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfMSWORD:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sMSWORD],
                        tmpscr->secretcset[sMSWORD]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iMSword;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfXSWORD:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sXSWORD],
                        tmpscr->secretcset[sXSWORD]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iXSword;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfSWORDBEAM:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sSWORDBEAM],
                        tmpscr->secretcset[sSWORDBEAM]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iSword;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 1);
               }
               break;

            case mfWSWORDBEAM:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sWSWORDBEAM],
                        tmpscr->secretcset[sWSWORDBEAM]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iWSword;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 2);
               }
               break;

            case mfMSWORDBEAM:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sMSWORDBEAM],
                        tmpscr->secretcset[sMSWORDBEAM]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iMSword;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 3);
               }
               break;

            case mfXSWORDBEAM:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sXSWORDBEAM],
                        tmpscr->secretcset[sXSWORDBEAM]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iXSword;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 4);
               }
               break;

            case mfHOOKSHOT:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sHOOKSHOT],
                        tmpscr->secretcset[sHOOKSHOT]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iHookshot;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfWAND:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sWAND], tmpscr->secretcset[sWAND]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iWand;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfHAMMER:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sHAMMER],
                        tmpscr->secretcset[sHAMMER]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  tempitem = iHammer;
                  if (lensclk & blink_rate)
                  {
                     tempitemx = x;
                     tempitemy = y;
                  }
                  putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                           lens_hint_item[tempitem][1], 0);
               }
               break;

            case mfSTRIKE:
               putcombo(framebuf, x, y, tmpscr->secretcombo[sSTRIKE],
                        tmpscr->secretcset[sSTRIKE]);
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
                  switch (strike_hint)
                  {
                     case mfARROW:
                        tempitem = iArrow;
                        if (lensclk & blink_rate)
                        {
                           tempitemx = x;
                           tempitemy = y;
                        }
                        putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                                 lens_hint_item[tempitem][1], 0);
                        break;
                     case mfBOMB:
                        tempweapon = wBomb;
                        if (lensclk & blink_rate)
                        {
                           tempweaponx = x;
                           tempweapony = y;
                        }
                        putweapon(framebuf, tempweaponx, tempweapony + lens_hint_weapon[tempweapon][4],
                                  tempweapon, 0, up, lens_hint_weapon[tempweapon][0],
                                  lens_hint_weapon[tempweapon][1]);
                        break;
                     case mfBRANG:
                        tempitem = iBrang;
                        if (lensclk & blink_rate)
                        {
                           tempitemx = x;
                           tempitemy = y;
                        }
                        putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                                 lens_hint_item[tempitem][1], 0);
                        break;
                     case mfWANDMAGIC:
                        tempitem = iWand;
                        tempweapon = wMagic;
                        if (lensclk & blink_rate)
                        {
                           tempitemx = x;
                           tempitemy = y;
                        }
                        else
                        {
                           tempweaponx = x;
                           tempweapony = y;
                           --lens_hint_weapon[wMagic][4];
                           if (lens_hint_weapon[wMagic][4] < -8)
                              lens_hint_weapon[wMagic][4] = 8;
                        }
                        putweapon(framebuf, tempweaponx, tempweapony + lens_hint_weapon[wMagic][4],
                                  tempweapon, 0, up, lens_hint_weapon[tempweapon][0],
                                  lens_hint_weapon[tempweapon][1]);
                        putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                                 lens_hint_item[tempitem][1], 0);
                        break;
                     case mfREFMAGIC:
                        tempitem = iMShield;
                        tempweapon = ewMagic;
                        if (lensclk & blink_rate)
                        {
                           tempitemx = x;
                           tempitemy = y;
                        }
                        else
                        {
                           tempweaponx = x;
                           tempweapony = y;
                           if (lens_hint_weapon[ewMagic][2] == up)
                              --lens_hint_weapon[ewMagic][4];

                           else
                              ++lens_hint_weapon[ewMagic][4];
                           if (lens_hint_weapon[ewMagic][4] > 8)
                              lens_hint_weapon[ewMagic][2] = up;
                           if (lens_hint_weapon[ewMagic][4] <= 0)
                              lens_hint_weapon[ewMagic][2] = down;
                        }
                        putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                                 lens_hint_item[tempitem][1], 0);
                        putweapon(framebuf, tempweaponx, tempweapony + lens_hint_weapon[ewMagic][4],
                                  tempweapon, 0, lens_hint_weapon[ewMagic][2], lens_hint_weapon[tempweapon][0],
                                  lens_hint_weapon[tempweapon][1]);
                        break;
                     case mfREFFIREBALL:
                        tempitem = iMShield;
                        tempweapon = ewFireball;
                        if (lensclk & blink_rate)
                        {
                           tempitemx = x;
                           tempitemy = y;
                           tempweaponx = x;
                           tempweapony = y;
                           ++lens_hint_weapon[ewFireball][3];
                           if (lens_hint_weapon[ewFireball][3] > 8)
                           {
                              lens_hint_weapon[ewFireball][3] = -8;
                              lens_hint_weapon[ewFireball][4] = 8;
                           }
                           if (lens_hint_weapon[ewFireball][3] > 0)
                              ++lens_hint_weapon[ewFireball][4];

                           else
                              --lens_hint_weapon[ewFireball][4];
                        }
                        putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                                 lens_hint_item[tempitem][1], 0);
                        putweapon(framebuf, tempweaponx + lens_hint_weapon[ewFireball][3],
                                  tempweapony + lens_hint_weapon[ewFireball][4], tempweapon, 0, up,
                                  lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
                        break;
                     case mfSWORD:
                        tempitem = iSword;
                        if (lensclk & blink_rate)
                        {
                           tempitemx = x;
                           tempitemy = y;
                        }
                        putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                                 lens_hint_item[tempitem][1], 0);
                        break;
                     case mfSWORDBEAM:
                        tempitem = iSword;
                        if (lensclk & blink_rate)
                        {
                           tempitemx = x;
                           tempitemy = y;
                        }
                        putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                                 lens_hint_item[tempitem][1], 1);
                        break;
                     case mfHOOKSHOT:
                        tempitem = iHookshot;
                        if (lensclk & blink_rate)
                        {
                           tempitemx = x;
                           tempitemy = y;
                        }
                        putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                                 lens_hint_item[tempitem][1], 0);
                        break;
                     case mfWAND:
                        tempitem = iWand;
                        if (lensclk & blink_rate)
                        {
                           tempitemx = x;
                           tempitemy = y;
                        }
                        putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                                 lens_hint_item[tempitem][1], 0);
                        break;
                     case mfHAMMER:
                        tempitem = iHammer;
                        if (lensclk & blink_rate)
                        {
                           tempitemx = x;
                           tempitemy = y;
                        }
                        putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                                 lens_hint_item[tempitem][1], 0);
                        break;
                  }
               }
               break;
            case mfARMOS_ITEM:
            case mfDIVE_ITEM:
               if (get_bit(quest_rules, qr_LENSHINTS))
               {
               }
               if (!getmapflag())
                  putitem2(framebuf, x, y, tmpscr->catchall, lens_hint_item[tmpscr->catchall][0],
                           lens_hint_item[tmpscr->catchall][1], 0);
               break;

            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
            case 26:
            case 27:
            case 28:
            case 29:
            case 30:
            case 31:
               putcombo(framebuf, x, y, tmpscr->secretcombo[(tmpscr->sflag[i]) - 16 + 4],
                        tmpscr->secretcset[(tmpscr->sflag[i]) - 16 + 4]);
               break;

            default:
               if (lensclk & 1)
                  rectfill(framebuf, x, y, x + 15, y + 15, WHITE);
               break;
         }
      }

      if (tmpscr->door[0] == dWALK)
         rectfill(framebuf, 120, 16 + 56, 135, 31 + 56, WHITE);

      if (tmpscr->door[1] == dWALK)
         rectfill(framebuf, 120, 144 + 56, 135, 159 + 56, WHITE);

      if (tmpscr->door[2] == dWALK)
         rectfill(framebuf, 16, 80 + 56, 31, 95 + 56, WHITE);

      if (tmpscr->door[3] == dWALK)
         rectfill(framebuf, 224, 80 + 56, 239, 95 + 56, WHITE);

      if (tmpscr->door[0] == dBOMB)
         showbombeddoor(0);

      if (tmpscr->door[1] == dBOMB)
         showbombeddoor(1);

      if (tmpscr->door[2] == dBOMB)
         showbombeddoor(2);

      if (tmpscr->door[3] == dBOMB)
         showbombeddoor(3);

      if (tmpscr->stairx + tmpscr->stairy)
      {
         putcombo(framebuf, tmpscr->stairx, tmpscr->stairy + 56,
                  tmpscr->secretcombo[sSTAIRS], tmpscr->secretcset[sSTAIRS]);
         if (get_bit(quest_rules, qr_LENSHINTS))
         {
            if (tmpscr->flags & fWHISTLE)
            {
               tempitem = iWhistle;
               int tempitemx = -16;
               int tempitemy = -16;
               if (lensclk & (blink_rate / 4))
               {
                  tempitemx = tmpscr->stairx;
                  tempitemy = tmpscr->stairy + 56;
               }
               putitem2(framebuf, tempitemx, tempitemy, tempitem, lens_hint_item[tempitem][0],
                        lens_hint_item[tempitem][1], 0);
            }
         }
      }
   }
}

void draw_lens_over()
{
   clear_to_color(tmp_scr, BLACK);
   circlefill(tmp_scr, LinkX() + 8, LinkY() + 8 + 56, 60, 0);
   circle(tmp_scr, LinkX() + 8, LinkY() + 8 + 56, 62, 0);
   circle(tmp_scr, LinkX() + 8, LinkY() + 8 + 56, 65, 0);
   masked_blit(tmp_scr, framebuf, 0, 56, 0, 56, 256, 168);
}

//----------------------------------------------------------------

void draw_wavy(int amplitude)
{
   BITMAP *wavebuf = create_bitmap_ex(8, 288, 224);
   clear_to_color(wavebuf, 0);
   blit(framebuf, wavebuf, 0, 0, 16, 0, 256, 224);

   int ofs;
   int amp2 = 168;
   int i = frame % amp2;
   for (int j = 0; j < 168; j++)
   {
      ofs = 0;
      if (j & 1)
         ofs = int(sin((double(i + j) * 2 * PI / amp2)) * amplitude);

      else
         ofs -= int(sin((double(i + j) * 2 * PI / amp2)) * amplitude);
      for (int k = 0; k < 256; k++)
         framebuf->line[j + 56][k] = wavebuf->line[j + 56][k + ofs + 16];
   }
   destroy_bitmap(wavebuf);
}

void draw_fuzzy(int fuzz)
// draws from right half of scrollbuf to framebuf
{
   int firstx, firsty, xstep, ystep, i, y, dx, dy;
   byte *start, *si, *di;

   if (fuzz < 1)
      fuzz = 1;

   xstep = 128 % fuzz;
   if (xstep > 0)
      xstep = fuzz - xstep;

   ystep = 112 % fuzz;
   if (ystep > 0)
      ystep = fuzz - ystep;

   firsty = 1;

   for (y = 0; y < 224;)
   {
      start = &(scrollbuf->line[y][256]);

      for (dy = 0; dy < ystep && dy + y < 224; dy++)
      {
         si = start;
         di = &(framebuf->line[y + dy][0]);
         i = xstep;
         firstx = 1;

         for (dx = 0; dx < 256; dx++)
         {
            *(di++) = *si;
            if (++i >= fuzz)
            {
               if (!firstx)
                  si += fuzz;

               else
               {
                  si += fuzz - xstep;
                  firstx = 0;
               }
               i = 0;
            }
         }
      }

      if (!firsty)
         y += fuzz;

      else
      {
         y += ystep;
         ystep = fuzz;
         firsty = 0;
      }
   }
}

void waitvsync()
{
   if (Capfps)
   {
      while (!myvsync)
         rest(1);
   }

   myvsync = 0;
}

void updatescr()
{
   if (!Playing)
      black_opening_count = 0;

   if (black_opening_count < 0)   //shape is opening up
   {
      black_opening(framebuf, black_opening_x, black_opening_y,
                    (66 + black_opening_count), 66);
      ++black_opening_count;
   }
   else if (black_opening_count > 0)     //shape is closing
   {
      black_opening(framebuf, black_opening_x, black_opening_y, black_opening_count,
                    66);
      --black_opening_count;
   }

   waitvsync();

   if (refreshpal)
   {
      refreshpal = false;
      RAMpal[253] = _RGB(0, 0, 0);
      RAMpal[254] = _RGB(63, 63, 63);
      set_palette_range(RAMpal, 0, 255, false);
   }

   if (Link.DrunkClock())
      draw_wavy(Link.DrunkClock() / (MAXDRUNKCLOCK / 32));

   bool nosubscr = (tmpscr->flags3 & fNOSUBSCR);

   if (nosubscr)
   {
      rectfill(tmp_scr, 0, 0, 255, 56 / 2, 0);
      rectfill(tmp_scr, 0, 168 + 56 / 2, 255, 168 + 56 - 1, 0);
      blit(framebuf, tmp_scr, 0, 56, 0, 56 / 2, 256, 224 - 56);
   }

   if (scanlines && sbig)
   {
      BITMAP *scanlinesbmp = create_bitmap_ex(8, 512, 448);
      stretch_blit(nosubscr ? tmp_scr : framebuf, scanlinesbmp, 0, 0, 256, 224, 0, 0,
                   512, 448);
      for (int i = 0; i < 224; ++i)
         hline(scanlinesbmp, 0, i * 2 + 1, 512, BLACK);
      blit(scanlinesbmp, screen, 0, 0, scrx + 32 - 128, scry + 8 - 112, 512, 448);

      destroy_bitmap(scanlinesbmp);
   }
   else if (sbig)
   {
      BITMAP *tempscreen = create_bitmap_ex(8, 512, 448);
      clear_bitmap(tempscreen);
      stretch_blit(nosubscr ? tmp_scr : framebuf, tempscreen, 0, 0, 256, 224, 0, 0,
                   512, 448);
      blit(tempscreen, screen, 0, 0, scrx + 32 - 128, scry + 8 - 112, 512, 448);
      destroy_bitmap(tempscreen);
   }
   else
      blit(nosubscr ? tmp_scr : framebuf, screen, 0, 0, scrx + 32, scry + 8, 256,
           224);

   if (ShowFPS)
      show_fps();

   ++framecnt;
}

//----------------------------------------------------------------

void f_Quit(int type)
{
   music_pause();
   pause_all_sfx();

   Status = type;

   eat_buttons();
}

//----------------------------------------------------------------

void syskeys()
{
   // Update joystick state
   poll_joystick();

   if (ReadKey(KEY_F1))
      Capfps = !Capfps;
   if (ReadKey(KEY_F2))
      ShowFPS = !ShowFPS;
   bool eBtn = rEbtn();
   if ((ReadKey(KEY_F6) || eBtn) && Playing)
      f_Quit(qQUIT);
   if (ReadKey(KEY_F7))
      f_Quit(qRESET);
   if (ReadKey(KEY_ESC) || (eBtn && !Playing))
      f_Quit(qEXIT);

   while (Playing && keypressed())
      readkey();
}

// 99*360 + 59*60
#define MAXTIME  21405240

void advanceframe()
{
   if (Status)
      return;

   if (Playing && game.time < MAXTIME)
      ++game.time;

   ++frame;

   syskeys();
   updatescr();
   sfx_cleanup();
}

void zapout()
{
   // draw screen on right half of scrollbuf
   blit(framebuf, scrollbuf, 0, 0, 256, 0, 256, 224);

   // zap out
   for (int i = 1; i <= 24; i++)
   {
      draw_fuzzy(i);
      syskeys();
      advanceframe();
      if (Status)
         break;
   }
}

void zapin()
{
   // draw screen on right half of scrollbuf
   draw_screen(tmpscr, 0, 0);
   putsubscr(framebuf, 0, 0);
   blit(framebuf, scrollbuf, 0, 0, 256, 0, 256, 224);

   // zap out
   for (int i = 24; i >= 1; i--)
   {
      draw_fuzzy(i);
      syskeys();
      advanceframe();
      if (Status)
         break;
   }
}


void wavyout()
{
   draw_screen(tmpscr, 0, 0);
   putsubscr(framebuf, 0, 0);

   BITMAP *wavebuf = create_bitmap_ex(8, 288, 224);
   clear_to_color(wavebuf, 0);
   blit(framebuf, wavebuf, 0, 0, 16, 0, 256, 224);

   PALETTE wavepal;

   int ofs;
   int amplitude = 8;

   int wavelength = 4;
   double palpos = 0, palstep = 4, palstop = 126;
   for (int i = 0; i < 168; i += wavelength)
   {
      for (int l = 0; l < 256; l++)
      {
         wavepal[l].r = vbound(int(RAMpal[l].r + ((palpos / palstop) *
                                   (63 - RAMpal[l].r))), 0, 63);
         wavepal[l].g = vbound(int(RAMpal[l].g + ((palpos / palstop) *
                                   (63 - RAMpal[l].g))), 0, 63);
         wavepal[l].b = vbound(int(RAMpal[l].b + ((palpos / palstop) *
                                   (63 - RAMpal[l].b))), 0, 63);
      }
      palpos += palstep;
      if (palpos >= 0)
         set_palette(wavepal);

      else
         set_palette(RAMpal);
      for (int j = 0; j < 168; j++)
      {
         for (int k = 0; k < 256; k++)
         {
            ofs = 0;
            if ((j < i) && (j & 1))
               ofs = int(sin((double(i + j) * 2 * PI / 168.0)) * amplitude);
            framebuf->line[j + 56][k] = wavebuf->line[j + 56][k + ofs + 16];
         }
      }
      syskeys();
      advanceframe();

      if (Status)
         break;
   }
   destroy_bitmap(wavebuf);
}

void wavyin()
{
   draw_screen(tmpscr, 0, 0);
   putsubscr(framebuf, 0, 0);

   BITMAP *wavebuf = create_bitmap_ex(8, 288, 224);
   clear_to_color(wavebuf, 0);
   blit(framebuf, wavebuf, 0, 0, 16, 0, 256, 224);

   PALETTE wavepal;
   loadfullpal();
   loadlvlpal(DMaps[currdmap].color);
   ringcolor();
   refreshpal = false;
   int ofs;
   int amplitude = 8;
   int wavelength = 4;
   double palpos = 168, palstep = 4, palstop = 126;
   for (int i = 0; i < 168; i += wavelength)
   {
      for (int l = 0; l < 256; l++)
      {
         wavepal[l].r = vbound(int(RAMpal[l].r + ((palpos / palstop) *
                                   (63 - RAMpal[l].r))), 0, 63);
         wavepal[l].g = vbound(int(RAMpal[l].g + ((palpos / palstop) *
                                   (63 - RAMpal[l].g))), 0, 63);
         wavepal[l].b = vbound(int(RAMpal[l].b + ((palpos / palstop) *
                                   (63 - RAMpal[l].b))), 0, 63);
      }
      palpos -= palstep;

      if (palpos >= 0)
         set_palette(wavepal);

      else
         set_palette(RAMpal);
      for (int j = 0; j < 168; j++)
      {
         for (int k = 0; k < 256; k++)
         {
            ofs = 0;
            if ((j < (167 - i)) && (j & 1))
               ofs = int(sin((double(i + j) * 2 * PI / 168.0)) * amplitude);
            framebuf->line[j + 56][k] = wavebuf->line[j + 56][k + ofs + 16];
         }
      }
      syskeys();
      advanceframe();

      if (Status)
         break;
   }
   destroy_bitmap(wavebuf);
}

void blackscr(int fcnt, bool showsubscr)
{
   reset_pal_cycling();
   while (fcnt > 0)
   {
      clear_bitmap(framebuf);
      if (showsubscr)
         putsubscr(framebuf, 0, 0);
      syskeys();
      advanceframe();
      if (Status)
         break;
      ;
      --fcnt;
   }
}

void openscreen()
{
   reset_pal_cycling();
   black_opening_count = 0;

   if (COOLSCROLL)
   {
      open_black_opening(LinkX() + 8, LinkY() + 8 + 56, true);
      return;
   }
   else
   {
      Link.setDontDraw(true);
      show_subscreen_dmap_dots = false;
      show_subscreen_numbers = false;
      show_subscreen_items = false;
      show_subscreen_life = false;
   }

   int x = 128;

   for (int i = 0; i < 80; i++)
   {
      draw_screen(tmpscr, 0, 0);
      putsubscr(framebuf, 0, 0);
      x = 128 - (((i * 128 / 80) / 8) * 8);
      if (x > 0)
      {
         rectfill(framebuf, 0, 56, x, 223, 0);
         rectfill(framebuf, 256 - x, 56, 255, 223, 0);
      }
      syskeys();
      advanceframe();
      if (Status)
         break;
   }
   Link.setDontDraw(false);
   show_subscreen_items = true;
   show_subscreen_dmap_dots = true;
}

int TriforceCount()
{
   int c = 0;
   for (int i = 1; i <= 8; i++)
      if (game.lvlitems[i]&liTRIFORCE)
         ++c;
   return c;
}

/*
char *key_str[] =
{
  "(none)",         "a",              "b",              "c",
  "d",              "e",              "f",              "g",
  "h",              "i",              "j",              "k",
  "l",              "m",              "n",              "o",
  "p",              "q",              "r",              "s",
  "t",              "u",              "v",              "w",
  "x",              "y",              "z",              "0",
  "1",              "2",              "3",              "4",
  "5",              "6",              "7",              "8",
  "9",              "num 0",          "num 1",          "num 2",
  "num 3",          "num 4",          "num 5",          "num 6",
  "num 7",          "num 8",          "num 9",          "f1",
  "f2",             "f3",             "f4",             "f5",
  "f6",             "f7",             "f8",             "f9",
  "f10",            "f11",            "f12",            "esc",
  "~",              "-",              "=",              "backspace",
  "tab",            "{",              "}",              "enter",
  ":",              "quote",          "\\",             "\\ (2)",
  ",",              ".",              "/",              "space",
  "insert",         "delete",         "home",           "end",
  "page up",        "page down",      "left",           "right",
  "up",             "down",           "num /",          "num *",
  "num -",          "num +",          "num delete",     "num enter",
  "print screen",   "pause",          "abnt c1",        "yen",
  "kana",           "convert",        "no convert",     "at",
  "circumflex",     ": (2)",          "kanji",          "num =",
  "back quote",     ";",              "command",        "unknown (0)",
  "unknown (1)",    "unknown (2)",    "unknown (3)",    "unknown (4)",
  "unknown (5)",    "unknown (6)",    "unknown (7)",    "left shift",
  "right shift",    "left control",   "right control",  "alt",
  "alt gr",         "left win",       "right win",      "menu",
  "scroll lock",    "number lock",    "caps lock",      "MAX"
};
*/

void LogVidMode()
{
   char str_a[44], str_b[44];
#ifdef ALLEGRO_DOS
   switch (VidMode)
   {
      case GFX_MODEX:
         sprintf(str_a, "VGA Mode X");
         break;
      case GFX_VESA1:
         sprintf(str_a, "VESA 1.x");
         break;
      case GFX_VESA2B:
         sprintf(str_a, "VESA2 Banked");
         break;
      case GFX_VESA2L:
         sprintf(str_a, "VESA2 Linear");
         break;
      case GFX_VESA3:
         sprintf(str_a, "VESA3");
         break;
      default:
         sprintf(str_a, "Unknown...");
         break;
   }
#else
   switch (VidMode)
   {
      case GFX_AUTODETECT_FULLSCREEN:
         sprintf(str_a, "Autodetect Fullscreen");
         break;
      case GFX_AUTODETECT_WINDOWED:
         sprintf(str_a, "Autodetect Windowed");
         break;
      default:
         sprintf(str_a, "Unknown...");
         break;
   }
#endif

   sprintf(str_b, "%dx%d %d-bit", resx, resy, get_color_depth());
   Z_message("Video Mode set: %s (%s)\n", str_a, str_b);
}

void color_layer(RGB *src, RGB *dest, char r, char g, char b, char pos,
                 int from, int to)
{
   PALETTE tmp;
   for (int i = 0; i < 256; i++)
   {
      tmp[i].r = r;
      tmp[i].g = g;
      tmp[i].b = b;
   }
   fade_interpolate(src, tmp, dest, pos, from, to);
}

void music_pause()
{
   midi_pause();
}

void music_resume()
{
   midi_resume();
}

void music_stop()
{
   stop_midi();
}

/*****************************/
/**** Custom Sound System ****/
/*****************************/

inline int mixvol(int v1, int v2)
{
   return (min(v1, 255) * min(v2, 255)) >> 8;
}

void jukebox(int index, int loop)
{
   music_stop();
   if (index < 0)
      index = MAXMUSIC - 1;
   if (index >= MAXMUSIC)
      index = 0;

   music_stop();
   set_volume(-1, mixvol(tunes[index].volume, midi_volume >> 1));
   play_midi(tunes[index].midi, loop);
   if (tunes[index].start > 0)
      midi_seek(tunes[index].start);

   midi_loop_end = tunes[index].loop_end;
   midi_loop_start = tunes[index].loop_start;

   currmidi = index;
   master_volume(digi_volume, midi_volume);
}

void jukebox(int index)
{
   if (index < 0)
      index = MAXMUSIC - 1;
   if (index >= MAXMUSIC)
      index = 0;

   // do nothing if it's already playing
   if (index == currmidi && midi_pos >= 0)
      return;

   jukebox(index, tunes[index].loop);
}

void play_DmapMusic()
{
   int m = DMaps[currdmap].midi;
   switch (m)
   {
      case 1:
         jukebox(MUSIC_OVERWORLD);
         break;
      case 2:
         jukebox(MUSIC_DUNGEON);
         break;
      case 3:
         jukebox(MUSIC_LEVEL9);
         break;
      default:
         if (m >= 4 && m < 4 + MAXMIDIS)
            jukebox(m - 4 + MUSIC_COUNT);

         else
            music_stop();
   }
}

void master_volume(int dv, int mv)
{
   if (dv >= 0)
      digi_volume = max(min(dv, 255), 0);
   if (mv >= 0)
      midi_volume = max(min(mv, 255), 0);
   int i = min(max(currmidi, 0), MAXMUSIC - 1);
   set_volume(digi_volume, mixvol(tunes[i].volume, midi_volume));
}

/*****************/
/*****  SFX  *****/
/*****************/

// array of voices, one for each sfx sample in the data file
// 0+ = voice #
// -1 = voice not allocated
static int sfx_voice[WAV_COUNT];

void Z_init_sound()
{
   for (int i = 0; i < WAV_COUNT; i++)
      sfx_voice[i] = -1;
   for (int i = 0; i < MUSIC_COUNT; i++)
      tunes[i].midi = (MIDI *)mididata[i].dat;
   master_volume(digi_volume, midi_volume);
}

// clean up finished samples
void sfx_cleanup()
{
   for (int i = 0; i < WAV_COUNT; i++)
      if (sfx_voice[i] != -1 && voice_get_position(sfx_voice[i]) < 0)
      {
         deallocate_voice(sfx_voice[i]);
         sfx_voice[i] = -1;
      }
}

// allocates a voice for the sample "wav_index" (index into zelda.dat)
// if a voice is already allocated (and/or playing), then it just returns true
// Returns true:  voice is allocated
//         false: unsuccessful
bool sfx_init(int index)
{
   // check index
   if (index < 0 || index >= WAV_COUNT)
      return false;

   if (sfx_voice[index] == -1)
   {
      // allocate voice
      sfx_voice[index] = allocate_voice((SAMPLE *)sfxdata[index].dat);
   }

   return sfx_voice[index] != -1;
}

// plays an sfx sample
void sfx(int index, int pan, bool loop)
{
   if (!sfx_init(index))
      return;

   voice_set_playmode(sfx_voice[index], loop ? PLAYMODE_LOOP : PLAYMODE_PLAY);
   voice_set_pan(sfx_voice[index], pan);

   int pos = voice_get_position(sfx_voice[index]);
   voice_set_position(sfx_voice[index], 0);
   if (pos <= 0)
      voice_start(sfx_voice[index]);
}

// start it (in loop mode) if it's not already playing,
// otherwise just leave it in its current position
void cont_sfx(int index)
{
   if (!sfx_init(index))
      return;

   if (voice_get_position(sfx_voice[index]) <= 0)
   {
      voice_set_position(sfx_voice[index], 0);
      voice_set_playmode(sfx_voice[index], PLAYMODE_LOOP);
      voice_start(sfx_voice[index]);
   }
}

// adjust parameters while playing
void adjust_sfx(int index, int pan, bool loop)
{
   if (index < 0 || index >= WAV_COUNT || sfx_voice[index] == -1)
      return;

   voice_set_playmode(sfx_voice[index], loop ? PLAYMODE_LOOP : PLAYMODE_PLAY);
   voice_set_pan(sfx_voice[index], pan);
}

// pauses a voice
void pause_sfx(int index)
{
   if (index >= 0 && index < WAV_COUNT && sfx_voice[index] != -1)
      voice_stop(sfx_voice[index]);
}

// resumes a voice
void resume_sfx(int index)
{
   if (index >= 0 && index < WAV_COUNT && sfx_voice[index] != -1)
      voice_start(sfx_voice[index]);
}

// pauses all active voices
void pause_all_sfx()
{
   for (int i = 0; i < WAV_COUNT; i++)
      if (sfx_voice[i] != -1)
         voice_stop(sfx_voice[i]);
}

// resumes all paused voices
void resume_all_sfx()
{
   for (int i = 0; i < WAV_COUNT; i++)
      if (sfx_voice[i] != -1)
         voice_start(sfx_voice[i]);
}

// stops an sfx and deallocates the voice
void stop_sfx(int index)
{
   if (index < 0 || index >= WAV_COUNT)
      return;

   if (sfx_voice[index] != -1)
   {
      deallocate_voice(sfx_voice[index]);
      sfx_voice[index] = -1;
   }
}

void kill_sfx()
{
   for (int i = 0; i < WAV_COUNT; i++)
      if (sfx_voice[i] != -1)
      {
         deallocate_voice(sfx_voice[i]);
         sfx_voice[i] = -1;
      }
}

int pan(int x)
{
   switch (pan_style)
   {
      case 0:
         return 128;
      case 1:
         return vbound((x >> 1) + 68, 0, 255);
      case 2:
         return vbound(((x * 3) >> 2) + 36, 0, 255);
   }
   return vbound(x, 0, 255);
}

/*******************************/
/******* Input Handlers ********/
/*******************************/

static bool rButton(bool(proc)(), bool &flag)
{
   if (!proc())
      flag = false;

   else if (!flag)
   {
      flag = true;
      return true;
   }
   return false;
}

bool Up()
{
   return key[DUkey] || joy[JoyN].stick[0].axis[1].d1;
}
bool Down()
{
   return key[DDkey] || joy[JoyN].stick[0].axis[1].d2;
}
bool Left()
{
   return key[DLkey] || joy[JoyN].stick[0].axis[0].d1;
}
bool Right()
{
   return key[DRkey] || joy[JoyN].stick[0].axis[0].d2;
}
bool cAbtn()
{
   return key[Akey] || joybtn(Abtn);
}
bool cBbtn()
{
   return key[Bkey] || joybtn(Bbtn);
}
bool cEbtn()
{
   return key[Ekey] || joybtn(Ebtn);
}
bool cSbtn()
{
   return key[Skey] || joybtn(Sbtn);
}
bool cLbtn()
{
   return key[Lkey] || joybtn(Lbtn);
}
bool cRbtn()
{
   return key[Rkey] || joybtn(Rbtn);
}
bool cMbtn()
{
   return key[Mkey] || joybtn(Mbtn);
}

bool rUp()
{
   return rButton(Up, Udown);
}
bool rDown()
{
   return rButton(Down, Ddown);
}
bool rLeft()
{
   return rButton(Left, Ldown);
}
bool rRight()
{
   return rButton(Right, Rdown);
}
bool rAbtn()
{
   return rButton(cAbtn, Adown);
}
bool rBbtn()
{
   return rButton(cBbtn, Bdown);
}
bool rEbtn()
{
   return rButton(cEbtn, Edown);
}
bool rSbtn()
{
   return rButton(cSbtn, Sdown);
}
bool rLbtn()
{
   return rButton(cLbtn, LBdown);
}
bool rRbtn()
{
   return rButton(cRbtn, RBdown);
}
bool rMbtn()
{
   return rButton(cMbtn, Mdown);
}

bool drunk()
{
   //  return ((!(frame%((rand()%100)+1)))&&(rand()%MAXDRUNKCLOCK<Link.DrunkClock()));
   return false;
}

bool DrunkUp()
{
   return drunk() ? (rand() % 2) ? 0 : !Up() : Up();
}
bool DrunkDown()
{
   return drunk() ? (rand() % 2) ? 0 : !Down() : Down();
}
bool DrunkLeft()
{
   return drunk() ? (rand() % 2) ? 0 : !Left() : Left();
}
bool DrunkRight()
{
   return drunk() ? (rand() % 2) ? 0 : !Right() : Right();
}
bool DrunkcAbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !cAbtn() : cAbtn();
}
bool DrunkcBbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !cBbtn() : cBbtn();
}
bool DrunkcSbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !cSbtn() : cSbtn();
}
bool DrunkcLbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !cLbtn() : cLbtn();
}
bool DrunkcRbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !cRbtn() : cRbtn();
}
bool DrunkcMbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !cMbtn() : cMbtn();
}

bool DrunkrUp()
{
   return drunk() ? (rand() % 2) ? 0 : !rUp() : rUp();
}
bool DrunkrDown()
{
   return drunk() ? (rand() % 2) ? 0 : !rDown() : rDown();
}
bool DrunkrLeft()
{
   return drunk() ? (rand() % 2) ? 0 : !rLeft() : rLeft();
}
bool DrunkrRight()
{
   return drunk() ? (rand() % 2) ? 0 : !rRight() : rRight();
}
bool DrunkrAbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !rAbtn() : rAbtn();
}
bool DrunkrBbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !rBbtn() : rBbtn();
}
bool DrunkrSbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !rSbtn() : rSbtn();
}
bool DrunkrLbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !rLbtn() : rLbtn();
}
bool DrunkrRbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !rRbtn() : rRbtn();
}
bool DrunkrMbtn()
{
   return drunk() ? (rand() % 2) ? 0 : !rMbtn() : rMbtn();
}

void eat_buttons()
{
   rAbtn();
   rBbtn();
   rSbtn();
   rLbtn();
   rRbtn();
   rMbtn();
}

bool ReadKey(int k)
{
   if (key[k])
   {
      key[k] = 0;
      return true;
   }
   return false;
}

bool joybtn(int b)
{
   return joy[JoyN].button[b].b;
}

char *time_str(dword time)
{
   static char s[32];

   dword secs = (time / 60) % 60;
   dword mins = (time / 3600) % 60;
   dword hours = time / 216000;

   sprintf(s, "%d:%02d:%02d", hours, mins, secs);
   return s;
}

int vbound(int x, int low, int high)
{
   if (x < low)
      x = low;
   if (x > high)
      x = high;
   return x;
}

float vbound(float x, float low, float high)
{
   if (x < low)
      x = low;
   if (x > high)
      x = high;
   return x;
}

int used_switch(int argc, char *argv[], const char *s)
{
   // assumes a switch won't be in argv[0]
   for (int i = 1; i < argc; i++)
      if (stricmp(argv[i], s) == 0)
         return i;
   return 0;
}

// Returns the first no switch (-) argv param
char *get_cmd_arg(int argc, char *argv[])
{
   // assumes a switch won't be in argv[0] since it is the exe name.
   for (int i = 1; i < argc; i++)
      if (argv[i][0] != '-')
         return argv[i];
   return NULL;
}

char datapwd[8] = { char('l' + 11), char('o' + 22), char('n' + 33), char('g' + 44), char('t' + 55), char('a' + 66), char('n' + 77), char(0 + 88) };

void resolve_password(char *pwd)
{
   for (int i = 0; i < 8; i++)
      pwd[i] -= (i + 1) * 11;
}

void set_bit(byte *bitstr, int bit, byte val)
{
   bitstr += bit >> 3;
   byte mask = 1 << (bit & 7);

   if (val)
      *bitstr |= mask;

   else
      *bitstr &= ~mask;
}

int get_bit(byte *bitstr, int bit)
{
   bitstr += bit >> 3;
   return ((*bitstr) >> (bit & 7)) & 1;
}

void Z_error(const char *format, ...)
{
   char buf[256];

   va_list ap;
   va_start(ap, format);
   vsprintf(buf, format, ap);
   va_end(ap);

#ifdef ALLEGRO_DOS
   printf("%s", buf);
#endif
   al_trace("%s", buf);
   exit(1);
}

void Z_message(const char *format, ...)
{
   char buf[2048];

   va_list ap;
   va_start(ap, format);
   vsprintf(buf, format, ap);
   va_end(ap);

#ifdef ALLEGRO_DOS
   printf("%s", buf);
#endif
   al_trace("%s", buf);
}

int anim_3_4(int clk, int speed)
{
   clk /= speed;
   switch (clk & 3)
   {
      case 0:
      case 2:
         clk = 0;
         break;
      case 1:
         clk = 1;
         break;
      case 3:
         clk = 2;
         break;
   }
   return clk;
}

/**********  Encryption Stuff  *****************/

static unsigned int seed = 0;
static unsigned int enc_mask[3] = {0x4C358938, 0x91B2A2D1, 0x4A7C1B87};
static unsigned int pvalue[3] = {0x62E9, 0x7D14, 0x1A82};
static unsigned int qvalue[3] = {0x3619, 0xA26B, 0xF03C};

static int rand_007(int method)
{
   short BX = seed >> 8;
   short CX = (seed & 0xFF) << 8;
   char AL = seed >> 24;
   char C = AL >> 7;
   char D = BX >> 15;
   AL <<= 1;
   BX = (BX << 1) | C;
   CX = (CX << 1) | D;
   CX += seed & 0xFFFF;
   BX += (seed >> 16) + C;
   CX += pvalue[method];
   BX += qvalue[method] + D;
   seed = (BX << 16) + CX;
   return (CX << 16) + BX;
}

//
// RETURNS:
//   0 - OK
//   1 - srcfile not opened
//   2 - destfile not opened
//
int encode_file_007(const char *srcfile, const char *destfile, unsigned int key,
                    const char *header, int method)
{
   FILE *src, *dest;
   int tog = 0, c, r = 0;
   short c1 = 0, c2 = 0;

   seed = key;
   src = fopen(srcfile, "rb");
   if (!src)
      return 1;

   dest = fopen(destfile, "wb");
   if (!dest)
   {
      fclose(src);
      return 2;
   }

   // write the header
   if (header)
   {
      for (c = 0; header[c]; c++)
         fputc(header[c], dest);
   }

   // write the key, XORed with MASK
   key ^= enc_mask[method];
   fputc(key >> 24, dest);
   fputc((key >> 16) & 255, dest);
   fputc((key >> 8) & 255, dest);
   fputc(key & 255, dest);

   // encode the data
   while ((c = fgetc(src)) != EOF)
   {
      c1 += c;
      c2 = (c2 << 4) + (c2 >> 12) + c;
      if (tog)
         c += r;

      else
      {
         r = rand_007(method);
         c ^= r;
      }
      tog ^= 1;

      fputc(c, dest);
   }

   // write the checksums
   r = rand_007(method);
   c1 ^= r;
   c2 += r;
   fputc(c1 >> 8, dest);
   fputc(c1 & 255, dest);
   fputc(c2 >> 8, dest);
   fputc(c2 & 255, dest);

   fclose(src);
   fclose(dest);
   return 0;
}

//
// RETURNS:
//   0 - OK
//   1 - srcfile not opened
//   2 - destfile not opened
//   3 - scrfile too small
//   4 - srcfile EOF
//   5 - checksum mismatch
//   6 - header mismatch
//
int decode_file_007(const char *srcfile, const char *destfile,
                    const char *header, int method, bool packed)
{
   FILE *normal_src = NULL, *dest = NULL;
   PACKFILE *packed_src = NULL;
   int tog = 0, c, r = 0, err;
   long size, i;
   short c1 = 0, c2 = 0, check1, check2;

   // open files
#ifdef ALLEGRO_DOS
   size = file_size(srcfile);
#else
   size = file_size_ex(srcfile);
#endif
   if (size < 1)
      return 1;
   size -= 8;                                                // get actual data size, minus key and checksums
   if (size < 1)
      return 3;

   if (!packed)
   {
      normal_src = fopen(srcfile, "rb");
      if (!normal_src)
         return 1;
   }
   else
   {
      packed_src = pack_fopen(srcfile, F_READ_PACKED);
      if (errno == EDOM)
         packed_src = pack_fopen(srcfile, F_READ);
      if (!packed_src)
         return 1;
   }

   dest = fopen(destfile, "wb");
   if (!dest)
   {
      if (packed)
         pack_fclose(packed_src);

      else
         fclose(normal_src);
      return 2;
   }

   // read the header
   err = 4;
   if (header)
   {
      for (i = 0; header[i]; i++)
      {
         if (packed)
         {
            if ((c = pack_getc(packed_src)) == EOF)
               goto error;
         }
         else
         {
            if ((c = fgetc(normal_src)) == EOF)
               goto error;
         }
         if ((c & 255) != header[i])
         {
            err = 6;
            goto error;
         }
         --size;
      }
   }

   // read the key
   if (packed)
   {
      if ((c = pack_getc(packed_src)) == EOF)
         goto error;
   }
   else
   {
      if ((c = fgetc(normal_src)) == EOF)
         goto error;
   }
   seed = c << 24;
   if (packed)
   {
      if ((c = pack_getc(packed_src)) == EOF)
         goto error;
   }
   else
   {
      if ((c = fgetc(normal_src)) == EOF)
         goto error;
   }
   seed += (c & 255) << 16;
   if (packed)
   {
      if ((c = pack_getc(packed_src)) == EOF)
         goto error;
   }
   else
   {
      if ((c = fgetc(normal_src)) == EOF)
         goto error;
   }
   seed += (c & 255) << 8;
   if (packed)
   {
      if ((c = pack_getc(packed_src)) == EOF)
         goto error;
   }
   else
   {
      if ((c = fgetc(normal_src)) == EOF)
         goto error;
   }
   seed += c & 255;
   seed ^= enc_mask[method];

   // decode the data
   for (i = 0; i < size; i++)
   {
      if (packed)
      {
         if ((c = pack_getc(packed_src)) == EOF)
            goto error;
      }
      else
      {
         if ((c = fgetc(normal_src)) == EOF)
            goto error;
      }

      if (tog)
         c -= r;

      else
      {
         r = rand_007(method);
         c ^= r;
      }
      tog ^= 1;

      c &= 255;
      c1 += c;
      c2 = (c2 << 4) + (c2 >> 12) + c;

      fputc(c, dest);
   }

   // read checksums
   if (packed)
   {
      if ((c = pack_getc(packed_src)) == EOF)
         goto error;
   }
   else
   {
      if ((c = fgetc(normal_src)) == EOF)
         goto error;
   }
   check1 = c << 8;
   if (packed)
   {
      if ((c = pack_getc(packed_src)) == EOF)
         goto error;
   }
   else
   {
      if ((c = fgetc(normal_src)) == EOF)
         goto error;
   }
   check1 += c & 255;
   if (packed)
   {
      if ((c = pack_getc(packed_src)) == EOF)
         goto error;
   }
   else
   {
      if ((c = fgetc(normal_src)) == EOF)
         goto error;
   }
   check2 = c << 8;
   if (packed)
   {
      if ((c = pack_getc(packed_src)) == EOF)
         goto error;
   }
   else
   {
      if ((c = fgetc(normal_src)) == EOF)
         goto error;
   }
   check2 += c & 255;

   // verify checksums
   r = rand_007(method);
   check1 ^= r;
   check2 -= r;
   check1 &= 0xFFFF;
   check2 &= 0xFFFF;
   if (check1 != c1 || check2 != c2)
   {
      err = 5;
      goto error;
   }

   if (packed)
      pack_fclose(packed_src);

   else
      fclose(normal_src);
   fclose(dest);
   return 0;

error:
   if (packed)
      pack_fclose(packed_src);

   else
      fclose(normal_src);
   fclose(dest);
   delete_file(destfile);
   return err;
}

/*** end of zc_sys.cpp ***/
