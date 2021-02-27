//--------------------------------------------------------
//  Zelda Classic
//  by Jeremy Craner, 1999-2000
//
//  zc_sys.cc
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

#ifdef ALLEGRO_DOS
#include <unistd.h>
#endif

#include "zdefs.h"
#include "zelda.h"
#include "tiles.h"
#include "colors.h"
#include "pal.h"
#include "zsys.h"
#include "qst.h"
#include "zc_sys.h"
#include "jwin.h"
#include "jwinfsel.h"
#include "gui.h"
#include "midi.h"
#include "subscr.h"
#include "maps.h"
#include "sprite.h"
#include "guys.h"
#include "link.h"
#include "title.h"
#include "particles.h"

extern FONT *lfont;
extern LinkClass Link;
extern sprite_list  guys, items, Ewpns, Lwpns, Sitems, chainlinks, decorations, particles;

/**********************************/
/******** System functions ********/
/**********************************/

static char cfg_sect[] = "zeldadx";

int d_dummy_proc(int msg,DIALOG *d,int c) { return D_O_K; }

void load_game_configs()
{
  Akey = get_config_int(cfg_sect,"key_a",KEY_ALT);
  Bkey = get_config_int(cfg_sect,"key_b",KEY_ZC_LCONTROL);
  Skey = get_config_int(cfg_sect,"key_s",KEY_ENTER);
  Lkey = get_config_int(cfg_sect,"key_l",KEY_Z);
  Rkey = get_config_int(cfg_sect,"key_r",KEY_X);
  Mkey = get_config_int(cfg_sect,"key_m",KEY_SPACE);

  DUkey = get_config_int(cfg_sect,"key_up",   KEY_UP);
  DDkey = get_config_int(cfg_sect,"key_down", KEY_DOWN);
  DLkey = get_config_int(cfg_sect,"key_left", KEY_LEFT);
  DRkey = get_config_int(cfg_sect,"key_right",KEY_RIGHT);

  digi_volume = get_config_int(cfg_sect,"sfx",248);
  midi_volume = get_config_int(cfg_sect,"music",255);
  pan_style = get_config_int(cfg_sect,"pan",1);
  
  Throttlefps = get_config_int(cfg_sect,"throttlefps",1)!=0;
  TransLayers = (bool)get_config_int(cfg_sect,"translayers",1);
  ShowFPS = (bool)get_config_int(cfg_sect,"showfps",0);

#ifdef ALLEGRO_DOS
  VidMode = get_config_int(cfg_sect,"dos_mode",GFX_AUTODETECT);
#elif defined(ALLEGRO_WINDOWS)
  VidMode = get_config_int(cfg_sect,"win_mode",GFX_AUTODETECT_FULLSCREEN);
#elif defined(ALLEGRO_LINUX)
  VidMode = get_config_int(cfg_sect,"linux_mode",GFX_AUTODETECT_FULLSCREEN);
#elif defined(ALLEGRO_MACOSX)
  VidMode = get_config_int(cfg_sect,"macosx_mode",GFX_AUTODETECT_FULLSCREEN);
#endif
  resx = get_config_int(cfg_sect,"resx",320);
  resy = get_config_int(cfg_sect,"resy",240);
  sbig = get_config_int(cfg_sect,"sbig",0);
  scanlines = get_config_int(cfg_sect,"scanlines",0);
  strcpy(qstpath,get_config_string(cfg_sect,"qst_dir",""));

  if(strlen(qstpath)==0)
  {
    getcwd(qstpath,280);
    fix_filename_case(qstpath);
    fix_filename_slashes(qstpath);
    put_backslash(qstpath);
  }
  else
  {
    chop_path(qstpath);
  }
  HeartBeep = get_config_int(cfg_sect,"heartbeep",1);
}

void save_game_configs()
{
  set_config_int(cfg_sect,"key_a",Akey);
  set_config_int(cfg_sect,"key_b",Bkey);
  set_config_int(cfg_sect,"key_s",Skey);
  set_config_int(cfg_sect,"key_l",Lkey);
  set_config_int(cfg_sect,"key_r",Rkey);
  set_config_int(cfg_sect,"key_m",Mkey);

  set_config_int(cfg_sect,"key_up",   DUkey);
  set_config_int(cfg_sect,"key_down", DDkey);
  set_config_int(cfg_sect,"key_left", DLkey);
  set_config_int(cfg_sect,"key_right",DRkey);

  set_config_int(cfg_sect,"sfx",digi_volume);
  set_config_int(cfg_sect,"music",midi_volume);
  set_config_int(cfg_sect,"pan",pan_style);
  set_config_int(cfg_sect,"throttlefps", (int)Throttlefps);
  set_config_int(cfg_sect,"translayers",(int)TransLayers);
  set_config_int(cfg_sect,"showfps",(int)ShowFPS);

#ifdef ALLEGRO_DOS
  set_config_int(cfg_sect,"dos_mode",VidMode);
#elif defined(ALLEGRO_WINDOWS)
  set_config_int(cfg_sect,"win_mode",VidMode);
#elif defined(ALLEGRO_LINUX)
  set_config_int(cfg_sect,"linux_mode",VidMode);
#elif defined(ALLEGRO_MACOSX)
  set_config_int(cfg_sect,"macosx_mode",VidMode);
#endif

  set_config_int(cfg_sect,"resx",resx);
  set_config_int(cfg_sect,"resy",resy);
  set_config_int(cfg_sect,"sbig",sbig);
  set_config_int(cfg_sect,"scanlines",scanlines);
  chop_path(qstpath);
  set_config_string(cfg_sect,"qst_dir",qstpath);
  set_config_int(cfg_sect,"heartbeep",HeartBeep);
}

//----------------------------------------------------------------

// Timers

void fps_callback()
{
  lastfps=framecnt;
  avgfps=((long double)avgfps*fps_secs+lastfps)/(++fps_secs);
  framecnt=0;
}

END_OF_FUNCTION(fps_callback)

void myvsync_callback()
{
  ++myvsync;
}

END_OF_FUNCTION(myvsync_callback)

void Z_init_timers()
{
  static bool didit = false;
  static char *err_str = "Couldn't allocate timer";

  if(didit)
    return;

  didit = true;

  LOCK_VARIABLE(lastfps);
  LOCK_VARIABLE(framecnt);
  LOCK_FUNCTION(fps_callback);
  if(install_int_ex(fps_callback,SECS_TO_TIMER(1)))
    Z_error(err_str);

  LOCK_VARIABLE(myvsync);
  LOCK_FUNCTION(myvsync_callback);
  if(install_int_ex(myvsync_callback,BPS_TO_TIMER(60)))
    Z_error(err_str);
}

//----------------------------------------------------------------

void go()
{
  scare_mouse();
  blit(screen,tmp_scr,scrx,scry,0,0,screen->w,screen->h);
  unscare_mouse();
}

void comeback()
{
  scare_mouse();
  blit(tmp_scr,screen,0,0,scrx,scry,screen->w,screen->h);
  unscare_mouse();
}

void dump_pal(BITMAP *dest)
{
  for(int i=0; i<256; i++)
    rectfill(dest,(i&63)<<2,(i&0xFC0)>>4,((i&63)<<2)+3,((i&0xFC0)>>4)+3,i);
}

void show_paused()
{
  //  return;
  char buf[7] = "PAUSED";
  for(int i=0; buf[i]!=0; i++)
    buf[i]+=0x60;

  //  text_mode(-1);
  if(sbig)
    textout_ex(screen,zfont,buf,scrx+40-120,scry+224+104,-1,-1);
  else
    textout_ex(screen,zfont,buf,scrx+40,scry+224,-1,-1);
}

void show_fps()
{
  char buf[50];

  //  text_mode(-1);
  sprintf(buf,"%2d/60",lastfps);
  //  sprintf(buf,"%d/%u/%f/%u",lastfps,int(avgfps),avgfps,fps_secs);
  for(int i=0; buf[i]!=0; i++)
    if(buf[i]!=' ')
      buf[i]+=0x60;

  if(sbig)
  {
    textout_ex(screen,zfont,buf,scrx+40-120,scry+216+104,-1,-1);
  }
  else
  {
    textout_ex(screen,zfont,buf,scrx+40,scry+216,-1,-1);
  }
}

//----------------------------------------------------------------

// sets the video mode and initializes the palette and mouse sprite
bool game_vid_mode(int mode,int wait)
{
  //  set_color_depth(colordepth);

  #ifdef ALLEGRO_DOS
  switch(mode)
  {
    case GFX_AUTODETECT:
    case GFX_VESA3:
      if(set_gfx_mode(GFX_VESA3,resx,resy,0,0)==0)
      {
        VidMode=GFX_VESA3;
        break;
      }
    case GFX_VESA2L:
      if(set_gfx_mode(GFX_VESA2L,resx,resy,0,0)==0)
      {
        VidMode=GFX_VESA2L;
        break;
      }
    case GFX_VESA2B:
      if(set_gfx_mode(GFX_VESA2B,resx,resy,0,0)==0)
      {
        VidMode=GFX_VESA2B;
        break;
      }
    case GFX_VESA1:
      if(set_gfx_mode(GFX_VESA1,resx,resy,0,0)==0)
      {
        VidMode=GFX_VESA1;
        break;
      }
    case GFX_MODEX:
      if(set_gfx_mode(GFX_MODEX,320,240,0,0)==0)
      {
        VidMode=GFX_MODEX;
        resx=320;
        resy=240;
        sbig=false;
        break;
      }
    default:
      return false;
      break;
  }
  #elif defined(ALLEGRO_WINDOWS)
  switch(mode)
  {
    case GFX_AUTODETECT_FULLSCREEN:
    case GFX_DIRECTX:
      if(set_gfx_mode(GFX_DIRECTX,resx,resy,0,0)==0)
      {
        VidMode=GFX_DIRECTX;
        break;
      }
    case GFX_DIRECTX_SOFT:
      if(set_gfx_mode(GFX_DIRECTX_SOFT,resx,resy,0,0)==0)
      {
        VidMode=GFX_DIRECTX_SOFT;
        break;
      }
    case GFX_DIRECTX_SAFE:
      if(set_gfx_mode(GFX_DIRECTX_SAFE,resx,resy,0,0)==0)
      {
        VidMode=GFX_DIRECTX_SAFE;
      }
      break;
    case GFX_AUTODETECT_WINDOWED:
    case GFX_DIRECTX_WIN:
      if(set_gfx_mode(GFX_DIRECTX_WIN,resx,resy,0,0)==0)
      {
        VidMode=GFX_DIRECTX_WIN;
        break;
      }
    case GFX_GDI:
      if(set_gfx_mode(GFX_GDI,resx,resy,0,0)==0)
      {
        VidMode=GFX_GDI;
        break;
      }
    default:
      return false;
      break;
  }
  #elif defined(ALLEGRO_LINUX)
  switch(mode)
  {
case GFX_AUTODETECT_FULLSCREEN:    
case GFX_AUTODETECT_WINDOWED:
      if(set_gfx_mode(GFX_AUTODETECT_WINDOWED,resx,resy,0,0)==0)
      {
        VidMode=GFX_AUTODETECT_WINDOWED;
        break;
      }
    default:
      return false;
      break;
  }
  #elif defined(ALLEGRO_MACOSX)
  switch(mode)
  {
    case GFX_AUTODETECT_WINDOWED:
      if(set_gfx_mode(GFX_AUTODETECT_WINDOWED,resx,resy,0,0)==0)
      {
        VidMode=GFX_AUTODETECT_WINDOWED;
        break;
      }
    default:
      return false;
      break;
  }
  #endif

  scrx = (resx-320)>>1;
  scry = (resy-240)>>1;

  set_mouse_sprite((BITMAP*)data[BMP_MOUSE].dat);
  for(int i=240; i<256; i++)
    RAMpal[i]=((RGB*)data[PAL_GUI].dat)[i];
  set_palette(RAMpal);
  clear_to_color(screen,BLACK);

  rest(wait);
  return true;
}

void init_NES_mode()
{
  if (!init_colordata(true, &QHeader, &QMisc))
  {
    return;
  }
  loadfullpal();
  init_tiles(false, &QHeader);
}

//----------------------------------------------------------------

qword trianglelines[16]=
{
  0x0000000000000000ULL,
  0xFD00000000000000ULL,
  0xFDFD000000000000ULL,
  0xFDFDFD0000000000ULL,
  0xFDFDFDFD00000000ULL,
  0xFDFDFDFDFD000000ULL,
  0xFDFDFDFDFDFD0000ULL,
  0xFDFDFDFDFDFDFD00ULL,
  0xFDFDFDFDFDFDFDFDULL,
  0x00FDFDFDFDFDFDFDULL,
  0x0000FDFDFDFDFDFDULL,
  0x000000FDFDFDFDFDULL,
  0x00000000FDFDFDFDULL,
  0x0000000000FDFDFDULL,
  0x000000000000FDFDULL,
  0x00000000000000FDULL,
};

word screen_triangles[28][32];
/*
qword triangles[4][16]= //[direction][value]
{
  {
    0x00000000, 0x10000000, 0x21000000, 0x32100000, 0x43210000, 0x54321000, 0x65432100, 0x76543210, 0x87654321, 0x88765432, 0x88876543, 0x88887654, 0x88888765, 0x88888876, 0x88888887, 0x88888888
  },
  {
    0x00000000, 0xF0000000, 0xEF000000, 0xFDF00000, 0xCFDF0000, 0xBCFDF000, 0xABCFDF00, 0x9ABCFDF0, 0x89ABCFDF, 0x889ABCFD, 0x8889ABCD, 0x88889ABC, 0x888889AB, 0x8888889A, 0x88888889, 0x88888888
  },
  {
    0x00000000, 0x00000001, 0x00000012, 0x00000123, 0x00001234, 0x00012345, 0x00123456, 0x01234567, 0x12345678, 0x23456788, 0x34567888, 0x45678888, 0x56788888, 0x67888888, 0x78888888, 0x88888888
  },
  {
    0x00000000, 0x0000000F, 0x000000FE, 0x00000FED, 0x0000FEDC, 0x000FEDCB, 0x00FEDCBA, 0x0FEDCBA9, 0xFEDCBA98, 0xEDCBA988, 0xDCBA9888, 0xCBA98888, 0xBA988888, 0xA9888888, 0x98888888, 0x88888888
  }
};
*/


/*
byte triangles[4][16][8]= //[direction][value][line]
{
  {
    {
       0,  0,  0,  0,  0,  0,  0,  0
    },
    {
       1,  0,  0,  0,  0,  0,  0,  0
    },
    {
       2,  1,  0,  0,  0,  0,  0,  0
    },
    {
       3,  2,  1,  0,  0,  0,  0,  0
    },
    {
       4,  3,  2,  1,  0,  0,  0,  0
    },
    {
       5,  4,  3,  2,  1,  0,  0,  0
    },
    {
       6,  5,  4,  3,  2,  1,  0,  0
    },
    {
       7,  6,  5,  4,  3,  2,  1,  0
    },
    {
       8,  7,  6,  5,  4,  3,  2,  1
    },
    {
       8,  8,  7,  6,  5,  4,  3,  2
    },
    {
       8,  8,  8,  7,  6,  5,  4,  3
    },
    {
       8,  8,  8,  8,  7,  6,  5,  4
    },
    {
       8,  8,  8,  8,  8,  7,  6,  5
    },
    {
       8,  8,  8,  8,  8,  8,  7,  6
    },
    {
       8,  8,  8,  8,  8,  8,  8,  7
    },
    {
       8,  8,  8,  8,  8,  8,  8,  8
    }
  },
  {
    {
       0,  0,  0,  0,  0,  0,  0,  0
    },
    {
      15,  0,  0,  0,  0,  0,  0,  0
    },
    {
      14, 15,  0,  0,  0,  0,  0,  0
    },
    {
      13, 14, 15,  0,  0,  0,  0,  0
    },
    {
      12, 13, 14, 15,  0,  0,  0,  0
    },
    {
      11, 12, 13, 14, 15,  0,  0,  0
    },
    {
      10, 11, 12, 13, 14, 15,  0,  0
    },
    {
       9, 10, 11, 12, 13, 14, 15,  0
    },
    {
       8,  9, 10, 11, 12, 13, 14, 15
    },
    {
       8,  8,  9, 10, 11, 12, 13, 14
    },
    {
       8,  8,  8,  9, 10, 11, 12, 13
    },
    {
       8,  8,  8,  8,  9, 10, 11, 12
    },
    {
       8,  8,  8,  8,  8,  9, 10, 11
    },
    {
       8,  8,  8,  8,  8,  8,  9, 10
    },
    {
       8,  8,  8,  8,  8,  8,  8,  9
    },
    {
       8,  8,  8,  8,  8,  8,  8,  8
    }
  },
  {
    {
       0,  0,  0,  0,  0,  0,  0,  0
    },
    {
       0,  0,  0,  0,  0,  0,  0,  1
    },
    {
       0,  0,  0,  0,  0,  0,  1,  2
    },
    {
       0,  0,  0,  0,  0,  1,  2,  3
    },
    {
       0,  0,  0,  0,  1,  2,  3,  4
    },
    {
       0,  0,  0,  1,  2,  3,  4,  5
    },
    {
       0,  0,  1,  2,  3,  4,  5,  6
    },
    {
       0,  1,  2,  3,  4,  5,  6,  7
    },
    {
       1,  2,  3,  4,  5,  6,  7,  8
    },
    {
       2,  3,  4,  5,  6,  7,  8,  8
    },
    {
       3,  4,  5,  6,  7,  8,  8,  8
    },
    {
       4,  5,  6,  7,  8,  8,  8,  8
    },
    {
       5,  6,  7,  8,  8,  8,  8,  8
    },
    {
       6,  7,  8,  8,  8,  8,  8,  8
    },
    {
       7,  8,  8,  8,  8,  8,  8,  8
    },
    {
       8,  8,  8,  8,  8,  8,  8,  8
    }
  },
  {
    {
       0,  0,  0,  0,  0,  0,  0,  0
    },
    {
       0,  0,  0,  0,  0,  0,  0, 15
    },
    {
       0,  0,  0,  0,  0,  0, 15, 14
    },
    {
       0,  0,  0,  0,  0, 15, 14, 13
    },
    {
       0,  0,  0,  0, 15, 14, 13, 12
    },
    {
       0,  0,  0, 15, 14, 13, 12, 11
    },
    {
       0,  0, 15, 14, 13, 12, 11, 10
    },
    {
       0, 15, 14, 13, 12, 11, 10,  9
    },
    {
      15, 14, 13, 12, 11, 10,  9,  8
    },
    {
      14, 13, 12, 11, 10,  9,  8,  8
    },
    {
      13, 12, 11, 10,  9,  8,  8,  8
    },
    {
      12, 11, 10,  9,  8,  8,  8,  8
    },
    {
      11, 10,  9,  8,  8,  8,  8,  8
    },
    {
      10,  9,  8,  8,  8,  8,  8,  8
    },
    {
       9,  8,  8,  8,  8,  8,  8,  8
    },
    {
       8,  8,  8,  8,  8,  8,  8,  8
    }
  }
};
*/



/*
for (int blockrow=0; blockrow<30; ++i)
{
  for (int linerow=0; linerow<8; ++i)
  {
    qword *triangleline=(qword*)(tmp_scr->line[(blockrow*8+linerow)]);
    for (int blockcolumn=0; blockcolumn<40; ++i)
    {
      triangleline=triangles[0][screen_triangles[blockrow][blockcolumn]][linerow];
      ++triangleline;
    }
  }
}
*/

// the ULL suffixes are to prevent this warning:
// warning: integer constant is too large for "long" type

qword triangles[4][16][8]= //[direction][value][line]
{
  {
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0xFD00000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0xFDFD000000000000ULL,
      0xFD00000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0xFDFDFD0000000000ULL,
      0xFDFD000000000000ULL,
      0xFD00000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0xFDFDFDFD00000000ULL,
      0xFDFDFD0000000000ULL,
      0xFDFD000000000000ULL,
      0xFD00000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFD00000000ULL,
      0xFDFDFD0000000000ULL,
      0xFDFD000000000000ULL,
      0xFD00000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFD00000000ULL,
      0xFDFDFD0000000000ULL,
      0xFDFD000000000000ULL,
      0xFD00000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFD00000000ULL,
      0xFDFDFD0000000000ULL,
      0xFDFD000000000000ULL,
      0xFD00000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFD00000000ULL,
      0xFDFDFD0000000000ULL,
      0xFDFD000000000000ULL,
      0xFD00000000000000ULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFD00000000ULL,
      0xFDFDFD0000000000ULL,
      0xFDFD000000000000ULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFD00000000ULL,
      0xFDFDFD0000000000ULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFD00000000ULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFD000000ULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFD0000ULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFD00ULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    }
  },
  {
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0x00000000000000FDULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0x000000000000FDFDULL,
      0x00000000000000FDULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0x0000000000FDFDFDULL,
      0x000000000000FDFDULL,
      0x00000000000000FDULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0x00000000FDFDFDFDULL,
      0x0000000000FDFDFDULL,
      0x000000000000FDFDULL,
      0x00000000000000FDULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0x000000FDFDFDFDFDULL,
      0x00000000FDFDFDFDULL,
      0x0000000000FDFDFDULL,
      0x000000000000FDFDULL,
      0x00000000000000FDULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0x0000FDFDFDFDFDFDULL,
      0x000000FDFDFDFDFDULL,
      0x00000000FDFDFDFDULL,
      0x0000000000FDFDFDULL,
      0x000000000000FDFDULL,
      0x00000000000000FDULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0x00FDFDFDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL,
      0x000000FDFDFDFDFDULL,
      0x00000000FDFDFDFDULL,
      0x0000000000FDFDFDULL,
      0x000000000000FDFDULL,
      0x00000000000000FDULL,
      0x0000000000000000ULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL,
      0x000000FDFDFDFDFDULL,
      0x00000000FDFDFDFDULL,
      0x0000000000FDFDFDULL,
      0x000000000000FDFDULL,
      0x00000000000000FDULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL,
      0x000000FDFDFDFDFDULL,
      0x00000000FDFDFDFDULL,
      0x0000000000FDFDFDULL,
      0x000000000000FDFDULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL,
      0x000000FDFDFDFDFDULL,
      0x00000000FDFDFDFDULL,
      0x0000000000FDFDFDULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL,
      0x000000FDFDFDFDFDULL,
      0x00000000FDFDFDFDULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL,
      0x000000FDFDFDFDFDULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    }
  },
  {
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0xFD00000000000000ULL
    },
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0xFD00000000000000ULL,
      0xFDFD000000000000ULL
    },
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0xFD00000000000000ULL,
      0xFDFD000000000000ULL,
      0xFDFDFD0000000000ULL
    },
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0xFD00000000000000ULL,
      0xFDFD000000000000ULL,
      0xFDFDFD0000000000ULL,
      0xFDFDFDFD00000000ULL
    },
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0xFD00000000000000ULL,
      0xFDFD000000000000ULL,
      0xFDFDFD0000000000ULL,
      0xFDFDFDFD00000000ULL,
      0xFDFDFDFDFD000000ULL
    },
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0xFD00000000000000ULL,
      0xFDFD000000000000ULL,
      0xFDFDFD0000000000ULL,
      0xFDFDFDFD00000000ULL,
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFDFDFD0000ULL
    },
    {
      0x0000000000000000ULL,
      0xFD00000000000000ULL,
      0xFDFD000000000000ULL,
      0xFDFDFD0000000000ULL,
      0xFDFDFDFD00000000ULL,
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFDFDFD00ULL
    },
    {
      0xFD00000000000000ULL,
      0xFDFD000000000000ULL,
      0xFDFDFD0000000000ULL,
      0xFDFDFDFD00000000ULL,
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0xFDFD000000000000ULL,
      0xFDFDFD0000000000ULL,
      0xFDFDFDFD00000000ULL,
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0xFDFDFD0000000000ULL,
      0xFDFDFDFD00000000ULL,
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0xFDFDFDFD00000000ULL,
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0xFDFDFDFDFD000000ULL,
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0xFDFDFDFDFDFD0000ULL,
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0xFDFDFDFDFDFDFD00ULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    }
  },
  {
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL
    },
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x00000000000000FDULL
    },
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x00000000000000FDULL,
      0x000000000000FDFDULL
    },
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x00000000000000FDULL,
      0x000000000000FDFDULL,
      0x0000000000FDFDFDULL
    },
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x00000000000000FDULL,
      0x000000000000FDFDULL,
      0x0000000000FDFDFDULL,
      0x00000000FDFDFDFDULL
    },
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x00000000000000FDULL,
      0x000000000000FDFDULL,
      0x0000000000FDFDFDULL,
      0x00000000FDFDFDFDULL,
      0x000000FDFDFDFDFDULL
    },
    {
      0x0000000000000000ULL,
      0x0000000000000000ULL,
      0x00000000000000FDULL,
      0x000000000000FDFDULL,
      0x0000000000FDFDFDULL,
      0x00000000FDFDFDFDULL,
      0x000000FDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL
    },
    {
      0x0000000000000000ULL,
      0x00000000000000FDULL,
      0x000000000000FDFDULL,
      0x0000000000FDFDFDULL,
      0x00000000FDFDFDFDULL,
      0x000000FDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL
    },
    {
      0x00000000000000FDULL,
      0x000000000000FDFDULL,
      0x0000000000FDFDFDULL,
      0x00000000FDFDFDFDULL,
      0x000000FDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0x000000000000FDFDULL,
      0x0000000000FDFDFDULL,
      0x00000000FDFDFDFDULL,
      0x000000FDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0x0000000000FDFDFDULL,
      0x00000000FDFDFDFDULL,
      0x000000FDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0x00000000FDFDFDFDULL,
      0x000000FDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0x000000FDFDFDFDFDULL,
      0x0000FDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0x0000FDFDFDFDFDFDULL,
      0x00FDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0x00FDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    },
    {
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL,
      0xFDFDFDFDFDFDFDFDULL
    }
  }
};

int black_opening_count=0;
int black_opening_x,black_opening_y;
int black_opening_shape=bosCIRCLE;
//int black_opening_shape=bosSMAS;

void close_black_opening(int x, int y, bool wait)
{
  int w=256, h=224;
  int blockrows=28, blockcolumns=32;
  int xoffset=(x-(w/2))/8, yoffset=(y-(h/2))/8;
  for (int blockrow=0; blockrow<blockrows; ++blockrow) //30
  {
    for (int blockcolumn=0; blockcolumn<blockcolumns; ++blockcolumn) //40
    {
      screen_triangles[blockrow][blockcolumn]=max(abs(int(double(blockcolumns-1)/2-blockcolumn+xoffset)),abs(int(double(blockrows-1)/2-blockrow+yoffset)))|0x0100|((blockrow-yoffset<blockrows/2)?0:0x8000)|((blockcolumn-xoffset<blockcolumns/2)?0x4000:0);
    }
  }
  black_opening_count = 66;
  black_opening_x = x;
  black_opening_y = y;
  lensclk = 0;
  //black_opening_shape=(black_opening_shape+1)%bosMAX;

  if(wait)
  {
    for(int i=0; i<66; i++)
    {
      drawit=true;
      draw_screen(tmpscr, 0, 0);
      putsubscr(framebuf,0,0);
//      blit(scrollbuf,framebuf,0,0,0,56,256,168);
      syskeys();
      advanceframe();
      if(Quit)
      {
        break;
      }
    }
  }
  logic_counter=0;
  drawit=true;
}

void open_black_opening(int x, int y, bool wait)
{
  int w=256, h=224;
  int blockrows=28, blockcolumns=32;
  int xoffset=(x-(w/2))/8, yoffset=(y-(h/2))/8;
  for (int blockrow=0; blockrow<blockrows; ++blockrow) //30
  {
    for (int blockcolumn=0; blockcolumn<blockcolumns; ++blockcolumn) //40
    {
      screen_triangles[blockrow][blockcolumn]=max(abs(int(double(blockcolumns-1)/2-blockcolumn+xoffset)),abs(int(double(blockrows-1)/2-blockrow+yoffset)))|0x0100|((blockrow-yoffset<blockrows/2)?0:0x8000)|((blockcolumn-xoffset<blockcolumns/2)?0x4000:0);
    }
  }
  black_opening_count = -66;
  black_opening_x = x;
  black_opening_y = y;
  lensclk = 0;

  if(wait)
  {
    for(int i=0; i<66; i++)
    {
      drawit=true;
      draw_screen(tmpscr, 0, 0);
      putsubscr(framebuf,0,0);
      syskeys();
      advanceframe();
      if(Quit)
      {
        break;
      }
    }
  }
  logic_counter=0;
  drawit=true;
}

void black_opening(BITMAP *dest,int x,int y,int a,int max_a)
{
  clear_to_color(tmp_scr,BLACK);
  int w=256, h=224;
  switch (black_opening_shape)
  {
    case bosOVAL:
    {
      double new_w=(w/2)+abs(w/2-x);
      double new_h=(h/2)+abs(h/2-y);
      double b=sqrt(((new_w*new_w)/4)+(new_h*new_h));
      ellipsefill(tmp_scr,x,y,int(2*a*b/max_a),int(a*b/max_a),0);
      break;
    }
    case bosTRIANGLE:
    {
      double new_w=(w/2)+abs(w/2-x);
      double new_h=(h/2)+abs(h/2-y);
      double r=a*(new_w*sqrt(3)+new_h)/max_a;
      double P2= (PI/2);
      double P23=(2*PI/3);
      double P43=(4*PI/3);
      double Pa= (-4*PI*a/(3*max_a));
      double angle=P2+Pa;
      double a0=angle;
      double a2=angle+P23;
      double a4=angle+P43;
      triangle(tmp_scr, x+int(cos(a0)*r), y-int(sin(a0)*r),
                        x+int(cos(a2)*r), y-int(sin(a2)*r),
                        x+int(cos(a4)*r), y-int(sin(a4)*r),
                        0);
      break;
    }
    case bosSMAS:
    {
      int distance=max(abs(w/2-x),abs(h/2-y))/8;
      for (int blockrow=0; blockrow<28; ++blockrow) //30
      {
        for (int linerow=0; linerow<8; ++linerow)
        {
          qword *triangleline=(qword*)(tmp_scr->line[(blockrow*8+linerow)]);
          for (int blockcolumn=0; blockcolumn<32; ++blockcolumn) //40
          {
            //*triangleline=triangles[(screen_triangles[blockrow][blockcolumn]&0xC000)>>14][min(max((((max_a-a)/2)+(screen_triangles[blockrow][blockcolumn]&0x0FFF)-15),0),15)][linerow];
            *triangleline=triangles[(screen_triangles[blockrow][blockcolumn]&0xC000)>>14]
                                   [min(max((((31+distance)*(max_a-a)/max_a)+((screen_triangles[blockrow][blockcolumn]&0x0FFF)-0x0100)-(15+distance)),0),15)]
                                   [linerow];
            ++triangleline;
            if (linerow==0)
            {
            }
          }
        }
      }
      break;
    }
    case bosCIRCLE:
    default:
    {
      double new_w=(w/2)+abs(w/2-x);
      double new_h=(h/2)+abs(h/2-y);
      int r=int(sqrt((new_w*new_w)+(new_h*new_h))*a/max_a);
      //circlefill(tmp_scr,x,y,a<<3,0);
      circlefill(tmp_scr,x,y,r,0);
      break;
    }
  }
  masked_blit(tmp_scr,dest,0,0,0,0,320,240);
}

//----------------------------------------------------------------

bool item_disabled(int item_type, int item)                 //is this item disabled?
{
  return false;
}

bool can_use_item(int item_type, int item)                  //can Link use this item?
{
  if (has_item(item_type, item) && !item_disabled(item_type, item))
  {
    return true;
  }
  return false;
}

bool has_item(int item_type, int it)                        //does Link possess this item?
{
  switch (item_type)
  {
    case itype_bomb:
    case itype_sbomb:
      return (game.items[item_type]>0);
      break;
    case itype_clock:
      return Link.getClock()?1:0;
    case itype_key:
      return (game.keys>0);
    case itype_magiccontainer:
      return (game.maxmagic>=MAGICPERBLOCK);
    case itype_triforcepiece:                               //it: -2=any, -1=current level, other=that level
    {
      switch (it)
      {
        case -2:
        {
          for (int i=0; i<MAXLEVELS; i++)
          {
            if (game.lvlitems[i]|liTRIFORCE)

            {
              return true;
            }
          }
          return false;
          break;
        }
        case -1:
          return (game.lvlitems[dlevel]|liTRIFORCE);
          break;
        default:
          if (it>=0&&it<MAXLEVELS)
          {
            return (game.lvlitems[it]|liTRIFORCE);
          }
          break;
      }
      return 0;
    }
    case itype_map:                                         //it: -2=any, -1=current level, other=that level
    {
      switch (it)
      {
        case -2:
        {
          for (int i=0; i<MAXLEVELS; i++)
          {
            if (game.lvlitems[i]|liMAP)
            {
              return true;
            }
          }
          return false;
        }
        break;
        case -1:
          return (game.lvlitems[dlevel]|liMAP);
          break;
        default:
          if (it>=0&&it<MAXLEVELS)
          {
            return (game.lvlitems[it]|liMAP);
          }
          break;
      }
      return 0;
    }
    case itype_compass:                                     //it: -2=any, -1=current level, other=that level
    {
      switch (it)
      {
        case -2:
        {
          for (int i=0; i<MAXLEVELS; i++)
          {
            if (game.lvlitems[i]|liCOMPASS)
            {
              return true;
            }
          }
          return false;
          break;
        }
        case -1:
          return (game.lvlitems[dlevel]|liCOMPASS);
          break;
        default:
          if (it>=0&&it<MAXLEVELS)
          {
            return (game.lvlitems[it]|liCOMPASS);
          }
          break;
      }
      return 0;
    }
    case itype_bosskey:                                     //it: -2=any, -1=current level, other=that level
    {
      switch (it)
      {
        case -2:
        {
          for (int i=0; i<MAXLEVELS; i++)
          {
            if (game.lvlitems[i]|liBOSSKEY)
            {
              return true;
            }
          }
          return false;
          break;
        }
        case -1:
          return (game.lvlitems[dlevel]|liBOSSKEY)?1:0;
          break;
        default:
          if (it>=0&&it<MAXLEVELS)
          {
            return (game.lvlitems[it]|liBOSSKEY)?1:0;
          }
          break;
      }
      return 0;
    }
    default:
      it=(1<<(it-1));
      if (item_type>=itype_max)
      {
        al_trace("Error - has_item() exception.");
        return false;
      }
      if (game.items[item_type]&it)
      {
        return true;
      }
      break;
  }
  return false;
}

int high_item(int jmax, int item_type, bool consecutive, int itemcluster, bool usecluster)
{

  if (usecluster)
  {
    for (int j=jmax-1; j>0; j--)
    {
      if (itemcluster&(1<<(j-1)))
      {
        return consecutive?j:(1<<(j-1));
      }
    }
  }
  else
  {
    for (int j=jmax-1; j>0; j--)
    {
      if (can_use_item(item_type, j))

      {
        return consecutive?j:(1<<(j-1));
      }
    }
  }

  return 0;
}

int current_item(int item_type, bool consecutive)           //item currently being used
{
  int jmax=0;
  switch(item_type)
  {
    case itype_sword:
      jmax=imax_sword;
      break;
    case itype_brang:
      jmax=imax_brang;
      break;
    case itype_arrow:
      jmax=imax_arrow;
      break;
    case itype_candle:
      jmax=imax_candle;
      break;
    case itype_whistle:
      jmax=imax_whistle;
      break;
    case itype_bait:
      jmax=imax_bait;
      break;
    case itype_letter:
      jmax=imax_letter;
      break;
    case itype_potion:
      jmax=imax_potion;
      break;
    case itype_wand:
      jmax=imax_wand;
      break;
    case itype_ring:
      jmax=imax_ring;
      break;
    case itype_wallet:
      jmax=imax_wallet;
      break;
    case itype_amulet:
      jmax=imax_amulet;
      break;
    case itype_shield:
      jmax=imax_shield;
      break;
    case itype_bow:
      jmax=imax_bow;
      break;
    case itype_raft:
      jmax=imax_raft;
      break;
    case itype_ladder:
      jmax=imax_ladder;
      break;
    case itype_book:
      jmax=imax_book;
      break;
    case itype_magickey:
      jmax=imax_magickey;
      break;
    case itype_bracelet:
      jmax=imax_bracelet;
      break;
    case itype_flippers:
      jmax=imax_flippers;
      break;
    case itype_boots:
      jmax=imax_boots;
      break;
    case itype_hookshot:
      jmax=imax_hookshot;
      break;
    case itype_lens:
      jmax=imax_lens;
      break;
    case itype_hammer:
      jmax=imax_hammer;
      break;
    case itype_dinsfire:
      jmax=imax_dinsfire;
      break;
    case itype_faroreswind:
      jmax=imax_faroreswind;
      break;
    case itype_nayruslove:
      jmax=imax_nayruslove;
      break;
    case itype_bomb:
    case itype_sbomb:
      return can_use_item(item_type,1) ? game.items[item_type] : 0;
      break;
    case itype_clock:
      return has_item(itype_clock,1) ? 1 : 0;
      break;
    case itype_key:
      return game.keys;
    case itype_magiccontainer:
      return game.maxmagic/MAGICPERBLOCK;
    case itype_triforcepiece:
    {
      int count=0;
      for (int i=0; i<MAXLEVELS; i++)
      {
        count+=(game.lvlitems[i]|liTRIFORCE)?1:0;
      }
      return 0;
      break;
    }
    case itype_map:
    {
      int count=0;
      for (int i=0; i<MAXLEVELS; i++)
      {
        count+=(game.lvlitems[i]|liMAP)?1:0;
      }
      return count;
      break;
    }
    case itype_compass:
    {
      int count=0;
      for (int i=0; i<MAXLEVELS; i++)
      {
        count+=(game.lvlitems[i]|liCOMPASS)?1:0;
      }
      return count;
      break;
    }
    case itype_bosskey:
    {
      int count=0;
      for (int i=0; i<MAXLEVELS; i++)
      {
        count+=(game.lvlitems[i]|liBOSSKEY)?1:0;
      }
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
  long tile=0;
  int ret=0;
  ret=current_item(itype_sword, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iSword].ltm; break;
    case 2: ret=itemsbuf[iWSword].ltm; break;
    case 3: ret=itemsbuf[iMSword].ltm; break;
    case 4: ret=itemsbuf[iXSword].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_brang, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iBrang].ltm; break;
    case 2: ret=itemsbuf[iMBrang].ltm; break;
    case 3: ret=itemsbuf[iFBrang].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_arrow, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iArrow].ltm; break;
    case 2: ret=itemsbuf[iSArrow].ltm; break;
    case 3: ret=itemsbuf[iGArrow].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_candle, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iBCandle].ltm; break;
    case 2: ret=itemsbuf[iRCandle].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_whistle, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iWhistle].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;
  ret=current_item(itype_bait, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iBait].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_letter, true);
  switch(ret)
  {
    case 1:
    case 2: ret=itemsbuf[iLetter].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_potion, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iBPotion].ltm; break;
    case 2: ret=itemsbuf[iRPotion].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_wand, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iWand].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_ring, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iBRing].ltm; break;
    case 2: ret=itemsbuf[iRRing].ltm; break;
    case 3: ret=itemsbuf[iGRing].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_wallet, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iWallet500].ltm; break;
    case 2: ret=itemsbuf[iWallet999].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;
  ret=current_item(itype_amulet, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iAmulet].ltm; break;
    case 2: ret=itemsbuf[iL2Amulet].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_shield, true);
  switch(ret)
  {
    case 1: ret=0; break;
    case 2: ret=itemsbuf[iShield].ltm; break;
    case 3: ret=itemsbuf[iMShield].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_bow, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iBow].ltm; break;
    case 2: ret=itemsbuf[iBow2].ltm; break;
    default: ret=0; break;

  }
  tile+=ret;

  ret=current_item(itype_raft, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iRaft].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_ladder, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iLadder].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_book, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iBook].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;
  ret=current_item(itype_magickey, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iMKey].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_bracelet, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iBracelet].ltm; break;
    case 2: ret=itemsbuf[iL2Bracelet].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_flippers, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iFlippers].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_boots, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iBoots].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_hookshot, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iHookshot].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_lens, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iLens].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;
  ret=current_item(itype_hammer, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iHammer].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_dinsfire, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iDinsFire].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_faroreswind, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iFaroresWind].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_nayruslove, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iNayrusLove].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_bomb, true);
  switch(ret)
  {
    case 0: ret=0; break;
    default: ret=itemsbuf[iBombs].ltm; break;
  }
  tile+=ret;

  ret=current_item(itype_sbomb, true);
  switch(ret)
  {
    case 0: ret=0; break;
    default: ret=itemsbuf[iSBomb].ltm; break;
  }
  tile+=ret;

  ret=current_item(itype_clock, true);
  switch(ret)
  {
    case 1: ret=itemsbuf[iClock].ltm; break;
    default: ret=0; break;
  }
  tile+=ret;

  ret=current_item(itype_key, true);
  switch(ret)
  {
    case 0: ret=0; break;
    default: ret=itemsbuf[iKey].ltm; break;
  }
  tile+=ret;

  ret=current_item(itype_map, true);
  switch(ret)
  {
    case 0: ret=0; break;
    default: ret=itemsbuf[iMap].ltm; break;
  }
  tile+=ret;

  ret=current_item(itype_compass, true);
  switch(ret)
  {
    case 0: ret=0; break;
    default: ret=itemsbuf[iCompass].ltm; break;
  }
  tile+=ret;

  ret=current_item(itype_bosskey, true);
  switch(ret)
  {
    case 0: ret=0; break;
    default: ret=itemsbuf[iBossKey].ltm; break;
  }
  tile+=ret;

  ret=current_item(itype_magiccontainer, true);
  switch(ret)
  {
    case 0: ret=0; break;
    default: ret=itemsbuf[iMagicC].ltm; break;
  }
  tile+=ret;

  ret=current_item(itype_triforcepiece, true);
  switch(ret)
  {
    case 0: ret=0; break;
    default: ret=itemsbuf[iTriforce].ltm; break;
  }
  tile+=ret;
  return tile;
}

int dmap_tile_mod()
{
  return 0;
}

void draw_lens_under()
{
  int strike_hint_table[11]=
  {
    mfARROW, mfBOMB, mfBRANG, mfWANDMAGIC,
    mfSWORD, mfREFMAGIC, mfHOOKSHOT,
    mfREFFIREBALL, mfHAMMER, mfSWORDBEAM, mfWAND
  };

  //  int page = tmpscr->cpage;
  {
    int blink_rate=1;
    //    int temptimer=0;
    int tempitem, tempweapon;
    strike_hint=strike_hint_table[strike_hint_counter];
    if (strike_hint_timer>32)
    {
      strike_hint_timer=0;
      strike_hint_counter=((strike_hint_counter+1)%11);
    }
    ++strike_hint_timer;

    for(int i=0; i<176; i++)
    {
      int x = (i & 15) << 4;
      int y = (i & 0xF0) + 56;
      int tempitemx=-16, tempitemy=-16;
      int tempweaponx=-16, tempweapony=-16;

      switch(tmpscr->sflag[i])
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
          if(lensclk&16)
          {
            putcombo(framebuf,x,y,tmpscr->undercombo,tmpscr->undercset);
          }
          if(lensclk&blink_rate)
          {
            if (get_bit(quest_rules,qr_LENSHINTS))
            {
              switch (combobuf[tmpscr->data[i]].type)
              {
                case cPUSH_HEAVY:
                case cPUSH_HW:
                  tempitem=iBracelet;
                  tempitemx=x, tempitemy=y;
                  putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  break;
                case cPUSH_HEAVY2:
                case cPUSH_HW2:
                  tempitem=iL2Bracelet;
                  tempitemx=x, tempitemy=y;
                  putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                  break;
              }
            }
          }
          break;

        case mfWHISTLE:
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iWhistle;
          if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfFAIRY:
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iFairyMoving;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfBCANDLE:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sBCANDLE],tmpscr->secretcset[sBCANDLE]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iBCandle;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfRCANDLE:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sRCANDLE],tmpscr->secretcset[sRCANDLE]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iRCandle;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfWANDFIRE:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sWANDFIRE],tmpscr->secretcset[sWANDFIRE]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iWand;
            tempweapon=wFire;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            else
            {
              tempweaponx=x; tempweapony=y;
            }
            putweapon(framebuf,tempweaponx,tempweapony,tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfDINSFIRE:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sDINSFIRE],tmpscr->secretcset[sDINSFIRE]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iDinsFire;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfARROW:
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            putcombo(framebuf,x,y,tmpscr->secretcombo[sARROW],tmpscr->secretcset[sARROW]);
            tempitem=iArrow;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfSARROW:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sSARROW],tmpscr->secretcset[sSARROW]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iSArrow;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfGARROW:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sGARROW],tmpscr->secretcset[sGARROW]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iGArrow;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfBOMB:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sBOMB],tmpscr->secretcset[sBOMB]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempweapon=wBomb;
            if(lensclk&blink_rate)
            {
              tempweaponx=x; tempweapony=y;
            }
            putweapon(framebuf,tempweaponx,tempweapony+lens_hint_weapon[tempweapon][4],tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
          }
          break;

        case mfSBOMB:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sSBOMB],tmpscr->secretcset[sSBOMB]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempweapon=wSBomb;
            if(lensclk&blink_rate)
            {
              tempweaponx=x; tempweapony=y;
            }
            putweapon(framebuf,tempweaponx,tempweapony+lens_hint_weapon[tempweapon][4],tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
          }
          break;

        case mfARMOS_SECRET:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sSTAIRS],tmpscr->secretcset[sSTAIRS]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
          }
          break;

        case mfBRANG:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sBRANG],tmpscr->secretcset[sBRANG]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iBrang;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfMBRANG:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sMBRANG],tmpscr->secretcset[sMBRANG]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iMBrang;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfFBRANG:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sFBRANG],tmpscr->secretcset[sFBRANG]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iFBrang;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfWANDMAGIC:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sWANDMAGIC],tmpscr->secretcset[sWANDMAGIC]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iWand;
            tempweapon=wMagic;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            else
            {
              tempweaponx=x; tempweapony=y;
              --lens_hint_weapon[wMagic][4];
              if (lens_hint_weapon[wMagic][4]<-8)
              {
                lens_hint_weapon[wMagic][4]=8;
              }
            }
            putweapon(framebuf,tempweaponx,tempweapony+lens_hint_weapon[tempweapon][4],tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfREFMAGIC:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sREFMAGIC],tmpscr->secretcset[sREFMAGIC]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iMShield;
            tempweapon=ewMagic;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            else
            {
              tempweaponx=x; tempweapony=y;
              if (lens_hint_weapon[ewMagic][2]==up)
              {
                --lens_hint_weapon[ewMagic][4];
              }
              else
              {
                ++lens_hint_weapon[ewMagic][4];
              }
              if (lens_hint_weapon[ewMagic][4]>8)
              {
                lens_hint_weapon[ewMagic][2]=up;
              }
              if (lens_hint_weapon[ewMagic][4]<=0)
              {
                lens_hint_weapon[ewMagic][2]=down;
              }
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
            putweapon(framebuf,tempweaponx,tempweapony+lens_hint_weapon[tempweapon][4],tempweapon, 0, lens_hint_weapon[ewMagic][2], lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
          }
          break;

        case mfREFFIREBALL:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sREFFIREBALL],tmpscr->secretcset[sREFFIREBALL]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iMShield;
            tempweapon=ewFireball;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
              tempweaponx=x; tempweapony=y;
              ++lens_hint_weapon[ewFireball][3];
              if (lens_hint_weapon[ewFireball][3]>8)
              {
                lens_hint_weapon[ewFireball][3]=-8;
                lens_hint_weapon[ewFireball][4]=8;
              }
              if (lens_hint_weapon[ewFireball][3]>0)
              {
                ++lens_hint_weapon[ewFireball][4];
              }
              else
              {
                --lens_hint_weapon[ewFireball][4];
              }
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
            putweapon(framebuf,tempweaponx+lens_hint_weapon[tempweapon][3],tempweapony+lens_hint_weapon[ewFireball][4],tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
          }
          break;

        case mfSWORD:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sSWORD],tmpscr->secretcset[sSWORD]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iSword;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfWSWORD:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sWSWORD],tmpscr->secretcset[sWSWORD]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iWSword;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfMSWORD:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sMSWORD],tmpscr->secretcset[sMSWORD]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iMSword;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfXSWORD:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sXSWORD],tmpscr->secretcset[sXSWORD]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iXSword;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfSWORDBEAM:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sSWORDBEAM],tmpscr->secretcset[sSWORDBEAM]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iSword;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 1);
          }
          break;

        case mfWSWORDBEAM:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sWSWORDBEAM],tmpscr->secretcset[sWSWORDBEAM]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iWSword;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 2);
          }
          break;

        case mfMSWORDBEAM:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sMSWORDBEAM],tmpscr->secretcset[sMSWORDBEAM]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iMSword;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 3);
          }
          break;

        case mfXSWORDBEAM:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sXSWORDBEAM],tmpscr->secretcset[sXSWORDBEAM]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iXSword;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 4);
          }
          break;

        case mfHOOKSHOT:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sHOOKSHOT],tmpscr->secretcset[sHOOKSHOT]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iHookshot;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfWAND:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sWAND],tmpscr->secretcset[sWAND]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iWand;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfHAMMER:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sHAMMER],tmpscr->secretcset[sHAMMER]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            tempitem=iHammer;
            if(lensclk&blink_rate)
            {
              tempitemx=x; tempitemy=y;
            }
            putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
          }
          break;

        case mfSTRIKE:
          putcombo(framebuf,x,y,tmpscr->secretcombo[sSTRIKE],tmpscr->secretcset[sSTRIKE]);
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
            switch (strike_hint)
            {
              case mfARROW:
                tempitem=iArrow;
                if(lensclk&blink_rate)
                {
                  tempitemx=x; tempitemy=y;
                }
                putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                break;
              case mfBOMB:
                tempweapon=wBomb;
                if(lensclk&blink_rate)
                {
                  tempweaponx=x; tempweapony=y;
                }
                putweapon(framebuf,tempweaponx,tempweapony+lens_hint_weapon[tempweapon][4],tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
                break;
              case mfBRANG:
                tempitem=iBrang;
                if(lensclk&blink_rate)
                {
                  tempitemx=x; tempitemy=y;
                }
                putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                break;
              case mfWANDMAGIC:
                tempitem=iWand;
                tempweapon=wMagic;
                if(lensclk&blink_rate)
                {
                  tempitemx=x; tempitemy=y;
                }
                else
                {
                  tempweaponx=x; tempweapony=y;
                  --lens_hint_weapon[wMagic][4];
                  if (lens_hint_weapon[wMagic][4]<-8)
                  {
                    lens_hint_weapon[wMagic][4]=8;
                  }
                }
                putweapon(framebuf,tempweaponx,tempweapony+lens_hint_weapon[wMagic][4],tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
                putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                break;
              case mfREFMAGIC:
                tempitem=iMShield;
                tempweapon=ewMagic;
                if(lensclk&blink_rate)
                {
                  tempitemx=x; tempitemy=y;
                }
                else
                {
                  tempweaponx=x; tempweapony=y;
                  if (lens_hint_weapon[ewMagic][2]==up)
                  {
                    --lens_hint_weapon[ewMagic][4];
                  }
                  else
                  {
                    ++lens_hint_weapon[ewMagic][4];
                  }
                  if (lens_hint_weapon[ewMagic][4]>8)
                  {
                    lens_hint_weapon[ewMagic][2]=up;
                  }
                  if (lens_hint_weapon[ewMagic][4]<=0)
                  {
                    lens_hint_weapon[ewMagic][2]=down;
                  }
                }
                putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                putweapon(framebuf,tempweaponx,tempweapony+lens_hint_weapon[ewMagic][4],tempweapon, 0, lens_hint_weapon[ewMagic][2], lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
                break;
              case mfREFFIREBALL:
                tempitem=iMShield;
                tempweapon=ewFireball;
                if(lensclk&blink_rate)
                {
                  tempitemx=x; tempitemy=y;
                  tempweaponx=x; tempweapony=y;
                  ++lens_hint_weapon[ewFireball][3];
                  if (lens_hint_weapon[ewFireball][3]>8)
                  {
                    lens_hint_weapon[ewFireball][3]=-8;
                    lens_hint_weapon[ewFireball][4]=8;
                  }
                  if (lens_hint_weapon[ewFireball][3]>0)
                  {
                    ++lens_hint_weapon[ewFireball][4];
                  }
                  else
                  {
                    --lens_hint_weapon[ewFireball][4];
                  }
                }
                putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                putweapon(framebuf,tempweaponx+lens_hint_weapon[ewFireball][3],tempweapony+lens_hint_weapon[ewFireball][4],tempweapon, 0, up, lens_hint_weapon[tempweapon][0], lens_hint_weapon[tempweapon][1]);
                break;
              case mfSWORD:
                tempitem=iSword;
                if(lensclk&blink_rate)
                {
                  tempitemx=x; tempitemy=y;
                }
                putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                break;
              case mfSWORDBEAM:
                tempitem=iSword;
                if(lensclk&blink_rate)
                {
                  tempitemx=x; tempitemy=y;
                }
                putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 1);
                break;
              case mfHOOKSHOT:
                tempitem=iHookshot;
                if(lensclk&blink_rate)
                {
                  tempitemx=x; tempitemy=y;
                }
                putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                break;
              case mfWAND:
                tempitem=iWand;
                if(lensclk&blink_rate)
                {
                  tempitemx=x; tempitemy=y;
                }
                putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                break;
              case mfHAMMER:
                tempitem=iHammer;
                if(lensclk&blink_rate)
                {
                  tempitemx=x; tempitemy=y;
                }
                putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
                break;
            }
          }
          break;
        case mfARMOS_ITEM:
        case mfDIVE_ITEM:
          if (get_bit(quest_rules,qr_LENSHINTS))
          {
          }
          if(!getmapflag())
                                                            //          putitem2(framebuf,x,y,tmpscr->catchall);
            putitem2(framebuf,x,y,tmpscr->catchall, lens_hint_item[tmpscr->catchall][0], lens_hint_item[tmpscr->catchall][1], 0);
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
          putcombo(framebuf,x,y,tmpscr->secretcombo[(tmpscr->sflag[i])-16+4],
            tmpscr->secretcset[(tmpscr->sflag[i])-16+4]);
          break;

        default:
          if(lensclk&1)
          {
            rectfill(framebuf,x,y,x+15,y+15,WHITE);
          }
          break;
      }
    }

    if(tmpscr->door[0]==dWALK)
      rectfill(framebuf, 120, 16+56, 135, 31+56, WHITE);

    if(tmpscr->door[1]==dWALK)
      rectfill(framebuf, 120, 144+56, 135, 159+56, WHITE);

    if(tmpscr->door[2]==dWALK)
      rectfill(framebuf, 16, 80+56, 31, 95+56, WHITE);

    if(tmpscr->door[3]==dWALK)
      rectfill(framebuf, 224, 80+56, 239, 95+56, WHITE);

    if(tmpscr->door[0]==dBOMB)
    {
      showbombeddoor(0);
    }

    if(tmpscr->door[1]==dBOMB)
    {
      showbombeddoor(1);
    }

    if(tmpscr->door[2]==dBOMB)
    {
      showbombeddoor(2);
    }

    if(tmpscr->door[3]==dBOMB)
    {
      showbombeddoor(3);
    }

    if(tmpscr->stairx + tmpscr->stairy)
    {
      putcombo(framebuf,tmpscr->stairx,tmpscr->stairy+56,tmpscr->secretcombo[sSTAIRS],tmpscr->secretcset[sSTAIRS]);
      if (get_bit(quest_rules,qr_LENSHINTS))
      {
        if(tmpscr->flags&fWHISTLE)
        {
          tempitem=iWhistle;
          int tempitemx=-16; int tempitemy=-16;
          if(lensclk&(blink_rate/4))
          {
            tempitemx=tmpscr->stairx; tempitemy=tmpscr->stairy+56;
          }
          putitem2(framebuf,tempitemx,tempitemy,tempitem, lens_hint_item[tempitem][0], lens_hint_item[tempitem][1], 0);
        }
      }
    }
  }
}

void draw_lens_over()
{
  clear_to_color(tmp_scr, BLACK);
  circlefill(tmp_scr, LinkX()+8, LinkY()+8+56, 60, 0);
  circle(tmp_scr, LinkX()+8, LinkY()+8+56, 62, 0);
  circle(tmp_scr, LinkX()+8, LinkY()+8+56, 65, 0);
  masked_blit(tmp_scr, framebuf, 0, 56, 0, 56, 256, 168);
}

                                                            //----------------------------------------------------------------

void draw_wavy(int amplitude)
{
  BITMAP *wavebuf = create_bitmap_ex(8,288,224);
  clear_to_color(wavebuf,0);
  blit(framebuf,wavebuf,0,0,16,0,256,224);

  int ofs;
                                                            //  int amplitude=8;
                                                            //  int wavelength=4;
  int amp2=168;
  int i=frame%amp2;
  for(int j=0; j<168; j++)
  {
    ofs=0;
    if (j&1)
    {
      ofs=int(sin((double(i+j)*2*PI/amp2))*amplitude);
    }
    else
    {
      ofs-=int(sin((double(i+j)*2*PI/amp2))*amplitude);
    }
    for (int k=0; k<256; k++)
    {
      framebuf->line[j+56][k]=wavebuf->line[j+56][k+ofs+16];
    }
  }
  destroy_bitmap(wavebuf);
}

void draw_fuzzy(int fuzz)
                                                            // draws from right half of scrollbuf to framebuf
{
  int firstx, firsty, xstep, ystep, i, y, dx, dy;
  byte *start, *si, *di;

  if(fuzz<1)
    fuzz = 1;

  xstep = 128%fuzz;
  if(xstep > 0)
    xstep = fuzz-xstep;

  ystep = 112%fuzz;
  if(ystep > 0)
    ystep = fuzz-ystep;

  firsty = 1;

  for(y=0; y<224; )
  {
    start = &(scrollbuf->line[y][256]);

    for(dy=0; dy<ystep && dy+y<224; dy++)
    {
      si = start;
      di = &(framebuf->line[y+dy][0]);
      i = xstep;
      firstx = 1;

      for(dx=0; dx<256; dx++)
      {
        *(di++) = *si;
        if(++i >= fuzz)
        {
          if(!firstx)
            si += fuzz;
          else
          {
            si += fuzz-xstep;
            firstx = 0;
          }
          i = 0;
        }
      }
    }

    if(!firsty)
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
  if((Throttlefps ^ (true && key[KEY_TILDE])))
  {
    while(!myvsync) rest(1);
    //    vsync();
  }
  myvsync=0;
}

void updatescr()
{
	//rest(1);
	if (halt)
	{
		char buf[20];
		int num=0;
		do
		{
			sprintf(buf, "zelda%03d.bmp", ++num);
		} while(num<999 && exists(buf));

		PALETTE tpal;
		get_palette(tpal);
		dump_pal(framebuf);
		if (tmpscr->flags3&fNOSUBSCR)
		{
			BITMAP *ssbuf = create_bitmap_ex(8,256,168);
			clear_to_color(ssbuf,0);
			blit(framebuf,ssbuf,0,56,0,0,256,168);
			save_bitmap(buf,ssbuf,tpal);
			destroy_bitmap(ssbuf);
		}
		else
		{
			save_bitmap(buf,framebuf,tpal);
		}
	}

	if(!Playing)
		black_opening_count=0;

	if(black_opening_count<0) //shape is opening up
	{
		black_opening(framebuf,black_opening_x,black_opening_y,(66+black_opening_count),66);
		if (Advance||(!Paused))
		{
			++black_opening_count;
		}
	}
	else if(black_opening_count>0) //shape is closing
	{
		black_opening(framebuf,black_opening_x,black_opening_y,black_opening_count,66);
		if (Advance||(!Paused))
		{
			--black_opening_count;
		}
	}

	//redo this and waitvsync(), too

	if (triplebuffer_not_available)
	{
		waitvsync();
	}
	else
	{
		//    myvsync=0;
	}


	if(refreshpal)
	{
		refreshpal=false;
		RAMpal[253] = _RGB(0,0,0);
		RAMpal[254] = _RGB(63,63,63);
		set_palette_range(RAMpal,0,255,false);
	}

	show_details();

	if (Link.DrunkClock())
	{
		draw_wavy(Link.DrunkClock()/(MAXDRUNKCLOCK/32));
	}
	BITMAP *panorama = NULL;

	bool nosubscr = (tmpscr->flags3&fNOSUBSCR) != 0;

	if (nosubscr)
	{
		panorama = create_bitmap_ex(8,256,224);
		rectfill(panorama,0,0,255,56/2,0);
		rectfill(panorama,0,168+56/2,255,168+56-1,0);
		blit(framebuf,panorama,0,56,0,56/2,256,224-56);
	}
	BITMAP *target;
	bool dontusetb = triplebuffer_not_available ||
		!(Throttlefps ^ (true && key[KEY_TILDE]));
	if(dontusetb)
		target=screen;
	else
		target=tb_page[curr_tb_page];

	if (scanlines && sbig)
	{
		BITMAP *scanlinesbmp = create_bitmap_ex(8,512,448);
		stretch_blit(nosubscr?panorama:framebuf,scanlinesbmp,0,0,256,224,0,0    ,512,448);
		for (int i=0; i<224; ++i)
		{
			hline(scanlinesbmp, 0, i*2+1, 512, BLACK);
		}
		blit(scanlinesbmp,target,0,0,scrx+32-128,scry+8-112,512,448);

		destroy_bitmap(scanlinesbmp);
	}
	else if (sbig)
	{
		//jman2050, what are you doing with this line below?
		//exists((char*)panorama); exists((char*)screen);
		//did you mean
		//assert((char*)panorama); assert((char*)screen);
		BITMAP *tempscreen = create_bitmap_ex(8,512,448);
		clear_bitmap(tempscreen);
		stretch_blit(nosubscr?panorama:framebuf,tempscreen,0,0,256,224,0,0,512,448);
		blit(tempscreen,target,0,0,scrx+32-128,scry+8-112,512,448);
		destroy_bitmap(tempscreen);
	}
	else
	{
		blit(nosubscr?panorama:framebuf,target,0,0,scrx+32,scry+8,256,224);
	}

	if (!dontusetb)
	{
		if(!poll_scroll())
		{
			request_video_bitmap(tb_page[curr_tb_page]);
			curr_tb_page=(curr_tb_page+1)%3;
			clear_to_color(tb_page[curr_tb_page],BLACK);
		}
		waitvsync();
	}


	if(ShowFPS)
		show_fps();


	if(Paused)
		show_paused();

	if(details)
	{
		textprintf_ex(screen,font,0,SCREEN_H-8,254,BLACK,"%-6d (%s)", idle_count, time_str_long(idle_count));
	}

	if(panorama!=NULL) destroy_bitmap(panorama);

	++framecnt;
}

//----------------------------------------------------------------

void f_Quit(int type)
{
  if(type==qQUIT && !Playing)
    return;

  music_pause();
  pause_all_sfx();
  Quit=type;

  eat_buttons();
  if(key[KEY_ESC])
    key[KEY_ESC]=0;
}

//----------------------------------------------------------------

void syskeys()
{
  if(ReadKey(KEY_F1))
  {
    if (key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
    {
      halt=!halt;
      //zinit.subscreen=(zinit.subscreen+1)%ssdtMAX;
    }
    else
    {
      Throttlefps=!Throttlefps;
      logic_counter=0;
    }
  }

  if(ReadKey(KEY_F2))    ShowFPS=!ShowFPS;
  if(ReadKey(KEY_F3))    TransLayers=!TransLayers;
  if(ReadKey(KEY_F4) && Playing)  { Paused=true; Advance=true; }
  if(ReadKey(KEY_F5) && Playing)  Paused=!Paused;
  if(ReadKey(KEY_F6))    f_Quit(qQUIT);
  if(ReadKey(KEY_F7))    f_Quit(qRESET);
  if(ReadKey(KEY_F8))    f_Quit(qEXIT);
  if(ReadKey(KEY_F9))    HeartBeep=!HeartBeep;

  while(Playing && keypressed())
    readkey();
}

                                                            // 99*360 + 59*60
#define MAXTIME  21405240

void advanceframe()
{
  if((Throttlefps ^ (true && key[KEY_TILDE])))
  {

    while(!logic_counter) rest(1);
    logic_counter=0;
  }
  if (zcmusic!=NULL)
  {
    zcmusic_poll();
  }
  while(Paused && !Advance && !Quit)
  {
    // have to call this, otherwise we'll get an infinite loop
    syskeys();
    // to keep fps constant
    updatescr();
    // to keep music playing
    if (zcmusic!=NULL)
    {
      zcmusic_poll();
    }
  }
  if(Quit)
    return;

  if(Playing && game.time<MAXTIME)
    ++game.time;

  Advance=false;
  ++frame;

  syskeys();
  updatescr();
  //textprintf_ex(screen,font,0,72,254,BLACK,"%d %d", lastentrance, lastentrance_dmap);
  sfx_cleanup();
}

void zapout()
{
                                                            // draw screen on right half of scrollbuf
                                                            /*
    putsubscr(framebuf,0,0);
    blit(scrollbuf,framebuf,0,0,0,56,256,168);
    do_layer(framebuf, 0, tmpscr, 0, 0, 2);
    do_layer(framebuf, 1, tmpscr, 0, 0, 2);
    do_layer(framebuf, -2, tmpscr, 0, 0, 2);
    do_layer(framebuf, 2, tmpscr, 0, 0, 2);
    do_layer(framebuf, 3, tmpscr, 0, 0, 2);
    do_layer(framebuf, -1, tmpscr, 0, 0, 2);
    do_layer(framebuf, 4, tmpscr, 0, 0, 2);
    do_layer(framebuf, 5, tmpscr, 0, 0, 2);
  */
  blit(framebuf,scrollbuf,0,0,256,0,256,224);

                                                            // zap out
  for(int i=1; i<=24; i++)
  {
    draw_fuzzy(i);
    syskeys();
    advanceframe();
    if(Quit)
    {
      break;
    }
  }
}

void zapin()
{
                                                            // draw screen on right half of scrollbuf
                                                            /*
    putsubscr(framebuf,0,0);
    blit(scrollbuf,framebuf,0,0,0,56,256,168);
    do_layer(framebuf, 0, tmpscr, 0, 0, 2);
    do_layer(framebuf, 1, tmpscr, 0, 0, 2);
    do_layer(framebuf, -2, tmpscr, 0, 0, 2);
    do_layer(framebuf, 2, tmpscr, 0, 0, 2);
    do_layer(framebuf, 3, tmpscr, 0, 0, 2);
    do_layer(framebuf, -1, tmpscr, 0, 0, 2);
    do_layer(framebuf, 4, tmpscr, 0, 0, 2);
    do_layer(framebuf, 5, tmpscr, 0, 0, 2);
  */
  draw_screen(tmpscr, 0, 0);
  putsubscr(framebuf,0,0);
  blit(framebuf,scrollbuf,0,0,256,0,256,224);

                                                            // zap out
  for(int i=24; i>=1; i--)
  {
    draw_fuzzy(i);
    syskeys();
    advanceframe();
    if(Quit)
    {
      break;
    }
  }
}


void wavyout()
{
  draw_screen(tmpscr, 0, 0);
  putsubscr(framebuf,0,0);

  BITMAP *wavebuf = create_bitmap_ex(8,288,224);
  clear_to_color(wavebuf,0);
  blit(framebuf,wavebuf,0,0,16,0,256,224);

  PALETTE wavepal;

  int ofs;
  int amplitude=8;

  int wavelength=4;
  double palpos=0, palstep=4, palstop=126;
  for(int i=0; i<168; i+=wavelength)
  {
    for (int l=0; l<256; l++)
    {
      wavepal[l].r=vbound(int(RAMpal[l].r+((palpos/palstop)*(63-RAMpal[l].r))),0,63);
      wavepal[l].g=vbound(int(RAMpal[l].g+((palpos/palstop)*(63-RAMpal[l].g))),0,63);
      wavepal[l].b=vbound(int(RAMpal[l].b+((palpos/palstop)*(63-RAMpal[l].b))),0,63);
    }
    palpos+=palstep;
    if (palpos>=0)
    {
      set_palette(wavepal);
    }
    else
    {
      set_palette(RAMpal);
    }
    for(int j=0; j<168; j++)
    {
      for (int k=0; k<256; k++)
      {
        ofs=0;
        if ((j<i)&&(j&1))
        {
          ofs=int(sin((double(i+j)*2*PI/168.0))*amplitude);
        }
        framebuf->line[j+56][k]=wavebuf->line[j+56][k+ofs+16];
      }
    }
    syskeys();
    advanceframe();
                                                            //    animate_combos();
    if(Quit)
      break;
  }
  destroy_bitmap(wavebuf);
}

void wavyin()
{
  draw_screen(tmpscr, 0, 0);
  putsubscr(framebuf,0,0);

  BITMAP *wavebuf = create_bitmap_ex(8,288,224);
  clear_to_color(wavebuf,0);
  blit(framebuf,wavebuf,0,0,16,0,256,224);

  PALETTE wavepal;
  loadfullpal();
  loadlvlpal(DMaps[currdmap].color);
  ringcolor();
  refreshpal=false;
  int ofs;
  int amplitude=8;
  int wavelength=4;
  double palpos=168, palstep=4, palstop=126;
  for(int i=0; i<168; i+=wavelength)
  {
    for (int l=0; l<256; l++)
    {
      wavepal[l].r=vbound(int(RAMpal[l].r+((palpos/palstop)*(63-RAMpal[l].r))),0,63);
      wavepal[l].g=vbound(int(RAMpal[l].g+((palpos/palstop)*(63-RAMpal[l].g))),0,63);
      wavepal[l].b=vbound(int(RAMpal[l].b+((palpos/palstop)*(63-RAMpal[l].b))),0,63);
    }
    palpos-=palstep;

    if (palpos>=0)
    {
      set_palette(wavepal);
    }
    else
    {
      set_palette(RAMpal);
    }
    for(int j=0; j<168; j++)
    {
      for (int k=0; k<256; k++)
      {
        ofs=0;
        if ((j<(167-i))&&(j&1))
        {
          ofs=int(sin((double(i+j)*2*PI/168.0))*amplitude);
        }
        framebuf->line[j+56][k]=wavebuf->line[j+56][k+ofs+16];
      }
    }
    syskeys();
    advanceframe();
                                                            //    animate_combos();

    if(Quit)
      break;
  }
  destroy_bitmap(wavebuf);
}

void blackscr(int fcnt,bool showsubscr)
{
  reset_pal_cycling();
  while(fcnt>0)
  {
    clear_bitmap(framebuf);
    if(showsubscr)
    {
      putsubscr(framebuf,0,0);
    }
    syskeys();
    advanceframe();
    if(Quit)
      break;;
    --fcnt;
  }
}

void openscreen()
{
  reset_pal_cycling();
  black_opening_count=0;

  if(COOLSCROLL)
  {
    open_black_opening(LinkX()+8, LinkY()+8+56, true);
    return;
  }
  else
  {
    Link.setDontDraw(true);
    show_subscreen_dmap_dots=false;
    show_subscreen_numbers=false;
    show_subscreen_items=false;
    show_subscreen_life=false;
  }

  int x=128;

  for(int i=0; i<80; i++)
  {
    draw_screen(tmpscr, 0, 0);
    putsubscr(framebuf,0,0);
    x=128-(((i*128/80)/8)*8);
    if (x>0)
    {
      rectfill(framebuf,0,56,x,223,0);
      rectfill(framebuf,256-x,56,255,223,0);
    }
//    x=((80-i)/2)*4;
/*
    --x;
    switch(++c)
    {
      case 5: c=0;
      case 0:
      case 2:
      case 3: --x; break;
    }
*/
    syskeys();
    advanceframe();
    if(Quit)
    {
      break;
    }
  }
  Link.setDontDraw(false);
  show_subscreen_items=true;
  show_subscreen_dmap_dots=true;
}

int TriforceCount()
{
  int c=0;
  for(int i=1; i<=8; i++)
    if(game.lvlitems[i]&liTRIFORCE)
      ++c;
  return c;
}

int onContinue()
{
  return D_CLOSE;
}

int onEsc()
{
  return key[KEY_ESC]?D_CLOSE:D_O_K;
}

int onVsync()
{
  Throttlefps = !Throttlefps;
  return D_O_K;
}

int onTransLayers()
{
  TransLayers = !TransLayers;
  return D_O_K;
}

int onShowFPS()
{
  ShowFPS = !ShowFPS;
  scare_mouse();

  if(ShowFPS)
    show_fps();
  else if(sbig)
    stretch_blit(fps_undo,screen,0,0,64,16,scrx+40-120,scry+216+96,128,32);
  else
    blit(fps_undo,screen,0,0,scrx+40,scry+216,64,16);
  if(Paused)
    show_paused();

  unscare_mouse();
  return D_O_K;
}

bool is_Fkey(int k)
{
  switch(k)
  {
    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
    case KEY_F5:
    case KEY_F6:
    case KEY_F7:
    case KEY_F8:
    case KEY_F9:
    case KEY_F10:
    case KEY_F11:
    case KEY_F12:
      return true;
  }
  return false;
}

void kb_getkey(DIALOG *d)
{
  d->flags|=D_SELECTED;

  scare_mouse();
  jwin_button_proc(MSG_DRAW,d,0);
  jwin_draw_win(screen, (resx-160)/2, (resy-48)/2, 160, 48, FR_WIN);
                                                            //  text_mode(vc(11));
  textout_centre_ex(screen, font, "Press a key", resx/2, resy/2 - 8, vc(0),vc(11));
  textout_centre_ex(screen, font, "ESC to cancel", resx/2, resy/2, vc(0),vc(11));
  unscare_mouse();

  clear_keybuf();
  int k = next_press_key();
  clear_keybuf();

                                                            //shnarf
                                                            //47=f1
                                                            //59=esc
  if(k>0 && k<123 && !((k>46)&&(k<60)))
    *((int*)d->dp3) = k;


  d->flags&=~D_SELECTED;
}

int d_kbutton_proc(int msg,DIALOG *d,int c)
{
  switch(msg)
  {
    case MSG_KEY:
    case MSG_CLICK:

      kb_getkey(d);
      while(gui_mouse_b())
        clear_keybuf();
      return D_REDRAW;
  }

  return jwin_button_proc(msg,d,c);
}

void j_getbtn(DIALOG *d)
{
  d->flags|=D_SELECTED;
  scare_mouse();
  jwin_button_proc(MSG_DRAW,d,0);
  jwin_draw_win(screen, (resx-160)/2, (resy-48)/2, 160, 48, FR_WIN);
                                                            //  text_mode(vc(11));
  int y = resy/2 - 12;
  textout_centre_ex(screen, font, "Press a button", resx/2, y, vc(0),vc(11));
  textout_centre_ex(screen, font, "ESC to cancel", resx/2, y+8, vc(0),vc(11));
  textout_centre_ex(screen, font, "SPACE to disable", resx/2, y+16, vc(0),vc(11));
  unscare_mouse();

  int b = next_press_btn();
  if(b>=0)
    *((int*)d->dp3) = b;

  d->flags&=~D_SELECTED;
}

int d_jbutton_proc(int msg,DIALOG *d,int c)
{
  switch(msg)
  {
    case MSG_KEY:
    case MSG_CLICK:

      j_getbtn(d);
      while(gui_mouse_b())
        clear_keybuf();
      return D_REDRAW;
  }

  return jwin_button_proc(msg,d,c);
}

                                                            //shnarf
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

char *pan_str[4] = { "MONO", " 1/2", " 3/4", "FULL" };
//char zcmusic_bufsz = 10;

static char str_a[44],str_b[44],str_s[44],str_m[16],str_l[16],str_r[16],str_p[16];

int d_stringloader(int msg,DIALOG *d,int c)
{
  if(msg==MSG_DRAW)
  {
    switch(d->w)
    {
      case 0:
        sprintf(str_a,"%d\n%s",Akey,key_str[Akey]);
        sprintf(str_b,"%d\n%s",Bkey,key_str[Bkey]);
        sprintf(str_s,"%d\n%s",Skey,key_str[Skey]);
        sprintf(str_l,"%d\n%s",Lkey,key_str[Lkey]);
        sprintf(str_r,"%d\n%s",Rkey,key_str[Rkey]);
        sprintf(str_p,"%d\n%s",Mkey,key_str[Mkey]);
        break;

      case 1:
        sprintf(str_a,"%d\n%s",DUkey,key_str[DUkey]);
        sprintf(str_b,"%d\n%s",DDkey,key_str[DDkey]);
        sprintf(str_l,"%d\n%s",DLkey,key_str[DLkey]);
        sprintf(str_r,"%d\n%s",DRkey,key_str[DRkey]);
        break;

      case 3:
        sprintf(str_a,"%3d",midi_volume);
        sprintf(str_b,"%3d",digi_volume);
        strcpy(str_s,pan_str[pan_style]);
        sprintf(str_m,"%2dKB",zcmusic_bufsz);
        break;
    }
  }
  return D_O_K;
}

int set_vol(void *dp3, int d2)
{
  switch(((int*)dp3)[0])
  {
    case 0:  midi_volume = min(d2<<3,255); break;
    case 1:  digi_volume = min(d2<<3,255); break;
  }
  scare_mouse();
  // text_mode(vc(11));
  textprintf_right_ex(screen,font,((int*)dp3)[1],((int*)dp3)[2],jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%3d",min(d2<<3,255));
  unscare_mouse();
  return D_O_K;
}

int set_pan(void *dp3, int d2)
{
  pan_style = vbound(d2,0,3);
  scare_mouse();
                                                            // text_mode(vc(11));
  textout_right_ex(screen,font,pan_str[pan_style],((int*)dp3)[1],((int*)dp3)[2],jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
  unscare_mouse();
  return D_O_K;
}

int set_buf(void *dp3, int d2)
{
  scare_mouse();
                                                            // text_mode(vc(11));
  zcmusic_bufsz = d2 + 1;
  textprintf_right_ex(screen,font,((int*)dp3)[1],((int*)dp3)[2],jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%2dKB",zcmusic_bufsz);
  unscare_mouse();
  return D_O_K;
}

int xtoi(char *hexstr)
{
  int val=0;
  while(isxdigit(*hexstr))
  {
    val<<=4;
    if(*hexstr<='9')
      val += *hexstr-'0';
    else val+= ((*hexstr)|0x20)-'a'+10;
    ++hexstr;
  }
  return val;
}

void LogVidMode()
{
  #ifdef ALLEGRO_DOS
  switch(VidMode)
  {
    case GFX_MODEX:  sprintf(str_a,"VGA Mode X"); break;
    case GFX_VESA1:  sprintf(str_a,"VESA 1.x"); break;
    case GFX_VESA2B: sprintf(str_a,"VESA2 Banked"); break;
    case GFX_VESA2L: sprintf(str_a,"VESA2 Linear"); break;
    case GFX_VESA3:  sprintf(str_a,"VESA3"); break;
    default:         sprintf(str_a,"Unknown... ?"); break;
  }
  #elif defined(ALLEGRO_WINDOWS)
  switch(VidMode)
  {
    case GFX_DIRECTX:      sprintf(str_a,"DirectX Hardware Accelerated"); break;
    case GFX_DIRECTX_SOFT: sprintf(str_a,"DirectX Software Accelerated"); break;
    case GFX_DIRECTX_SAFE: sprintf(str_a,"DirectX Safe"); break;
    case GFX_DIRECTX_WIN:  sprintf(str_a,"DirectX Windowed"); break;
    case GFX_GDI:          sprintf(str_a,"GDI"); break;
    default:               sprintf(str_a,"Unknown... ?"); break;
  }
  #elif defined(ALLEGRO_MACOSX)
  switch(VidMode)
  {
    case GFX_SAFE:               sprintf(str_a,"MacOS X Safe"); break;
    case GFX_QUARTZ_FULLSCREEN:  sprintf(str_a,"MacOS X Fullscreen Quartz"); break;
    case GFX_QUARTZ_WINDOW:      sprintf(str_a,"MacOS X Windowed Quartz"); break;
    default:                     sprintf(str_a,"Unknown... ?"); break;
  }
  #elif defined(ALLEGRO_LINUX)
  switch(VidMode)
  {
    case GFX_AUTODETECT_WINDOWED:  sprintf(str_a,"Autodetect Windowed"); break;
    default:               sprintf(str_a,"Unknown... ?"); break;
  }
  #endif

  sprintf(str_b,"%dx%d %d-bit",resx,resy, get_color_depth());
  al_trace("Video Mode set: %s (%s)\n",str_a,str_b);
}

void color_layer(RGB *src,RGB *dest,char r,char g,char b,char pos,int from,int to)
{
  PALETTE tmp;
  for(int i=0; i<256; i++)
  {
    tmp[i].r=r;
    tmp[i].g=g;
    tmp[i].b=b;
  }
  fade_interpolate(src,tmp,dest,pos,from,to);
}

/*void game_pal()
{
  clear_to_color(screen,BLACK);
  set_palette_range(RAMpal,0,255,false);
}*/

void music_pause()
{
                                                            //al_pause_duh(tmplayer);
  zcmusic_pause(zcmusic, ZCM_PAUSE);
  midi_pause();
}

void music_resume()
{
                                                            //al_resume_duh(tmplayer);
  zcmusic_pause(zcmusic, ZCM_RESUME);
  midi_resume();
}

void music_stop()
{
                                                            //al_stop_duh(tmplayer);
                                                            //unload_duh(tmusic);
                                                            //tmusic=NULL;
                                                            //tmplayer=NULL;
  zcmusic_stop(zcmusic);
  zcmusic_unload_file(zcmusic);
  stop_midi();
}

void fix_dialogs()
{
  int x = scrx-(sbig?160:0);
  int y = scry-(sbig?120:0);
  if(x>0) x+=3;
  if(y>0) y+=3;
  if(x<0) x=0;
  if(y<0) y=0;

  jwin_center_dialog(gamemode_dlg);
}

/*****************************/
/**** Custom Sound System ****/
/*****************************/

inline int mixvol(int v1,int v2)
{
  return (min(v1,255)*min(v2,255)) >> 8;
}

void jukebox(int index,int loop)
{
  music_stop();
  if(index<0)         index=MAXMUSIC-1;
  if(index>=MAXMUSIC) index=0;

  music_stop();
  set_volume(-1, mixvol(tunes[index].volume,midi_volume>>1));
  play_midi(tunes[index].midi,loop);
  if(tunes[index].start>0)
    midi_seek(tunes[index].start);

  midi_loop_end = tunes[index].loop_end;
  midi_loop_start = tunes[index].loop_start;

  currmidi=index;
  master_volume(digi_volume,midi_volume);
}

void jukebox(int index)
{
  if(index<0)         index=MAXMUSIC-1;
  if(index>=MAXMUSIC) index=0;

  // do nothing if it's already playing
  if(index==currmidi && midi_pos>=0)
    return;

  jukebox(index,tunes[index].loop);
}

void play_DmapMusic()
{
  static char tfile[2048];
  bool domidi=false;
  if (DMaps[currdmap].tmusic[0]!=0)
  {
    if((zcmusic==NULL) || (strcmp(tfile,DMaps[currdmap].tmusic)!=0))
    {
      char tmfname[2048];

      if(zcmusic != NULL)
      {
        zcmusic_stop(zcmusic);
        zcmusic_unload_file(zcmusic);
        zcmusic = NULL;
      }
      tmfname[0] = 0;
#ifdef ALLEGRO_MACOSX
      sprintf(tmfname, "../");
#endif
      strcat(tmfname, DMaps[currdmap].tmusic);

      zcmusic = (ZCMUSIC*)zcmusic_load_file(tmfname);

      if (zcmusic!=NULL)
      {
        stop_midi();
        strcpy(tfile,DMaps[currdmap].tmusic);
        zcmusic_play(zcmusic, midi_volume);
      }
      else
      {
        tfile[0] = 0;
        domidi=true;
      }
    }
  }
  else
  {
    domidi=true;
  }

  if (domidi)
  {
    int m=DMaps[currdmap].midi;
    switch(m)
    {
      case 1: jukebox(MUSIC_OVERWORLD); break;
      case 2: jukebox(MUSIC_DUNGEON); break;
      case 3: jukebox(MUSIC_LEVEL9); break;
      default:
        if(m>=4 && m<4+MAXMIDIS)
          jukebox(m-4+MUSIC_COUNT);
        else
          music_stop();
    }
  }
}

void master_volume(int dv,int mv)
{
  if(dv>=0) digi_volume=max(min(dv,255),0);
  if(mv>=0) midi_volume=max(min(mv,255),0);
  int i = min(max(currmidi,0),MAXMUSIC-1);
  set_volume(digi_volume,mixvol(tunes[i].volume,midi_volume));
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
  for(int i=0; i<WAV_COUNT; i++)
    sfx_voice[i]=-1;
  for(int i=0; i<MUSIC_COUNT; i++)
    tunes[i].midi = (MIDI*)mididata[i].dat;
  master_volume(digi_volume,midi_volume);
}

                                                            // returns number of voices currently allocated
int sfx_count()
{
  int c=0;
  for(int i=0; i<WAV_COUNT; i++)
    if(sfx_voice[i]!=-1)
      ++c;
  return c;
}

                                                            // clean up finished samples
void sfx_cleanup()
{
  for(int i=0; i<WAV_COUNT; i++)
    if(sfx_voice[i]!=-1 && voice_get_position(sfx_voice[i])<0)
  {
    deallocate_voice(sfx_voice[i]);
    sfx_voice[i]=-1;
  }
}

                                                            // allocates a voice for the sample "wav_index" (index into zelda.dat)
                                                            // if a voice is already allocated (and/or playing), then it just returns true
                                                            // Returns true:  voice is allocated
                                                            //         false: unsuccessful
bool sfx_init(int index)
{
                                                            // check index
  if(index<0 || index>=WAV_COUNT)
    return false;

  if(sfx_voice[index]==-1)
  {
                                                            // allocate voice
                                                            /*
        if(index!=WAV_REFILL)
          sfx_voice[index]=allocate_voice((SAMPLE*)sfxdata[index].dat);
        else
          sfx_voice[index]=allocate_voice(&wav_refill);
    */
    sfx_voice[index]=allocate_voice((SAMPLE*)sfxdata[index].dat);
  }

  return sfx_voice[index] != -1;
}

                                                            // plays an sfx sample
void sfx(int index,int pan,bool loop)
{
  if(!sfx_init(index))
    return;

  voice_set_playmode(sfx_voice[index],loop?PLAYMODE_LOOP:PLAYMODE_PLAY);
  voice_set_pan(sfx_voice[index],pan);

  int pos = voice_get_position(sfx_voice[index]);
  voice_set_position(sfx_voice[index],0);
  if(pos<=0)
    voice_start(sfx_voice[index]);
}

                                                            // start it (in loop mode) if it's not already playing,
                                                            // otherwise just leave it in its current position
void cont_sfx(int index)
{
  if(!sfx_init(index))
    return;

  if(voice_get_position(sfx_voice[index])<=0)
  {
    voice_set_position(sfx_voice[index],0);
    voice_set_playmode(sfx_voice[index],PLAYMODE_LOOP);
    voice_start(sfx_voice[index]);
  }
}

                                                            // adjust parameters while playing
void adjust_sfx(int index,int pan,bool loop)
{
  if(index<0 || index>=WAV_COUNT || sfx_voice[index]==-1)
    return;

  voice_set_playmode(sfx_voice[index],loop?PLAYMODE_LOOP:PLAYMODE_PLAY);
  voice_set_pan(sfx_voice[index],pan);
}

                                                            // pauses a voice
void pause_sfx(int index)
{
  if(index>=0 && index<WAV_COUNT && sfx_voice[index]!=-1)
    voice_stop(sfx_voice[index]);
}

                                                            // resumes a voice
void resume_sfx(int index)
{
  if(index>=0 && index<WAV_COUNT && sfx_voice[index]!=-1)
    voice_start(sfx_voice[index]);
}

                                                            // pauses all active voices
void pause_all_sfx()
{
  for(int i=0; i<WAV_COUNT; i++)
    if(sfx_voice[i]!=-1)
      voice_stop(sfx_voice[i]);
}

                                                            // resumes all paused voices
void resume_all_sfx()
{
  for(int i=0; i<WAV_COUNT; i++)
    if(sfx_voice[i]!=-1)
      voice_start(sfx_voice[i]);
}

                                                            // stops an sfx and deallocates the voice
void stop_sfx(int index)
{
  if(index<0 || index>=WAV_COUNT)
    return;

  if(sfx_voice[index]!=-1)
  {
    deallocate_voice(sfx_voice[index]);
    sfx_voice[index]=-1;
  }
}

void kill_sfx()
{
  for(int i=0; i<WAV_COUNT; i++)
    if(sfx_voice[i]!=-1)
  {
    deallocate_voice(sfx_voice[i]);
    sfx_voice[i]=-1;
  }
}

int pan(int x)
{
  switch(pan_style)
  {
    case 0: return 128;
    case 1: return vbound((x>>1)+68,0,255);
    case 2: return vbound(((x*3)>>2)+36,0,255);
  }
  return vbound(x,0,255);
}

                                                            /*******************************/
                                                            /******* Input Handlers ********/
                                                            /*******************************/

int next_press_key()
{
  char k[128];
  for(int i=0; i<128; i++)
    k[i]=key[i];
  do
  {
    for(int i=0; i<128; i++)
      if(key[i]!=k[i])
        return i;
  } while(1);
}

int next_press_btn()
{
  clear_keybuf();
  while(1)
  {
    if(keypressed())
    {
      switch(readkey()>>8)
      {
        case KEY_ESC:   return -1;
        case KEY_SPACE: return 0;
      }
    }
  }
}

static bool rButton(bool(proc)(),bool &flag)
{
  if(!proc())
    flag=false;
  else if(!flag)
  {
    flag=true;
    return true;
  }
  return false;
}

bool Up()     { return key[DUkey]; }
bool Down()   { return key[DDkey]; }
bool Left()   { return key[DLkey]; }
bool Right()  { return key[DRkey]; }
bool cAbtn()  { return key[Akey]; }
bool cBbtn()  { return key[Bkey]; }
bool cSbtn()  { return key[Skey]; }
bool cEbtn()  { return key[KEY_ESC]; }
bool cLbtn()  { return key[Lkey]; }
bool cRbtn()  { return key[Rkey]; }
bool cMbtn()  { return key[Mkey]; }

bool rUp()    { return rButton(Up,Udown); }
bool rDown()  { return rButton(Down,Ddown); }
bool rLeft()  { return rButton(Left,Ldown); }
bool rRight() { return rButton(Right,Rdown); }
bool rAbtn()  { return rButton(cAbtn,Adown); }
bool rBbtn()  { return rButton(cBbtn,Bdown); }
bool rSbtn()  { return rButton(cSbtn,Sdown); }
bool rEbtn()  { return rButton(cEbtn,Mdown); }
bool rLbtn()  { return rButton(cLbtn,LBdown); }
bool rRbtn()  { return rButton(cRbtn,RBdown); }
bool rMbtn()  { return rButton(cMbtn,Pdown); }

bool drunk()
{
                                                            //  return ((!(frame%((rand()%100)+1)))&&(rand()%MAXDRUNKCLOCK<Link.DrunkClock()));
  return false;
}

bool DrunkUp()     { return drunk()?(rand()%2)?0:!Up():Up(); }
bool DrunkDown()   { return drunk()?(rand()%2)?0:!Down():Down(); }
bool DrunkLeft()   { return drunk()?(rand()%2)?0:!Left():Left(); }
bool DrunkRight()  { return drunk()?(rand()%2)?0:!Right():Right(); }
bool DrunkcAbtn()  { return drunk()?(rand()%2)?0:!cAbtn():cAbtn(); }
bool DrunkcBbtn()  { return drunk()?(rand()%2)?0:!cBbtn():cBbtn(); }
bool DrunkcSbtn()  { return drunk()?(rand()%2)?0:!cSbtn():cSbtn(); }
bool DrunkcEbtn()  { return drunk()?(rand()%2)?0:!cEbtn():cEbtn(); }
bool DrunkcLbtn()  { return drunk()?(rand()%2)?0:!cLbtn():cLbtn(); }
bool DrunkcRbtn()  { return drunk()?(rand()%2)?0:!cRbtn():cRbtn(); }
bool DrunkcMbtn()  { return drunk()?(rand()%2)?0:!cMbtn():cMbtn(); }

bool DrunkrUp()    { return drunk()?(rand()%2)?0:!rUp():rUp(); }
bool DrunkrDown()  { return drunk()?(rand()%2)?0:!rDown():rDown(); }
bool DrunkrLeft()  { return drunk()?(rand()%2)?0:!rLeft():rLeft(); }
bool DrunkrRight() { return drunk()?(rand()%2)?0:!rRight():rRight(); }
bool DrunkrAbtn()  { return drunk()?(rand()%2)?0:!rAbtn():rAbtn(); }
bool DrunkrBbtn()  { return drunk()?(rand()%2)?0:!rBbtn():rBbtn(); }
bool DrunkrSbtn()  { return drunk()?(rand()%2)?0:!rSbtn():rSbtn(); }
bool DrunkrEbtn()  { return drunk()?(rand()%2)?0:!rEbtn():rEbtn(); }
bool DrunkrLbtn()  { return drunk()?(rand()%2)?0:!rLbtn():rLbtn(); }
bool DrunkrRbtn()  { return drunk()?(rand()%2)?0:!rRbtn():rRbtn(); }
bool DrunkrMbtn()  { return drunk()?(rand()%2)?0:!rMbtn():rMbtn(); }

void eat_buttons()
{
  rAbtn();
  rBbtn();
  rSbtn();
  rEbtn();
  rLbtn();
  rRbtn();
  rMbtn();
}

bool ReadKey(int k)
{
  if(key[k])
  {
    key[k]=0;
    return true;
  }
  return false;
}

void zc_putpixel(int layer, int x, int y, int cset, int color, int timer)
{
  particles.add(new particle(fix(x), fix(y), layer, cset, color, timer));
}

                                                            /*** end of zc_sys.cc ***/
