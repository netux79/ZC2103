//--------------------------------------------------------
//  Zelda Classic
//  by Jeremy Craner, 1999-2000
//
//  guys.cc
//
//  "Guys" code (and other related stuff) for zelda.cc
//
//  Still has some hardcoded stuff that should be moved
//  out into defdata.cc for customizing the enemies.
//
//--------------------------------------------------------

#include "guys.h"
#include "zelda.h"
#include "zsys.h"
#include "maps.h"
#include "link.h"
#include "subscr.h"
#include <string.h>
#include <stdio.h>

extern LinkClass   Link;
extern sprite_list  guys, items, Ewpns, Lwpns, Sitems, chainlinks, decorations;

bool repaircharge;
bool adjustmagic;
bool learnslash;
int itemindex;
int wallm_load_clk=0;
int sle_x,sle_y,sle_cnt,sle_clk;
int vhead=0;
int guyindex=0;

bool hasBoss();
void never_return(int index);

bool can_do_clock()
{
  if(watch || hasBoss() || (get_bit(quest_rules,qr_NOCLOCKS))) return false;
  if(items.idFirst(iClock)>=0) return false;
  return true;
}

bool m_walkflag(int x,int y,int special)
{
  int yg = (special==spw_floater)?8:0;

  if(x<16 || y<16-yg || x>=240 || y>=160)
    return true;

  if(isdungeon() || special==spw_wizzrobe)
  {
    if(y<32-yg || y>=144)
      return true;
    if(x<32 || x>=224)
      if(special!=spw_door)                                 // walk in door way
        return true;
  }

  switch(special)
  {
    case spw_clipbottomright: if(y>=128) return true;
    case spw_clipright: if(x>=208) return true; break;
    case spw_wizzrobe:
    case spw_floater: return false;
  }

  x&=(special==spw_halfstep)?(~7):(~15);
  y&=(special==spw_halfstep)?(~7):(~15);

  if(special==spw_water)
    //    return water_walkflag(x,y+8,1) && water_walkflag(x+8,y+8,1);
    return water_walkflag(x,y+8,1) || water_walkflag(x+8,y+8,1);

  //  return _walkflag(x,y+8,1) && _walkflag(x+8,y+8,1);
  return _walkflag(x,y+8,1) || _walkflag(x+8,y+8,1) ||
    COMBOTYPE(x,y+8)==cPIT || COMBOTYPE(x+8,y+8)==cPIT;
}

int link_on_wall()
{
  int lx = Link.getX();
  int ly = Link.getY();
  if(lx>=48 && lx<=192)
  {
    if(ly==32)  return up+1;
    if(ly==128) return down+1;
  }
  if(ly>=48 && ly<=112)
  {
    if(lx==32)  return left+1;
    if(lx==208) return right+1;
  }
  return 0;
}

bool tooclose(int x,int y,int d)
{
  return (abs(int(LinkX())-x)<d && abs(int(LinkY())-y)<d);
}

bool isflier(int id)
{
  switch (id&0x0FFF)
  {
    case ePEAHAT:
    case eKEESE1:
    case eKEESE2:
    case eKEESE3:




    case eKEESETRIB:
    case eBAT:
    case ePATRA1:
    case ePATRA2:
    case ePATRAL2:
    case ePATRAL3:
    case ePATRABS:
    case eITEMFAIRY:
      return true;
      break;
  }
  return false;
}

bool isfloater(int id)
{
  switch (id&0x0FFF)
  {
    case eMANHAN:
    case eMANHAN2:
      return true;
      break;
  }
  return isflier(id);
}

/**********************************/
/*******  Enemy Base Class  *******/
/**********************************/

/* ROM data flags

 */

enemy::enemy(fix X,fix Y,int Id,int Clk) : sprite()
{
  x=X; y=Y; id=Id; clk=Clk;
  fading = misc = clk2 = clk3 = stunclk = hclk = sclk = 0;
  grumble = movestatus = foobyte = timer = 0;
  yofs = 54;

  d = guysbuf + (id & 255);
  hp = d->hp;
  cs = d->cset;
  tile = d->tile;
  step = d->step/100.0;
  item_set = d->item_set;
  grumble = d->grumble;
  frate = d->frate;
  if(frate == 0)
    frate = 256;

  superman = (d->flags & (guy_superman | guy_sbombonly)) >> 3;
  if(superman > 2)
    superman = 2;

  leader = itemguy = dying = scored = false;
  canfreeze = count_enemy = mainguy = true;
  dir = rand()&3;
}

enemy::~enemy() {}

// Supplemental animation code that all derived classes should call
// as a return value for animate().
// Handles the death animation and returns true when enemy is finished.
bool enemy::Dead(int index)
{
  if(dying)
  {
    --clk2;
    if(clk2==12 && hp>-1000)                                // not killed by ringleader
      death_sfx();
    if(clk2==0)
    {
      if(index>-1 && (d->flags&guy_neverret))
        never_return(index);
      if(leader)
        kill_em_all();
      leave_item();
      return true;
    }
  }
  return false;
}

// Basic animation code that all derived classes should call.
// The one with an index is the one that is called by
// the guys sprite list; index is the enemy's index in the list.
bool enemy::animate(int index)
{
  // clk is incremented here
  if(++clk >= frate)
    clk=0;
  // hit and death handling
  if(hclk>0)
    --hclk;
  if(stunclk>0)
    --stunclk;

  if(!dying && hp<=0)
  {
    if (itemguy && hasitem==2)
    {
      items.spr(itemindex)->x = x;
      items.spr(itemindex)->y = y - 2;
    }
    dying=true;
    if(fading==fade_flash_die)
      clk2=19+18*4;
    else
    {
      clk2 = BSZ ? 15 : 19;
      if(fading!=fade_blue_poof)
        fading=0;
    }
    if(itemguy)
    {
      hasitem=0;
      item_set=0;
    }
    if(currscr<128 && count_enemy)
      game.guys[(currmap<<7)+currscr]-=1;
  }

  scored=false;

  // returns true when enemy is defeated
  return Dead();
}

// to allow for different sfx on defeating enemy
void enemy::death_sfx()
{
  sfx(WAV_EDEAD,pan(int(x)));
}

void enemy::move(fix dx,fix dy)
{
  if(!watch)
  {
    x+=dx;
    y+=dy;
  }
}

void enemy::move(fix s)
{
  if(!watch)
    sprite::move(s);
}

void enemy::leave_item()

{
  int i=-1;
  int r=rand()%100;

  if (get_bit(quest_rules,qr_ENABLEMAGIC)
    &&(game.maxmagic>0))
  {
    switch(item_set)
    {
      case isDEFAULT:
        if(r<3)       i=iFairyMoving;                       // 3%
        else if(r<8)  i=i5Rupies;                           // 5%
        else if(r<20) i=iHeart;                             // 12%
        else if(r<40) i=iRupy;                              // 20%
        break;                                              // 60%

      case isBOMBS:
        if(r<2)       i=iFairyMoving;                       // 2%
        else if(r<6)  i=can_do_clock()?iClock:-1;           // 4%
        else if(r<16) i=iRupy;                              // 10%
        else if(r<20) i=i5Rupies;                           // 4%
        else if(r<30) i=iHeart;                             // 10%
        else if(r<42) i=iBombs;                             // 12%
        break;                                              // 58%

      case isMAGICBOMBS:
        if(r<2)       i=iFairyMoving;                       // 2%

        else if(r<4)  i=iLMagic;                            // 2%
        else if(r<8)  i=iSMagic;                            // 4%
        else if(r<12) i=can_do_clock()?iClock:-1;           // 4%
        else if(r<22) i=iRupy;                              // 10%
        else if(r<26) i=i5Rupies;                           // 4%
        else if(r<36) i=iHeart;                             // 10%
        else if(r<48) i=iBombs;                             // 12%
        break;                                              // 52%

      case isMONEY:
        if(r<3)       i=iFairyMoving;                       // 3%
        else if(r<8)  i=can_do_clock()?iClock:-1;           // 5%
        else if(r<23) i=i5Rupies;                           // 15%
        else if(r<33) i=iHeart;                             // 10%
        else if(r<55) i=iRupy;                              // 22%
        break;                                              // 45%

      case isMAGICMONEY:
        if(r<3)       i=iFairyMoving;                       // 3%
        else if(r<6)  i=iLMagic;                            // 3%
        else if(r<12) i=iSMagic;                            // 6%
        else if(r<17) i=can_do_clock()?iClock:-1;           // 5%
        else if(r<32) i=i5Rupies;                           // 15%
        else if(r<42) i=iHeart;                             // 10%
        else if(r<64) i=iRupy;                              // 22%
        break;                                              // 36%

      case isLIFE:
        if(r<8)       i=iFairyMoving;                       // 8%
        else if(r<16) i=iRupy;                              // 8%
        else if(r<48) i=iHeart;                             // 32%
        break;                                              // 52%

      case isMAGICLIFE:
        if(r<4)       i=iFairyMoving;                       // 4%
        else if(r<8)  i=iLMagic;                            // 4%
        else if(r<16) i=iSMagic;                            // 8%
        else if(r<32) i=iRupy;                              // 16%
        else if(r<80) i=iHeart;                             // 48%
        break;                                              // 20%

      case isMAGIC:
        if(r<8)       i=iLMagic;                            // 8%
        else if(r<32) i=iSMagic;                            // 16%
        break;                                              // 76%

      case isMAGIC2:
        if(r<25)       i=iLMagic;                           // 25%
        else           i=iSMagic;                           // 75%
        break;                                              // 0%

      case isBOMB100: i=iBombs; break;                      //100%
      case isSBOMB100: i=iSBomb; break;                     //100%
    }
  }
  else
  {
    switch(item_set)
    {
      case isDEFAULT:
        if(r<3)       i=iFairyMoving;                       // 3%

        else if(r<8)  i=i5Rupies;                           // 5%
        else if(r<20) i=iHeart;                             // 12%
        else if(r<40) i=iRupy;                              // 20%
        break;                                              // 60%

      case isMAGICBOMBS:
      case isBOMBS:
        if(r<2)       i=iFairyMoving;                       // 2%
        else if(r<6)  i=can_do_clock()?iClock:-1;           // 4%
        else if(r<16) i=iRupy;                              // 10%
        else if(r<20) i=i5Rupies;                           // 4%
        else if(r<30) i=iHeart;                             // 10%
        else if(r<42) i=iBombs;                             // 12%
        break;                                              // 58%

      case isMAGICMONEY:
      case isMONEY:
        if(r<3)       i=iFairyMoving;                       // 3%
        else if(r<8)  i=can_do_clock()?iClock:-1;           // 5%
        else if(r<23) i=i5Rupies;                           // 15%
        else if(r<33) i=iHeart;                             // 10%
        else if(r<55) i=iRupy;                              // 22%
        break;                                              // 45%

      case isMAGICLIFE:
      case isLIFE:
        if(r<8)       i=iFairyMoving;                       // 8%
        else if(r<16) i=iRupy;                              // 8%
        else if(r<48) i=iHeart;                             // 32%
        break;                                              // 52%

      case isBOMB100: i=iBombs; break;                      //100%
      case isSBOMB100: i=iSBomb; break;                     //100%
    }
  }

  //    if(i!=-1) {
  if (i!=iFairyMoving||!m_walkflag(x,y,0))
  {
    items.add(new item(x,y,i,ipBIGRANGE+ipTIMER,0));
  }
  //    }

  //        items.add(new item(x,y,iClock,ipBIGRANGE+ipTIMER,0));
}

// auomatically kill off enemy (for rooms with ringleaders)
void enemy::kickbucket()
{
  if(!superman)
    hp=-1000;                                               // don't call death_sfx()
}


// take damage or ignore it
int enemy::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying || clk<0 || hclk>0)
    return 0;
  if(superman && !(wpnId==wSBomb && superman==2)            // vulnerable to super bombs
                                                            // fire boomerang, for nailing untouchable enemies
    && !(wpnId==wBrang && current_item(itype_brang,true)==3))
    return 0;

  switch(wpnId)
  {
    case wPhantom:
      return 0;
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    case wGSparkle:
    case wMSparkle:
      return 0;
    case wFSparkle:
      takehit(wSword, DAMAGE_MULTIPLIER>>1, wpnx, wpny, wpnDir);
      break;
    case wBrang:
      if(!(d->flags & guy_bhit))
      {
        stunclk=160;
        if (current_item(itype_brang,true)==3)
        {
          takehit(wSword, 2*DAMAGE_MULTIPLIER, wpnx, wpny, wpnDir);
        }
        break;
      }
    case wHookshot:
      if(!(d->flags & guy_bhit))
      {
        stunclk=160;
        break;
      }
    case wHSHandle:
      if(!(d->flags & guy_bhit))
      {
        stunclk=160;
        break;
      }
    default:
      hp-=power;
      hclk=33;
      if((dir&2)==(wpnDir&2))
        sclk=(wpnDir<<8)+16;
  }

  if(((wpnId==wBrang) || (get_bit(quest_rules,qr_NOFLASHDEATH))) && hp<=0)
  {
    fading=fade_blue_poof;
  }

  sfx(WAV_EHIT,pan(int(x)));
                                                            // arrow keeps going
  return ((wpnId==wArrow)&&(current_item(itype_arrow,true)>2)) ? 0 : 1;
  //    return 1;
}

bool enemy::dont_draw()
{
  if(fading==fade_invisible || (fading==fade_flicker && (clk&1)))
    return true;
  /*
      if((d->flags&guy_invisible) || ((tmpscr->flags3&fINVISROOM))
        && !(game.misc2&iCROSS))
        return true;
  */
  if(d->flags&guy_invisible)
    return true;
  if(d->flags&lens_only && !lensclk)
    return true;
  return false;
}

// base drawing function to be used by all derived classes instead of
// sprite::draw()
void enemy::draw(BITMAP *dest)
{
  if(dont_draw())
    return;

  int cshold=cs;
  if(dying)
  {
    if(clk2>=19)
    {
      if(!(clk2&2))
        sprite::draw(dest);
      return;
    }
    flip = 0;
    tile = wpnsbuf[iwDeath].tile;

    if(BSZ)
      tile += min((15-clk2)/3,4);
    else if(clk2>6 && clk2<=12)
      ++tile;

    /* trying to get more death frames here
       if(wpnsbuf[wid].frames)
       {
         if(++clk2 >= wpnsbuf[wid].speed)
         {
           clk2 = 0;
           if(++aframe >= wpnsbuf[wid].frames)
             aframe = 0;
         }
         tile = wpnsbuf[wid].tile + aframe;
       }
    */

    if(BSZ || fading==fade_blue_poof)
      cs = wpnsbuf[iwDeath].csets&15;
    else
      cs = (((clk2+5)>>1)&3)+6;
  }
  else if(hclk>0 && (hclk<33 || id==eGANON))
    cs=(((hclk-1)>>1)&3)+6;
  if ((tmpscr->flags3&fINVISROOM)&& !(current_item(itype_amulet,true)))
  {
    sprite::drawcloaked(dest);
  }
  else
  {
    sprite::draw(dest);
  }
  cs=cshold;
}

// similar to the overblock function--can do up to a 32x32 sprite
void enemy::drawblock(BITMAP *dest,int mask)
{
  int thold=tile;
  int t1=tile;
  int t2=tile+20;
  int t3=tile+1;
  int t4=tile+21;
  switch(mask)
  {
    case 1:
      enemy::draw(dest);
      break;
    case 3:
      if(flip&2)
        swap(t1,t2);
      tile=t1; enemy::draw(dest);
      tile=t2; yofs+=16; enemy::draw(dest); yofs-=16;
      break;
    case 5:
      t2=tile+1;
      if(flip&1)
        swap(t1,t2);
      tile=t1; enemy::draw(dest);
      tile=t2; xofs+=16; enemy::draw(dest); xofs-=16;
      break;
    case 15:
      if(flip&1)
      {
        swap(t1,t3);
        swap(t2,t4);
      }
      if(flip&2)
      {
        swap(t1,t2);
        swap(t3,t4);
      }
      tile=t1; enemy::draw(dest);
      tile=t2; yofs+=16; enemy::draw(dest); yofs-=16;
      tile=t3; xofs+=16; enemy::draw(dest);
      tile=t4; yofs+=16; enemy::draw(dest); xofs-=16; yofs-=16;
      break;
  }
  tile=thold;
}

void enemy::drawshadow(BITMAP *dest, bool translucent)
{
  if(dont_draw())
  {
    return;
  }

  int cshold=cs;
  if(dying)
  {
    return;
  }
  if (((tmpscr->flags3&fINVISROOM)&& !(current_item(itype_amulet,true)))||
    (darkroom))
  {
    return;
  }
  else
  {
    sprite::drawshadow(dest,translucent);
  }
  cs=cshold;
}

void enemy::masked_draw(BITMAP *dest,int mx,int my,int mw,int mh)
{
  BITMAP *sub=create_sub_bitmap(dest,mx,my,mw,mh);
  if(sub!=NULL)
  {
    xofs-=mx;
    yofs-=my;
    enemy::draw(sub);
    xofs+=mx;
    yofs+=my;
    destroy_bitmap(sub);
  }
  else
    enemy::draw(dest);
}

// override hit detection to check for invicibility, stunned, etc
bool enemy::hit(sprite *s)
{
  return (dying || hclk>0) ? false : sprite::hit(s);
}

bool enemy::hit(int tx,int ty,int txsz,int tysz)
{
  return (dying || hclk>0) ? false : sprite::hit(tx,ty,txsz,tysz);
}

bool enemy::hit(weapon *w)
{
  return (dying || hclk>0) ? false : sprite::hit(w);
}

//                         --==**==--

//   Movement routines that can be used by derived classes as needed

//                         --==**==--

void enemy::fix_coords()
{
  x=(fix)((int(x)&0xF0)+((int(x)&8)?16:0));
  y=(fix)((int(y)&0xF0)+((int(y)&8)?16:0));
}

// returns true if next step is ok, false if there is something there
bool enemy::canmove(int ndir,fix s,int special,int dx1,int dy1,int dx2,int dy2)
{
  bool ok;
  switch(ndir)
  {
    case 8:
    case up:     ok = !m_walkflag(x,y+dy1-s,(special==spw_clipbottomright)?spw_none:special) && !((special==spw_floater)&&(COMBOTYPE(x,y+dy1-s)==cNOFLYZONE)); break;
    case 12:
    case down:   ok = !m_walkflag(x,y+dy2+s,special) && !((special==spw_floater)&&(COMBOTYPE(x,y+dy2+s)==cNOFLYZONE)); break;
    case 14:
    case left:   ok = !m_walkflag(x+dx1-s,y+8,(special==spw_clipbottomright||special==spw_clipright)?spw_none:special) && !((special==spw_floater)&&(COMBOTYPE(x+dx1-s,y+8)==cNOFLYZONE)); break;
    case 10:
    case right:  ok = !m_walkflag(x+dx2+s,y+8,special) && !((special==spw_floater)&&(COMBOTYPE(x+dx2+s,y+8)==cNOFLYZONE)); break;
    case 9:
    case r_up:   ok = !m_walkflag(x,y+dy1-s,special) && !m_walkflag(x+dx2+s,y+8,special)
      && !((special==spw_floater)&&(COMBOTYPE(x,y+dy1-s)==cNOFLYZONE)) && !((special==spw_floater)&&(COMBOTYPE(x+dx2+s,y+8)==cNOFLYZONE)); break;
    case 11:
    case r_down: ok = !m_walkflag(x,y+dy2+s,special) && !m_walkflag(x+dx2+s,y+8,special)
      && !((special==spw_floater)&&(COMBOTYPE(x,y+dy2+s)==cNOFLYZONE)) && !((special==spw_floater)&&(COMBOTYPE(x+dx2+s,y+8)==cNOFLYZONE)); break;
    case 13:
    case l_down: ok = !m_walkflag(x,y+dy2+s,special) && !m_walkflag(x+dx1-s,y+8,special)
      && !((special==spw_floater)&&(COMBOTYPE(x,y+dy2+s)==cNOFLYZONE)) && !((special==spw_floater)&&(COMBOTYPE(x+dx1-s,y+8)==cNOFLYZONE)); break;
    case 15:
    case l_up:   ok = !m_walkflag(x,y+dy1-s,special) && !m_walkflag(x+dx1-s,y+8,special)
      && !((special==spw_floater)&&(COMBOTYPE(x,y+dy1-s)==cNOFLYZONE)) && !((special==spw_floater)&&(COMBOTYPE(x+dx1-s,y+8)==cNOFLYZONE)); break;
    default: db=99; ok=true;
  }
  return ok;
}


bool enemy::canmove(int ndir,fix s,int special)
{
  return canmove(ndir,s,special,0,-8,15,15);
}

bool enemy::canmove(int ndir,int special)
{
  return canmove(ndir,(fix)1,special,0,-8,15,15);
}

bool enemy::canmove(int ndir)
{
  return canmove(ndir,(fix)1,spw_none,0,-8,15,15);
}

// 8-directional
void enemy::newdir_8(int rate,int special,int dx1,int dy1,int dx2,int dy2)
{
  int ndir=0;
  // can move straight, check if it wants to turn
  if(canmove(dir,step,special,dx1,dy1,dx2,dy2))
  {
    int r=rand();
    if(rate>0 && !(r%rate))
    {
      ndir = ((dir+((r&64)?-1:1))&7)+8;
      if(canmove(ndir,step,special,dx1,dy1,dx2,dy2))
        dir=ndir;
      else
      {
        ndir = ((dir+((r&64)?1:-1))&7)+8;
        if(canmove(ndir,step,special,dx1,dy1,dx2,dy2))
          dir=ndir;
      }
      if(dir==ndir)
      {
        x.v&=0xFFFF0000;
        y.v&=0xFFFF0000;
      }
    }
    return;
  }
  // can't move straight, must turn
  int i=0;
  for( ; i<32; i++)
  {
    ndir=(rand()&7)+8;
    if(canmove(ndir,step,special,dx1,dy1,dx2,dy2))
      break;
  }
  if(i==32)
  {
    for(ndir=8; ndir<16; ndir++)
    {
      if(canmove(i,step,special,dx1,dy1,dx2,dy2))
        goto ok;
    }
    ndir = -1;
  }

  ok:
  dir=ndir;
  x.v&=0xFFFF0000;
  y.v&=0xFFFF0000;
}

void enemy::newdir_8(int rate,int special)
{
  newdir_8(rate,special,0,-8,15,15);
}

// makes the enemy slide backwards when hit
// sclk: first byte is clk, second byte is dir
bool enemy::slide()
{
  if(sclk==0 || hp<=0)
    return false;
  if((sclk&255)==16 && !canmove(sclk>>8,(fix)12,0))
  {
    sclk=0;
    return false;
  }
  --sclk;
  switch(sclk>>8)
  {
    case up:    y-=4; break;
    case down:  y+=4; break;
    case left:  x-=4; break;
    case right: x+=4; break;
  }
  if(!canmove(sclk>>8,(fix)0,0))
  {
    switch(sclk>>8)
    {
      case up:
      case down:
        if( (int(y)&15) > 7 )
          y=(int(y)&0xF0)+16;
        else
          y=(int(y)&0xF0);
        break;
      case left:
      case right:
        if( (int(x)&15) > 7 )
          x=(int(x)&0xF0)+16;
        else
          x=(int(x)&0xF0);
        break;
    }
    sclk=0;
    clk3=0;
  }
  if((sclk&255)==0)
    sclk=0;
  return true;
}


bool enemy::fslide()
{
  if(sclk==0 || hp<=0)
    return false;
  if((sclk&255)==16 && !canmove(sclk>>8,(fix)12,spw_floater))
  {
    sclk=0;
    return false;
  }
  --sclk;
  switch(sclk>>8)
  {
    case up:    y-=4; break;
    case down:  y+=4; break;
    case left:  x-=4; break;
    case right: x+=4; break;
  }
  if(!canmove(sclk>>8,(fix)0,spw_floater))
  {
    switch(sclk>>8)
    {
      case up:
      case down:
        if( (int(y)&15) > 7 )
          y=(int(y)&0xF0)+16;
        else
          y=(int(y)&0xF0);
        break;
      case left:
      case right:
        if( (int(x)&15) > 7 )
          x=(int(x)&0xF0)+16;
        else
          x=(int(x)&0xF0);
        break;
    }
    sclk=0;
    clk3=0;
  }
  if((sclk&255)==0)
    sclk=0;
  return true;
}

// changes enemy's direction, checking restrictions
// rate:   0 = no random changes, 16 = always random change
// homing: 0 = none, 256 = always
// grumble 0 = none, 4 = strongest appetite
void enemy::newdir(int rate,int homing,int special)
{
  int ndir;
  if(grumble && (rand()&3)<grumble)
    //  for super bait
    //    if(1)
  {
    int w = Lwpns.idFirst(wBait);
    if(w>=0)
    {
      int bx = Lwpns.spr(w)->x;
      int by = Lwpns.spr(w)->y;
      if(abs(int(y)-by)>14)
      {
        ndir = (by<y) ? up : down;
        if(canmove(ndir,special))
        {
          dir=ndir;
          return;
        }
      }
      ndir = (bx<x) ? left : right;
      if(canmove(ndir,special))
      {
        dir=ndir;
        return;
      }
    }
  }
  if((rand()&255)<homing)
  {
    ndir = lined_up(8);
    if(ndir>=0 && canmove(ndir,special))
    {
      dir=ndir;
      return;
    }
  }

  int i=0;
  for( ; i<32; i++)
  {
    int r=rand();
    if((r&15)<rate)
      ndir=(r>>4)&3;
    else
      ndir=dir;
    if(canmove(ndir,special))
      break;
  }
  if(i==32)
  {
    for(ndir=0; ndir<4; ndir++)
    {
      if(canmove(ndir,special))
        goto ok;
    }
    ndir = -1;
  }

  ok:
  dir = ndir;
}

void enemy::newdir()
{
  newdir(4,0,spw_none);
}

fix enemy::distance_left()
{
  int a=x.v>>16;
  int b=y.v>>16;

  switch(dir)
  {
    case up:    return (fix)(b&0xF);
    case down:  return (fix)(16-(b&0xF));
    case left:  return (fix)(a&0xF);
    case right: return (fix)(16-(a&0xF));
  }
  return (fix)0;
}

// keeps walking around
void enemy::constant_walk(int rate,int homing,int special)
{
  if(slide())
    return;
  if(clk<0 || dying || stunclk || watch)
    return;
  if(clk3<=0)
  {
    fix_coords();
    newdir(rate,homing,special);
    clk3=int(16.0/step);
  }
  else if(scored)
  {
    dir^=1;
    clk3=int(16.0/step)-clk3;
  }
  --clk3;
  move(step);
}

void enemy::constant_walk()
{
  constant_walk(4,0,spw_none);
}

int enemy::pos(int x,int y)
{
  return (y<<8)+x;
}

// for variable step rates
void enemy::variable_walk(int rate,int homing,int special)
{
  if(slide())
    return;
  if(clk<0 || dying || stunclk || watch)
    return;
  if((int(x)&15)==0 && (int(y)&15)==0 && clk3!=pos(x,y))
  {
    fix_coords();
    newdir(rate,homing,special);
    clk3=pos(x,y);
  }
  move(step);
}

// pauses for a while after it makes a complete move (to a new square)
void enemy::halting_walk(int rate,int homing,int special,int hrate, int haltcnt)
{
  if(sclk && clk2)
    clk3=0;
  if(slide() || clk<0 || dying || stunclk || watch)
    return;
  if(clk2>0)
  {
    --clk2;
    return;
  }
  if(clk3<=0)
  {
    fix_coords();
    newdir(rate,homing,special);
    clk3=int(16.0/step);
    if(clk2<0)
      clk2=0;
    else if((rand()&15)<hrate)
    {
      clk2=haltcnt;
      return;
    }
  }
  else if(scored)
  {
    dir^=1;

    clk3=int(16.0/step)-clk3;
  }
  --clk3;
  move(step);
}

// 8-directional movement, aligns to 8 pixels
void enemy::constant_walk_8(int rate,int special)
{
  if(clk<0 || dying || stunclk || watch)
    return;
  if(clk3<=0)
  {
    newdir_8(rate,special);
    clk3=int(8.0/step);
  }
  --clk3;
  move(step);
}

// 8-directional movement, no alignment
void enemy::variable_walk_8(int rate,int newclk,int special)
{
  if(clk<0 || dying || stunclk || watch)
    return;
  if(!canmove(dir,step,special))
    clk3=0;
  if(clk3<=0)
  {
    newdir_8(rate,special);
    clk3=newclk;
  }
  --clk3;
  move(step);
}

// same as above but with variable enemy size
void enemy::variable_walk_8(int rate,int newclk,int special,int dx1,int dy1,int dx2,int dy2)
{
  if(clk<0 || dying || stunclk || watch)
    return;
  if(!canmove(dir,step,special,dx1,dy1,dx2,dy2))
    clk3=0;
  if(clk3<=0)
  {
    newdir_8(rate,special,dx1,dy1,dx2,dy2);
    clk3=newclk;
  }
  --clk3;
  move(step);
}

// the variable speed floater movement
// ms is max speed
// ss is step speed
// s is step count
// p is pause count
// g is graduality :)

void enemy::floater_walk(int rate,int newclk,fix ms,fix ss,int s,int p, int g)
{
  ++clk2;
  switch(movestatus)
  {
    case 0:                                                 // paused
      if(clk2>=p)
      {
        movestatus=1;
        clk2=0;
      }
      break;

    case 1:                                                 // speeding up
      if(clk2<g*s)
      {
        if(!((clk2-1)%g))
          step+=ss;
      }
      else
      {
        movestatus=2;
        clk2=0;
      }
      break;

    case 2:                                                 // normal
      step=ms;
      if(clk2>48 && !(rand()%768))
      {
        step=ss*s;
        movestatus=3;
        clk2=0;
      }
      break;

    case 3:                                                 // slowing down
      if(clk2<=g*s)
      {
        if(!(clk2%g))
          step-=ss;
      }
      else
      {
        movestatus=0;
        clk2=0;
      }
      break;
  }
  variable_walk_8(movestatus==2?rate:0,newclk,spw_floater);
}

void enemy::floater_walk(int rate,int newclk,fix s)
{
  floater_walk(rate,newclk,s,(fix)0.125,3,80,32);
}

// Checks if enemy is lined up with Link. If so, returns direction Link is
// at as compared to enemy. Returns -1 if not lined up. Range is inclusive.
int enemy::lined_up(int range)
{
  int lx = Link.getX();
  int ly = Link.getY();
  if(abs(lx-int(x))<=range)
  {
    if(ly<y)
      return up;
    return down;
  }
  if(abs(ly-int(y))<=range)
  {
    if(lx<x)
      return left;
    return right;
  }
  return -1;
}

// returns true if Link is within 'range' pixels of the enemy
bool enemy::LinkInRange(int range)
{
  int lx = Link.getX();
  int ly = Link.getY();
  return abs(lx-int(x))<=range && abs(ly-int(y))<=range;
}

// place the enemy in line with Link (red wizzrobes)
void enemy::place_on_axis(bool floater)
{
  int lx=min(max(int(Link.getX())&0xF0,32),208);
  int ly=min(max(int(Link.getY())&0xF0,32),128);
  int pos=rand()%23;
  int tried=0;
  bool last_resort,placed=false;

  do
  {
    if(pos<14)
      { x=(pos<<4)+16; y=ly; }
      else
        { x=lx; y=((pos-14)<<4)+16; }

        if(x<32 || y<32 || x>=224 || y>=144)
          last_resort=false;
    else
      last_resort=true;

    if(abs(lx-int(x))>16 || abs(ly-int(y))>16)
    {
      if(!m_walkflag(x,y,1))

        placed=true;
      else if(floater && last_resort && !iswater(MAPDATA(x,y)))
        placed=true;
    }

    if(!placed && tried>=22 && last_resort)
      placed=true;
    ++tried;
    pos=(pos+3)%23;
  } while(!placed);

  if(y==ly)
    dir=(x<lx)?right:left;
  else
    dir=(y<ly)?down:up;
  clk2=tried;
}

void enemy::update_enemy_frame()
{
  bool f2 = (clk >= (frate>>1));                            // second frame of 2 frame animations

  tile = d->tile;

  switch(d->anim)
  {
    case aFLIP:   flip  = f2 ? 1 : 0; break;
    case a2FRM:   tile += f2 ? 0 : 1; break;

    case aOCTO:
      switch(dir)
      {
        case up:    flip = 2; tile += f2 ? 1 : 0; break;
        case down:  flip = 0; tile += f2 ? 1 : 0; break;
        case left:  flip = 0; tile += f2 ? 3 : 2; break;
        case right: flip = 1; tile += f2 ? 3 : 2; break;
      } break;

    case aWALK:
      switch(dir)
      {
        case up:    tile+=3; flip = f2 ? 1:0; break;
        case down:  tile+=2; flip = f2 ? 1:0; break;
        case left:  flip=1; tile += f2 ? 1:0; break;
        case right: flip=0; tile += f2 ? 1:0; break;
      } break;

    case aTEK:
      if(misc==0)
        tile += f2 ? 1 : 0;
      else if(misc!=1)
        ++tile;
      break;

    case aARMOS:
      if(misc)
      {
        tile += f2 ? 1 : 0;
        if(dir==up)
          tile += 2;
      } break;

    case aGHINI:
      switch(dir)
      {
        case 8:
        case 9:
        case up: ++tile; flip=0; break;
        case 15: ++tile; flip=1; break;
        case 10:
        case 11:
        case right: flip=1; break;
        default:
          flip=0; break;
      } break;

  }                                                         // switch(d->anim)

  // flashing
  if(d->cset > 14)
    cs = (frame&3) + 6;
}

/********************************/
/*********  Guy Class  **********/
/********************************/

// good guys, fires, fairy, and other non-enemies
// based on enemy class b/c guys in dungeons act sort of like enemies
// also easier to manage all the guys this way
guy::guy(fix X,fix Y,int Id,int Clk,bool mg) : enemy(X,Y,Id,Clk)
{
  mainguy=mg;
  canfreeze=false;
  dir=down;
  yofs=56;
  hxofs=2;
  hxsz=12;
  hysz=17;
  if(!superman && !isdungeon())
    superman = 1;
}

bool guy::animate(int index)
{
  if(mainguy && clk==0 && misc==0)
  {
    setupscreen();
    misc = 1;
  }
  if(mainguy && fadeclk==0)
    return true;
  hp=256;                                                   // good guys never die...
  if(hclk && !clk2)
  {                                                         // but if they get hit...
    ++clk2;                                                 // only do this once
    if (!get_bit(quest_rules,qr_NOGUYFIRES))
    {
      addenemy(BSZ?64:72,68,eFBALL,0);
      addenemy(BSZ?176:168,68,eFBALL,0);
    }
  }
  return enemy::animate(index);
}

void guy::draw(BITMAP *dest)
{
  update_enemy_frame();
  if(!mainguy || fadeclk<0 || fadeclk&1)
    enemy::draw(dest);
}

/*******************************/
/*********   Enemies   *********/
/*******************************/

eFire::eFire(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+1580;
    frate=16;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

void eFire::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
  if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
  {
    dir=l_down;
  }
  else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
  {
    dir=down;

  }
  else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
  {
    dir=r_down;
  }
  else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
  {
    dir=right;
  }
  else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
  {
    dir=r_up;
  }
  else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
  {
    dir=up;
  }
  else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
  {
    dir=l_up;
  }
  else
  {
    dir=left;
  }

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
      case l_up:
        flip=0; tile+=20;
        break;
      case r_up:
        flip=0; tile+=24;
        break;
      case l_down:
        flip=0; tile+=28;
        break;
      case r_down:
        flip=0; tile+=32;
        break;
    }

    tile+=f2;
  }
  else
  {
    flip  = f2 ? 1 : 0;
  }
  enemy::draw(dest);
}

void removearmos(int ax,int ay)
{
  ax&=0xF0;
  ay&=0xF0;
  int cd = (ax>>4)+ay;
  int f = MAPFLAG(ax,ay);

  if (combobuf[tmpscr->data[cd]].type!=cARMOS)
  {
    return;
  }
  tmpscr->data[cd] = tmpscr->undercombo;
  tmpscr->cset[cd] = tmpscr->undercset;
  tmpscr->sflag[cd] = 0;

  switch(f)
  {
    case mfARMOS_SECRET:
      tmpscr->data[cd] = tmpscr->secretcombo[sSTAIRS];
      tmpscr->cset[cd] = tmpscr->secretcset[sSTAIRS];
      tmpscr->sflag[cd]=tmpscr->secretflag[sSTAIRS];
      if (!nosecretsounds)
      {
        sfx(WAV_SECRET);
      }
      break;

    case mfARMOS_ITEM:
      if(!getmapflag())
      {
        additem(ax,ay,tmpscr->catchall, (ipONETIME + ipBIGRANGE)
          | ((tmpscr->flags3&fHOLDITEM) ? ipHOLDUP : 0)
          );
        if (!nosecretsounds)
        {
          sfx(WAV_SECRET);
        }
      }
      break;
  }
  putcombo(scrollbuf,ax,ay,tmpscr->data[cd],tmpscr->cset[cd]);
}

eArmos::eArmos(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,0)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+680;
  }
  else
  {
    dummy_int[0]=tile;
  }
  //    tile = d->tile + (rand()&1); // wth?
  fromstatue=true;
  superman = 1;
  fading=fade_flicker;
  step=(rand()&1)?1.5:.5;
  count_enemy=false;
  if(!canmove(down,(fix)8,spw_none))
    dir=-1;
  else
  {
    dir=down;
    clk3=int(13.0/step);
  }
}

bool eArmos::animate(int index)
{
  if(dying)
    return Dead();
  if(misc)
    constant_walk(d->rate, d->homing, 0);
  else if(++clk2 > 60)
  {
    misc=1;
    superman=0;
    fading=0;
    removearmos(x,y);
    clk=-1;
    clk2=0;
    if(dir==-1)
    {
      dir=0;
      y.v&=0xF00000;
    }
  }

  return enemy::animate(index);
}

void eArmos::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    if (step>1)
    {
      tile+=20;
    }
    if(misc)
    {
      tile+=f2;
    }
  }
  else
  {
    if(dir==up)
      tile += 2;
    if(misc)
    {
      tile += f2 ? 1 : 0;
    }
  }

  enemy::draw(dest);
}

eGhini::eGhini(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if (id==eGHINI2&&get_bit(quest_rules,qr_PHANTOMGHINI2))
  {
    drawstyle=1;
  }
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+620;
  }
  else
  {
    dummy_int[0]=tile;
  }
  if(d->misc1)
  {
    //fixing      step=0;
    fading=fade_flicker;
    count_enemy=false;
    dir=12;
    movestatus=1;
    clk=0;
  }
  frate=256;                                                // don't use frate, allow clk to go up past 160
  foobyte=0;
  c=0;
}

bool eGhini::animate(int index)
{
  if(dying)
    return Dead();
  if(d->misc1==0 && clk>=0)
    constant_walk(d->rate,d->homing,0);
  if(d->misc1)
  {
    if(misc)
    {
      if(clk>160)
        misc=2;
      floater_walk((misc==1)?0:4,12,(fix)0.625,(fix)0.0625,10,120,10);
    }
    else if(clk>=60)
    {
      misc=1;
      clk3=32;
      fading=0;
      guygrid[(int(y)&0xF0)+(int(x)>>4)]=0;
    }
  }

  int nx = real_x(x);
  int ny = real_y(y);
  if(ox!=nx || oy!=ny)
    ++c;
  if (c==4)
  {
    c=0;
  }
  dummy_int[1] = c;                                         // fix for BSZ style...
  ox = nx;
  oy = ny;

  return enemy::animate(index);
}

void eGhini::draw(BITMAP *dest)
{
  if ((id==eGHINI2)&&(get_bit(quest_rules,qr_GHINI2BLINK))&&(clk&1))
  {
    return;
  }
  tile=dummy_int[0];
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    if (id==eGHINI2)
    {
      tile+=20;
      switch (dir-8)                                        //directions get screwed up after 8.  *shrug*
      {

        case up:                                            //u
          flip=0;
          break;
        case l_up:                                          //d
          flip=0; tile+=4;
          break;
        case l_down:                                        //l
          flip=0; tile+=8;
          break;
        case left:                                          //r
          flip=0; tile+=12;
          break;
        case r_down:                                        //ul
          flip=0; tile+=20;
          break;
        case down:                                          //ur
          flip=0; tile+=24;
          break;
        case r_up:                                          //dl
          flip=0; tile+=28;
          break;
        case right:                                         //dr
          flip=0; tile+=32;
          break;
      }
    }
    else
    {
      switch (dir)
      {
        case up:
          flip=0;
          break;
        case down:
          flip=0; tile+=4;
          break;
        case left:
          flip=0; tile+=8;
          break;
        case right:
          flip=0; tile+=12;
          break;
      }
    }
    tile+=dummy_int[1];
  }
  else
  {
    //      tile+=dummy_int[1];
    switch(dir)
    {
      case 8:
      case 9:
      case up: ++tile; flip=0; break;
      case 15: ++tile; flip=1; break;
      case 10:
      case 11:
      case right: flip=1; break;
      default:
        flip=0; break;
    }
  }

  enemy::draw(dest);
}

void eGhini::kickbucket()
{
  hp=-1000;                                                 // don't call death_sfx()
}

eTektite::eTektite(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  dir=down;
  misc=1;
  clk=-15;
  if(!BSZ)
    clk*=rand()%3+1;

  // avoid divide by 0 errors
  if(d->misc1 == 0)
    d->misc1 = 24;
  if(d->misc2 == 0)
    d->misc2 = 3;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+760;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eTektite::animate(int index)
{
  if(dying)
    return Dead();
  if(clk>=0 && !stunclk && (!watch || misc==0))
    switch(misc)
    {
      case 0:                                               // normal
        if(!(rand()%d->misc1))
        {
          misc=1;
          clk2=32;
        }
        break;

      case 1:                                               // waiting to pounce
      if(--clk2<=0)
      {
        int r=rand();
        misc=2;
        step=-2.5;                                          // initial speed
        clk3=(r&1)+2;                                       // left or right
        clk2start=clk2=(r&31)+10;                           // flight time
        if(y<32)  clk2+=2;                                  // make them come down from top of screen
        if(y>112) clk2-=2;                                  // make them go back up
        cstart=c = 9-((r&31)>>3);                           // time before gravity kicks in
    }
    break;

    case 2:                                                 // in flight
      move(step);
      if (step>0)                                           //going down
      {
        if (COMBOTYPE(x+8,y+16)==cNOJUMPZONE)
        {
          step=0-step;
        }
      }
      else if (step<0)
      {
        if (COMBOTYPE(x+8,y)==cNOJUMPZONE)
        {
          step=0-step;
        }
      }

      if (clk3==left)
      {
        if (COMBOTYPE(x,y+8)==cNOJUMPZONE)
        {
          clk3^=1;
        }
      }
      else
      {
        if (COMBOTYPE(x+16,y+8)==cNOJUMPZONE)
        {
          clk3^=1;
        }
      }
      --c;
      if(c<0 && step<2.5)
        step+=.5;
      if(x<=16)  clk3=right;
      if(x>=224) clk3=left;
      x += (clk3==left) ? -1 : 1;
      if((--clk2<=0 && y>=16) || y>=144)
      {
        if(rand()%d->misc2)                                 //land and wait
        {
          clk=misc=0;
        }                                                   //land and jump again
        else
        {
          misc=1;
          clk2=0;
        }
      }
      break;

  }                                                         // switch

  if(stunclk && (clk&31)==1)
    clk=0;
  return enemy::animate(index);
}

void eTektite::drawshadow(BITMAP *dest,bool translucent)
{
  int tempy=yofs;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  flip = 0;
  shadowtile = wpnsbuf[iwShadow].tile;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    if(misc==0)
    {
      shadowtile+=f2;
    }
    else if(misc!=1)
      shadowtile+=2;
  }
  else
  {
    if(misc==0)
    {
      shadowtile += f2 ? 1 : 0;
    }
    else if(misc!=1)
    {
      ++shadowtile;
    }
  }
  yofs+=8;
  if (misc==2)
  {
    yofs+=max(0,min(clk2start-clk2,clk2));
  }
  enemy::drawshadow(dest,translucent);
  yofs=tempy;
}

void eTektite::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    if (step<0)                                             //up
    {
      switch (clk3)
      {
        case left:
          flip=0; tile+=20;
          break;
        case right:
          flip=0; tile+=24;
          break;
      }
    }
    else if (step==0)
    {
      switch (clk3)
      {
        case left:
          flip=0; tile+=8;
          break;
        case right:
          flip=0; tile+=12;
          break;
      }
    }                                                       //down
    else
    {
      switch (clk3)
      {
        case left:
          flip=0; tile+=28;
          break;
        case right:
          flip=0; tile+=32;
          break;
      }
    }
    if (id==eBTEK)
    {
      tile+=40;

    }

    if(misc==0)
    {
      tile+=f2;
    }
    else if(misc!=1)
      tile+=2;
  }
  else
  {
    if(misc==0)
    {
      tile += f2 ? 1 : 0;
    }
    else if(misc!=1)
    {
      ++tile;
    }
  }
  enemy::draw(dest);
}

eItemFairy::eItemFairy(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  hp=guysbuf[eITEMFAIRY].hp;
  step=(fix)(guysbuf[eITEMFAIRY].step)/100;
  frate=guysbuf[eITEMFAIRY].frate;
  superman=1;
  dir=8;
  hxofs=1000;
  mainguy=false;
  count_enemy=false;
}

bool eItemFairy::animate(int index)
{
  if(dying)
    return Dead();
  if(clk>32)
    misc=1;
  bool w=watch;
  watch=false;
  variable_walk_8(misc?3:0,8,spw_floater);
  watch=w;
  return enemy::animate(index);
}

void eItemFairy::draw(BITMAP *dest)
{
}

ePeahat::ePeahat(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  floater_walk(misc?4:0,      8, (fix)0.625, (fix)0.0625, 10,  80, 16);
  dir=8;
  movestatus=1;
  moving_superman = (d->flags&guy_sbombonly) ? 2 : 1;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+720;
  }
  else
  {
    dummy_int[0]=tile;
  }
  c=0; dummy_int[1]=0; foobyte=0;
}

bool ePeahat::animate(int index)
{
  if(dying)
    return Dead();
  if(stunclk==0 && clk>96)
    misc=1;
  floater_walk(misc?4:0,      8, (fix)0.625, (fix)0.0625, 10,  80, 16);
  superman = (movestatus) ? moving_superman : 0;
  stunclk=0;

  int nx = real_x(x);
  int ny = real_y(y);
  if(ox!=nx || oy!=ny)
    ++c;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    if (c==4)
    {
      c=0;
    }
    dummy_int[1] = c;                                       // fix for BSZ style...
    foobyte=0;
  }
  else
  {
    if(c>=2)
    {
      foobyte ^= 1;                                         // fix for BSZ style...
      c=0;
    }
  }
  ox = nx;
  oy = ny;

  return enemy::animate(index);
}

void ePeahat::drawshadow(BITMAP *dest, bool translucent)
{
  int tempy=yofs;
  flip = 0;
  shadowtile = wpnsbuf[iwShadow].tile+foobyte+dummy_int[1];
  yofs+=8;
  yofs+=int(step/(fix)0.0625);
  enemy::drawshadow(dest,translucent);
  yofs=tempy;
}

void ePeahat::draw(BITMAP *dest)
{
  tile=dummy_int[0];

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir-8)                                          //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
        flip=0;
        break;
      case l_up:                                            //d
        flip=0; tile+=4;
        break;
      case l_down:                                          //l
        flip=0; tile+=8;
        break;
      case left:                                            //r
        flip=0; tile+=12;
        break;
      case r_down:                                          //ul
        flip=0; tile+=20;
        break;
      case down:                                            //ur
        flip=0; tile+=24;
        break;
      case r_up:                                            //dl
        flip=0; tile+=28;
        break;
      case right:                                           //dr
        flip=0; tile+=32;
        break;
    }
  }

  tile+=foobyte;
  tile+=dummy_int[1];

  enemy::draw(dest);

}

int ePeahat::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying || clk<0 || hclk>0)
    return 0;
  if(superman && !(wpnId==wSBomb && superman==2)            // vulnerable to super bombs
                                                            // fire boomerang, for nailing peahats
    && !(wpnId==wBrang && current_item(itype_brang,true)==3))
    return 0;


  switch(wpnId)
  {
    case wPhantom:
      return 0;
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wWind:
      return 0;
    case wBrang:
      if(!(d->flags & guy_bhit))
      {
        if (current_item(itype_brang,true)==3)
        {
          stunclk=160;
          clk2=0;
          movestatus=0;
          misc=0;
          clk=0;
          step=0;
          //            floater_walk(misc?4:0,8,0.625,0.0625,10,240,16);

        }
        break;
      }
    default:
      hp-=power;
      hclk=33;
      if((dir&2)==(wpnDir&2))
        sclk=(wpnDir<<8)+16;
  }
  //    if(wpnId==wBrang && hp<=0)
  if(((wpnId==wBrang) || (get_bit(quest_rules,qr_NOFLASHDEATH))) && hp<=0)
  {
    fading=fade_blue_poof;
  }
  sfx(WAV_EHIT,pan(int(x)));
  return 1;
}

eLeever::eLeever(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if(d->misc1==0) { misc=-1; clk-=16; }
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+1460;
    frate=16;
  }
  else
  {
    dummy_int[0]=tile;
  }
  temprule=(get_bit(quest_rules,qr_NEWENEMYTILES));
}

bool eLeever::animate(int index)
{
  if(dying)
    return Dead();
  if(clk>=0 && !slide())
    switch(d->misc1)
    {
      case 0:
        switch(misc)
        {
          case -1:
          {
            int active=0;
            for(int i=0; i<guys.Count(); i++)
              if(guys.spr(i)->id==id && ((enemy*)guys.spr(i))->misc>=0)
                ++active;
          if(active<2)
            misc=0;
        } break;

      case 0:
      {
        int s=0;
        for(int i=0; i<guys.Count(); i++)
          if(guys.spr(i)->id==id && ((enemy*)guys.spr(i))->misc==1)
            ++s;
        if(s>0)
          break;
        int d=rand()&1;
        if(LinkDir()>=left)
          d+=2;
        if(canplace(d) || canplace(d^1))
        {
          misc=1;
          clk2=0;
          clk=0;
        }
      } break;

      case 1: if(++clk2>16) misc=2; break;
      case 2: if(++clk2>24) misc=3; break;
      case 3: if(stunclk) break; if(scored) dir^=1; if(!canmove(dir)) misc=4; else move((fix)0.5); break;

      case 4: if(--clk2==16) { misc=5; clk=8; }
      break;
      case 5: if(--clk2==0)  misc=0; break;
    }                                                       // switch(misc)
    break;

    default:
      step=0.167;
      ++clk2;
      if(clk2<32)    misc=1;
      else if(clk2<48)    misc=2;
      else if(clk2<300) { misc=3; step = d->step/100.0; }
      else if(clk2<316)   misc=2;
      else if(clk2<412)   misc=1;
      else if(clk2<540) { misc=0; step=0; }
      else clk2=0;
      if(clk2==48) clk=0;
      variable_walk(d->rate, d->homing, 0);
  }                                                         // switch(d->misc1)

  hxofs=(misc>=2)?0:1000;
  return enemy::animate(index);
}

bool eLeever::canplace(int d)
{
  int nx=LinkX();
  int ny=LinkY();

  if(d<left) ny&=0xF0;
  else       nx&=0xF0;

  switch(d)
  {
    case up:    ny-=32; break;
    case down:  ny+=32; if(ny-LinkY()<32) ny+=16; break;
    case left:  nx-=32; break;
    case right: nx+=32; if(nx-LinkX()<32) nx+=16; break;
  }
  if(m_walkflag(nx,ny,spw_halfstep ))                       /*none*/
    return false;
  x=nx;
  y=ny;
  dir=d^1;
  return true;
}

void eLeever::draw(BITMAP *dest)
{
  cs = d->cset;
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  bool f4 = ((clk/5)&1);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    if (id==eBLEV)
    {
      tile+=60;
    }
    switch(misc)
    {
      case -1:
      case 0: return;
      case 1:
      case 5: cs = d->misc2; break;
      case 2:
      case 4: tile += 20; break;
      case 3: tile += 40; break;
    }
    tile+=f2;
  }
  else
  {
    switch(misc)
    {
      case -1:
      case 0: return;
      case 1:
      case 5: tile += (f2) ? 1 : 0; cs = d->misc2; break;
      case 2:
      case 4: tile += 2; break;
      case 3: tile += (f4) ? 4 : 3; break;
    }
  }

  enemy::draw(dest);
}

eGel::eGel(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+200;
    frate=frate*2;
  }
  else
  {
    dummy_int[0]=tile;
  }
  clk2=-1;
  if(id>=0x1000)
    count_enemy=false;
  hxsz=hysz=14;
}

bool eGel::animate(int index)
{
  if(dying)
    return Dead();
  if(clk>=0)
    switch(id>>12)
    {
      case 0: halting_walk(d->rate, d->homing, spw_none, d->hrate, ((rand()&7)<<3)+2); break;
      case 1:
        if(misc==1)
        {
          dir=up;
          step=8;
        }
        if(misc<=2)
        {
        move(step);
        if(!canmove(dir,(fix)0,0))
          dir=down;
    }
    if(misc==3)
    {
      if(canmove(right,(fix)16,0))
        x+=16;
    }
    ++misc;
    break;
    case 2:
      if(misc==1)
      {
        dir=down;
        step=8;
      }
      if(misc<=2)
        move(step);
      if(misc==3)
      {
        if(canmove(left,(fix)16,0))
          x-=16;
      }
      ++misc;
      break;
  }
  if(misc>=4)
  {
    id&=0xFFF;
    step = d->step/100.0;
    if(x<32) x=32;
    if(x>208) x=208;
    if(y<32) y=32;
    if(y>128) y=128;
  }
  return enemy::animate(index);
}

void eGel::draw(BITMAP *dest)
{
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  tile = dummy_int[0];

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    tile+=f2;
  }
  else
  {
    tile += f2 ? 1 : 0;
  }
  //    update_enemy_frame();
  enemy::draw(dest);
}

eZol::eZol(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  clk2=-1;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+220;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eZol::animate(int index)
{
  if(dying)
    return Dead();
  if(hp>0 && hp<guysbuf[eGEL+1].hp && !slide())
  {
    guys.add(new eGel(x,y,eGEL+0x1000,-21));
    guys.add(new eGel(x,y,eGEL+0x2000,-22));
    return true;
  }
  else
    halting_walk(d->rate, d->homing, spw_none, d->hrate, (rand()&7)<<4);
  return enemy::animate(index);
}

void eZol::draw(BITMAP *dest)
{
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  tile = dummy_int[0];

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    tile+=f2;
  }
  else
  {
    tile += f2 ? 1 : 0;
  }
  //    update_enemy_frame();
  enemy::draw(dest);
}

int eZol::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  int ret = enemy::takehit(wpnId,power,wpnx,wpny,wpnDir);
  if(sclk)
    sclk+=128;
  return ret;
}

eGelTrib::eGelTrib(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  clk4=0;
  clk2=-1;
  if(id>=0x1000)
    count_enemy=false;
  hxsz=hysz=14;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+240;
    frate=frate*2;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eGelTrib::animate(int index)
{
  ++clk4;
  if(dying)
    return Dead();
  if(clk>=0)
    switch(id>>12)
    {
      case 0: halting_walk(d->rate, d->homing, spw_none, d->hrate, ((rand()&7)<<3)+2); break;
      case 1:
        if(misc==1)
        {
          dir=up;
          step=8;
        }
        if(misc<=2)
        {
        move(step);
        if(!canmove(dir,(fix)0,0))
          dir=down;
    }
    if(misc==3)
    {
      if(canmove(right,(fix)16,0))
        x+=16;
    }
    ++misc;
    break;
    case 2:
      if(misc==1)
      {
        dir=down;
        step=8;
      }
      if(misc<=2)
        move(step);
      if(misc==3)
      {
        if(canmove(left,(fix)16,0))
          x-=16;
      }
      ++misc;
      break;
  }
  if(misc>=4)
  {
    id&=0xFFF;
    step = d->step/100.0;
    if(x<32) x=32;
    if(x>208) x=208;
    if(y<32) y=32;
    if(y>128) y=128;
  }
  if (clk4==256)
  {
    int kids = guys.Count();
    if(get_bit(quest_rules, qr_OLDTRIBBLES))
    {
      guys.add(new eZol((fix)x,(fix)y,eZOL,-21));
    }
    else
    {
      guys.add(new eZolTrib((fix)x,(fix)y,eZOLTRIB,-21));
    }
//    ((enemy*)guys.spr(kids))->count_enemy = false;
    ((enemy*)guys.spr(kids))->count_enemy = count_enemy;
    return true;
  }
  return enemy::animate(index);
}

void eGelTrib::draw(BITMAP *dest)
{
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  tile = dummy_int[0];

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;

        break;
    }
    tile+=f2;
  }
  else
  {
    tile += f2 ? 1 : 0;
  }
  //    update_enemy_frame();
  enemy::draw(dest);
}

eZolTrib::eZolTrib(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  clk2=-1;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+260;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eZolTrib::animate(int index)
{
  if(dying)
    return Dead();
  if(hp>0 && hp<guysbuf[eGELTRIB+1].hp && !slide())
  {
    guys.add(new eGelTrib(x,y,eGELTRIB+0x1000,-21));
    guys.add(new eGelTrib(x,y,eGELTRIB+0x2000,-22));
    return true;
  }
  else
    halting_walk(d->rate, d->homing, spw_none, d->hrate, (rand()&7)<<4);
  return enemy::animate(index);
}

void eZolTrib::draw(BITMAP *dest)
{
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  tile = dummy_int[0];

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;

      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    tile+=f2;
  }
  else
  {
    tile += f2 ? 1 : 0;
  }
  //    update_enemy_frame();
  enemy::draw(dest);
}

int eZolTrib::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  int ret = enemy::takehit(wpnId,power,wpnx,wpny,wpnDir);
  if(sclk)
    sclk+=128;
  return ret;
}

eWallM::eWallM(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  haslink=false;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+1000;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eWallM::animate(int index)
{
  if(dying)
    return Dead();
  hxofs=1000;
  if(misc==0)
  {
    if(frame-wallm_load_clk>80 && !watch && clk>=0)
    {
      int wall=link_on_wall();
      int wallm_cnt=0;
      for(int i=0; i<guys.Count(); i++)
        if(guys.spr(i)->id==eWALLM)
      {
        register int m=((enemy*)guys.spr(i))->misc;
        if(m && ((enemy*)guys.spr(i))->clk3==(wall^1))
          ++wallm_cnt;
      }
      if(wall>0)
      {
        --wall;
        misc=1;
        clk2=0;
        clk3=wall^1;
        wallm_load_clk=frame;
        if(wall<=down)
        {
          if(LinkDir()==left)
            dir=right;
          else
            dir=left;
        }
        else
        {
          if(LinkDir()==up)
            dir=down;
          else
            dir=up;
        }
        switch(wall)
        {
          case up:    y=0;   break;
          case down:  y=160; break;
          case left:  x=0;   break;
          case right: x=240; break;
        }
        switch(dir)
        {
          case up:    y=LinkY()+48-(wallm_cnt&1)*12; flip=wall&1;           break;
          case down:  y=LinkY()-48+(wallm_cnt&1)*12; flip=((wall&1)^1)+2;   break;
          case left:  x=LinkX()+48-(wallm_cnt&1)*12; flip=(wall==up?2:0)+1; break;
          case right: x=LinkX()-48+(wallm_cnt&1)*12; flip=(wall==up?2:0);   break;
        }
      }
    }
  }
  else
    wallm_crawl();
  return enemy::animate(index);
}

void eWallM::wallm_crawl()
{
  hxofs=0;
  if(slide())
    return;
  if(dying || watch || (!haslink && stunclk))
    return;
  ++clk2;
  misc=(clk2/40)+1;
  switch(misc)
  {
    case 1:
    case 2: swap(dir,clk3); move(step); swap(dir,clk3); break;
    case 3:
    case 4:
    case 5: move(step); break;
    case 6:
    case 7: swap(dir,clk3); dir^=1; move(step); dir^=1; swap(dir,clk3); break;
    default: misc=0; break;
  }
}

void eWallM::grablink()
{
  haslink=true;
  superman=1;
}

void eWallM::draw(BITMAP *dest)
{
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  int tempdir=0;

  tile = dummy_int[0];

  switch(misc)
  {
    case 1:
    case 2: tempdir=clk3; break;
    case 3:
    case 4:
    case 5: tempdir=dir; break;
    case 6:
    case 7: tempdir=clk3^1; break;
  }
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (tempdir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    if(!haslink)
    {
      tile+=f2;
    }
  }
  else
  {
    if(!haslink)
    {
      tile += f2 ? 1 : 0;
    }
  }
  if(misc>0)
    masked_draw(dest,16,72,224,144);
  //    enemy::draw(dest);
  //    tile = clk&8 ? 128:129;
}

eTrap::eTrap(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  ox=x;                                                     //original x
  oy=y;                                                     //original y
  mainguy=false;
  count_enemy=false;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+420;
  }
  else
  {
    dummy_int[0]=tile;
  }
  dummy_int[1]=0;
}

bool eTrap::animate(int index)
{
  if(clk<0)
    return enemy::animate(index);

  if(misc==0)                                               // waiting
  {
    double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
    if ((ddir<=(((-1)*PI)/4))&&(ddir>(((-3)*PI)/4)))
    {
      dir=down;
    }
    else if ((ddir<=(((1)*PI)/4))&&(ddir>(((-1)*PI)/4)))
    {
      dir=right;
    }
    else if ((ddir<=(((3)*PI)/4))&&(ddir>(((1)*PI)/4)))
    {
      dir=up;
    }
    else
    {
      dir=left;
    }
    int d=lined_up(15);
    if (((d<left) && (id==eTRAP_H)) ||
      ((d>down) && (id==eTRAP_V)))
    {
      d=-1;
    }
    if(d!=-1 && trapmove(d))
    {
      dir=d;
      misc=1;
      clk2=(dir==down)?3:0;
    }
  }

  if(misc==1)                                               // charging
  {
    clk2=(clk2+1)&3;
    step=(clk2==3)?1:2;
    if(!trapmove(dir) || clip())
    {
      misc=2;
      dir=dir^1;
    }
    else
    {
      sprite::move(step);
    }
  }
  if(misc==2)                                               // retreating
  {
    step=(++clk2&1)?1:0;
    switch (dir)
    {
      case up:
      case left:
        if(int(x)<=ox && int(y)<=oy)
        {
          x=ox;
          y=oy;
          misc=0;
        }
        else
        {
          sprite::move(step);
        }
        break;
      case down:
      case right:
        if(int(x)>=ox && int(y)>=oy)
        {
          x=ox;
          y=oy;
          misc=0;
        }
        else
        {
          sprite::move(step);
        }
        break;
    }
  }

  return enemy::animate(index);
}

bool eTrap::trapmove(int ndir)
{
  if(get_bit(quest_rules,qr_MEANTRAPS))
  {
    if(tmpscr->flags2&fFLOATTRAPS)
      return canmove(ndir,(fix)1,spw_floater, 0, 0, 15, 15);
    return canmove(ndir,(fix)1,spw_water, 0, 0, 15, 15);
  }

  if(oy==80 && ndir<left)
    return false;
  if(oy<80 && ndir==up)
    return false;
  if(oy>80 && ndir==down)
    return false;
  if(ox<128 && ndir==left)
    return false;
  if(ox>128 && ndir==right)
    return false;
  return true;
}

bool eTrap::clip()
{
  if(get_bit(quest_rules,qr_MEANPLACEDTRAPS))
  {
    switch(dir)
    {
      case up:    if (y<=0)   return true; break;
      case down:  if (y>=160) return true; break;
      case left:  if (x<=0)   return true; break;
      case right: if (x>=240) return true; break;
    }
    return false;
  }
  else
  {
    switch(dir)
    {
      case up:    if(oy>80 && y<=86) return true; break;
      case down:  if(oy<80 && y>=80) return true; break;
      case left:  if(ox>128 && x<=124) return true; break;
      case right: if(ox<120 && x>=116) return true; break;
    }
    return false;
  }
}

void eTrap::draw(BITMAP *dest)
{
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  tile = dummy_int[0];

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    switch (id)
    {
      case eTRAP:
        tile+=dummy_int[1]*20;
        break;
      case eTRAP_V:
        tile+=60;
        break;
      case eTRAP_H:
        tile+=80;
        break;
    }
    tile+=f2;
  }
  enemy::draw(dest);
}

int eTrap::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  return 0;
}

eTrap2::eTrap2(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  lasthit=-1;
  lasthitclk=0;
  mainguy=false;
  count_enemy=false;
  step=2;

  if (id==eTRAP_LR)
  {
    dir=(x<=112)?right:left;
  }
  if (id==eTRAP_UD)
  {
    dir=(y<=72)?down:up;
  }
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+((id==eTRAP_LR)?540:520);
  }
  else
  {
    dummy_int[0]=tile;
  }

}

bool eTrap2::animate(int index)
{
  if(clk<0)

    return enemy::animate(index);

  if (!get_bit(quest_rules,qr_PHANTOMPLACEDTRAPS))
  {
    if (lasthitclk>0)
    {
      --lasthitclk;
    }
    else
    {
      lasthit=-1;
    }
    bool hitenemy=false;
    for(int j=0; j<guys.Count(); j++)
    {
      if ((j!=index) && (lasthit!=j))
      {
        if(hit(guys.spr(j)))
        {
          lasthit=j;
          lasthitclk=10;
          hitenemy=true;
          guys.spr(j)->lasthit=index;
          guys.spr(j)->lasthitclk=10;

          guys.spr(j)->dir=guys.spr(j)->dir^1;
        }
      }
    }
    if(!trapmove(dir) || clip() || hitenemy)
    {
      if(!trapmove(dir) || clip())
      {
        lasthit=-1;
        lasthitclk=0;
      }
      dir=dir^1;
    }
    sprite::move(step);
  }
  else
  {
    if(!trapmove(dir) || clip())
    {
      dir=dir^1;
    }
    sprite::move(step);
  }
  return enemy::animate(index);
}

bool eTrap2::trapmove(int ndir)
{
  if(tmpscr->flags2&fFLOATTRAPS)
    return canmove(ndir,(fix)1,spw_floater, 0, 0, 15, 15);
  return canmove(ndir,(fix)1,spw_water, 0, 0, 15, 15);

}

bool eTrap2::clip()
{
  switch(dir)
  {
    case up:    if(y<=0) return true; break;
    case down:  if(y>=160) return true; break;
    case left:  if(x<=0) return true; break;
    case right: if(x>=240) return true; break;
  }
  return false;
}

void eTrap2::draw(BITMAP *dest)
{
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  tile = dummy_int[0];

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    tile+=f2;
  }
  enemy::draw(dest);
}

int eTrap2::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  return 0;
}

eRock::eRock(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  mainguy=false;
  clk2=clk;
  hxofs=hyofs=-2;
  hxsz=hysz=20;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+1640;
    frate=16;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eRock::animate(int index)
{
  if(dying)
    return Dead();
  if(++clk2==0)                                             // start it
  {
    x=rand()&0xF0;
    y=0;
    clk3=0;
    clk2=rand()&15;
  }
  if(clk2>16)                                               // move it
  {
    if(clk3<=0)                                             // start bounce
    {
      dir=rand()&1;
      if(x<32)  dir=1;
      if(x>208) dir=0;
    }
    if(clk3<13+16)
    {
      x += dir ? 1 : -1;                                    //right, left
      dummy_int[1]=dir;
      if(clk3<2)
      {
        y-=2;                                               //up
        dummy_int[2]=(dummy_int[1]==1)?r_up:l_up;
      }
      else if(clk3<5)
      {
        --y;                                                //up
        dummy_int[2]=(dummy_int[1]==1)?r_up:l_up;
      }
      else if(clk3<8)
      {
        dummy_int[2]=(dummy_int[1]==1)?right:left;
      }
      else if(clk3<11)
      {
        ++y;                                                //down
        dummy_int[2]=(dummy_int[1]==1)?r_down:l_down;
      }
      else
      {
        y+=2;                                               //down
        dummy_int[2]=(dummy_int[1]==1)?r_down:l_down;
      }

      ++clk3;
    }
    else if(y<176)
      clk3=0;                                               // next bounce
    else
      clk2 = -(rand()&63);                                  // back to top
  }
  return enemy::animate(index);
}

void eRock::drawshadow(BITMAP *dest, bool translucent)
{
  int tempy=yofs;
  flip = 0;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  shadowtile = wpnsbuf[iwShadow].tile+f2;

  yofs+=8;
  yofs+=max(0,min(29-clk3,clk3));
  enemy::drawshadow(dest, translucent);
  yofs=tempy;
}

void eRock::draw(BITMAP *dest)
{
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  tile = dummy_int[0];
  if(clk>=0)
  {
    if (get_bit(quest_rules,qr_NEWENEMYTILES))
    {
      switch (dummy_int[2])
      {
        case up:
          flip=0;
          break;
        case down:
          flip=0; tile+=4;
          break;
        case left:
          flip=0; tile+=8;
          break;
        case right:
          flip=0; tile+=12;
          break;
        case l_up:
          flip=0; tile+=20;
          break;
        case r_up:
          flip=0; tile+=24;
          break;
        case l_down:
          flip=0; tile+=28;
          break;
        case r_down:
          flip=0; tile+=32;
          break;
      }
    }
    tile+=f2;
    enemy::draw(dest);
  }
}

int eRock::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  return 0;
}

eFBall::eFBall(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  /* fixing
      hp=1;
  */
  mainguy=false;
  count_enemy=false;
  superman=1;
  hxofs=1000;
  hclk=clk;                                                 // the "no fire" range
  clk=96;
}

bool eFBall::animate(int index)
{
  if(++clk>80)
  {
    unsigned r=rand();
    if(!(r&63) && !LinkInRange(hclk))
    {
      addEwpn(x,y,ewFireball,0,d->wdp,0);
      if(!((r>>7)&15))
        addEwpn(x-4,y,ewFireball,0,d->wdp,0);
      clk=0;
    }
  }
  return false;
}

void eFBall::draw(BITMAP *dest)
{
}

eTrigger::eTrigger(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  x=-1000; y=-1000;
}

void eTrigger::draw(BITMAP *dest)
{
}

eNPC::eNPC(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  dummy_int[0]=tile+wpnsbuf[iwNPCs].tile;
  count_enemy=false;
}

bool eNPC::animate(int index)
{
  if(dying)
    return Dead();

  switch (id)
  {
    case eNPCSTAND1:
    case eNPCSTAND2:
    case eNPCSTAND3:
    case eNPCSTAND4:
    case eNPCSTAND5:
    case eNPCSTAND6:
    {
      double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
      if ((ddir<=(((-1)*PI)/4))&&(ddir>(((-3)*PI)/4)))
      {
        dir=down;
      }

      else if ((ddir<=(((1)*PI)/4))&&(ddir>(((-1)*PI)/4)))
      {
        dir=right;
      }
      else if ((ddir<=(((3)*PI)/4))&&(ddir>(((1)*PI)/8)))
      {
        dir=up;
      }
      else
      {
        dir=left;
      }
    }
    break;
    case eNPCWALK1:
    case eNPCWALK2:
    case eNPCWALK3:
    case eNPCWALK4:
    case eNPCWALK5:
    case eNPCWALK6:
      halting_walk(d->rate, d->homing, 0, d->hrate, 48);

      if(clk2==1 && (misc < d->misc1) && !(rand()&15))
      {
        newdir(d->rate, d->homing, 0);
        clk2=48;
        ++misc;
      }

      if(clk2==0)
        misc=0;

      break;
  }
  return enemy::animate(index);
}

void eNPC::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);


  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;

        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }

    tile+=f2;
  }
  else
  {
    switch(dir)
    {
      case up:    flip = 0; break;
      case down:  flip = 0; tile += 2; break;
      case left:  flip = 0; tile += 4; break;
      case right: flip = 1; tile += 4; break;
    }
    tile+=(f2?1:0);
  }
  enemy::draw(dest);
}

int eNPC::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  return 0;
}

eZora::eZora(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,0)
{
  mainguy=false;
  count_enemy=false;
  if(iswater(tmpscr->data[(((int)y&0xF0)+(x>>4))]) &&
    (x>-17 && x<0))
  {
    clk=1;
  }
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+880;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

void eZora::facelink()
{
  if (Link.x-x==0)
  {
    dir=(Link.y+8<y)?up:down;
  }
  else
  {
    double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
    if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
    {
      dir=l_down;
    }
    else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
    {
      dir=down;
    }
    else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
    {
      dir=r_down;
    }
    else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
    {
      dir=right;
    }
    else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
    {
      dir=r_up;
    }
    else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
    {
      dir=up;
    }
    else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
    {
      dir=l_up;
    }
    else
    {
      dir=left;
    }
  }
}

bool eZora::animate(int index)
{
  if(dying)
    return Dead();

  if(watch)
  {
    ++clock_zoras;
    return true;
  }

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    facelink();
  }
  switch(clk)
  {
    case 0:                                                 // reposition him
    {
      int t=0;
      int pos=rand()%160 + 16;
      bool placed=false;
      while(!placed && t<160)
      {
        if(iswater(tmpscr->data[pos]) && (pos&15)>0 && (pos&15)<15)
        {
          x=(pos&15)<<4;
          y=pos&0xF0;
          hp=guysbuf[eZORA].hp;                             // refill life each time
          hxofs=1000;                                       // avoid hit detection
          stunclk=0;
          placed=true;
        }
        pos+=19;
        if(pos>=176)
          pos-=160;
        ++t;
      }
      if(!placed || whistleclk>=88)                         // can't place him, he's gone
        return true;

    } break;
    case 35:
      if (!get_bit(quest_rules,qr_NEWENEMYTILES))
      {
        dir=(Link.y+8<y)?up:down;
      }
      hxofs=0; break;
    case 35+19: addEwpn(x,y,ewFireball,0,d->wdp,0); break;
    case 35+66: hxofs=1000; break;
    case 198:   clk=-1; break;
  }

  return enemy::animate(index);
}

void eZora::draw(BITMAP *dest)
{
  if(clk<3)
    return;

  //shnarf
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/16)%4:((clk>=(frate>>1))?1:0);

  tile = dummy_int[0];

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
      case l_up:
        flip=0; tile+=20;
        break;
      case r_up:
        flip=0; tile+=24;
        break;
      case l_down:
        flip=0; tile+=28;
        break;
      case r_down:
        flip=0; tile+=32;
        break;
    }

    int dl;
    if ((clk>35)&&(clk<36+67))                              //surfaced
    {
      if ((clk>=(35+10))&&(clk<(38+56)))                    //mouth open
      {
        tile+=80;
      }                                                     //mouth closed
      else
      {
        tile+=40;
      }
      tile+=f2;
    }
    else
    {
      if (clk<36)
      {
        dl=clk+5;
      }
      else
      {
        dl=clk-36-66;
      }
      tile+=((dl/5)&3);
    }
  }
  else
  {
    int dl;
    if(clk<36)
    {
      dl=clk+5;
      goto waves2;
    }
    if(clk<36+66)
      tile=(dir==up)?113:112;
    else
    {
      dl=clk-36-66;
      waves2:
      tile=((dl/11)&1)+96;
    }
  }
  enemy::draw(dest);
}

eStalfos::eStalfos(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  doubleshot=false;

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+2380;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eStalfos::animate(int index)
{
  if(dying)
    return Dead();
  if(id==eSTALFOS)
    constant_walk(4,128,0);
  else
  {
    halting_walk(4,128,0,4,48);
    if(clk2==16 && sclk==0 && !stunclk && !watch)
    {
      if (id==eSTALFOS3)
      {
        Ewpns.add(new weapon(x,y,ewSword,0,d->wdp,up));
        Ewpns.add(new weapon(x,y,ewSword,0,d->wdp,down));
        Ewpns.add(new weapon(x,y,ewSword,0,d->wdp,left));
        Ewpns.add(new weapon(x,y,ewSword,0,d->wdp,right));
      }
      else
      {
        Ewpns.add(new weapon(x,y,ewSword,0,d->wdp,dir));
      }
    }
    if(clk2==1 && !doubleshot && !(rand()&15))
    {
      newdir(4,128,0);
      clk2=48;
      doubleshot=true;
    }
    if(clk2==0)
      doubleshot=false;
  }
  return enemy::animate(index);
}

void eStalfos::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    tile+=f2;
    if (id!=eSTALFOS)
    {
      tile+=20;
      if (id==eSTALFOS3)
      {
        tile+=60;
      }
      if (clk2>0)                                           //stopped to fire
      {
        tile+=20;
        if (clk2<17)                                        //firing
        {
          tile+=20;
        }
      }
    }
  }
  else
  {
    flip = f2?1:0;
  }
  enemy::draw(dest);
}

eGibdo::eGibdo(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=new_enemy_tile_start+560;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eGibdo::animate(int index)
{
  if(dying)
    return Dead();
  constant_walk(4,128,0);
  return enemy::animate(index);
}

void eGibdo::draw(BITMAP *dest)
{
  //    update_enemy_frame();
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    tile+=f2;
  }
  else
  {
    flip=f2?1:0;
  }
  enemy::draw(dest);
}

eBubble::eBubble(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  superman=1;
  mainguy=false;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+300;
  }
  else
  {
    dummy_int[0]=tile;
  }

}

bool eBubble::animate(int index)
{
  if(dying)
    return Dead();
  constant_walk();
  return enemy::animate(index);
}

void eBubble::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?

    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    switch (id)
    {
      case eBUBBLE:
        break;
      case eRBUBBLE:
        tile+=20;
        break;
      case eBBUBBLE:
        tile+=40;
        break;
      case eIBUBBLE:
        tile+=60;
        break;

      case eRIBUBBLE:
        tile+=80;
        break;
      case eBIBUBBLE:
        tile+=100;
        break;
    }
    tile+=f2;
  }
  else
  {
    flip  = f2 ? 1 : 0;
  }
  //    if((d->cset > 14)&&(!get_bit(quest_rules,qr_NOBUBBLEFLASH)))
  if(((id==eBUBBLE)||(id==eIBUBBLE))&&(!get_bit(quest_rules,qr_NOBUBBLEFLASH)))
    cs = (frame&3) + 6;
  enemy::draw(dest);
}

eRope::eRope(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  charging=false;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=new_enemy_tile_start+840;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eRope::animate(int index)
{
  if(dying)
    return Dead();
  charge_attack();
  return enemy::animate(index);
}

void eRope::charge_attack()
{
  if(slide())
    return;
  if(clk<0 || dir<0 || stunclk || watch)
    return;

  if(clk3<=0)
  {
    x=(fix)(int(x)&0xF0);
    y=(fix)(int(y)&0xF0);
    if(!charging)
    {
      int ldir = lined_up(7);
      if(ldir!=-1 && canmove(ldir))
      {
        dir=ldir;
        charging=true;
        step=1.5;
      }
      else newdir(4,0,0);
    }
    if(!canmove(dir))
    {
      newdir();
      charging=false;
      step=0.5;
    }
    clk3=int(16.0/step);
  }

  move(step);
  --clk3;
}

void eRope::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?

    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    tile+=f2;
    if(id==eROPE2)
    {
      tile+=20;
    }

  }
  else
  {
    tile+=f2?0:1;
    flip = dir==left ? 1:0;
  }
  if((id==eROPE2)&&(!get_bit(quest_rules,qr_NOROPE2FLASH)))
  {
    cs=(frame&3)+6;
  }
  enemy::draw(dest);
}

eKeese::eKeese(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  dir=(rand()&7)+8;
  step=0;
  movestatus=1;
  c=0;
  hxofs=2;
  hxsz=12;
  hyofs=4;
  hysz=8;

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets;
  }
  else
  {
    dummy_int[0]=tile;
  }
  dummy_int[1]=0;
}

bool eKeese::animate(int index)
{
  if(dying)
    return Dead();
  if (id==eBAT)
  {
    floater_walk(2,8,(fix)1,(fix)0,10,0,0);
  }
  else
  {
    floater_walk(2,8,(fix)0.625,(fix)0.0625,10,120,16);
  }

  int nx = real_x(x);
  int ny = real_y(y);
  if(ox!=nx || oy!=ny)
    ++c;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    if (c==4)
    {
      c=0;
    }
    dummy_int[1] = c;                                       // fix for BSZ style...
    foobyte=0;
  }
  else
  {
    if(c>=2)
    {
      foobyte ^= 1;                                         // fix for BSZ style...
      c=0;
    }
  }
  ox = nx;
  oy = ny;

  return enemy::animate(index);
}

void eKeese::drawshadow(BITMAP *dest, bool translucent)
{
  int tempy=yofs;
  flip = 0;
  shadowtile = wpnsbuf[iwShadow].tile+foobyte+dummy_int[1];
  yofs+=8;
  yofs+=int(step/(fix)0.0625);
  enemy::drawshadow(dest, translucent);
  yofs=tempy;
}

void eKeese::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir-8)                                          //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
        flip=0;
        break;
      case l_up:                                            //d
        flip=0; tile+=4;
        break;
      case l_down:                                          //l
        flip=0; tile+=8;
        break;
      case left:                                            //r
        flip=0; tile+=12;
        break;
      case r_down:                                          //ul
        flip=0; tile+=20;
        break;
      case down:                                            //ur
        flip=0; tile+=24;
        break;
      case r_up:                                            //dl
        flip=0; tile+=28;
        break;
      case right:                                           //dr
        flip=0; tile+=32;
        break;
    }
    switch (id)
    {
      case eKEESE1:
        break;
      case eKEESE2:
        tile+=40;
        break;
      case eKEESE3:
        tile+=80;
        break;
      case eBAT:
        tile+=3300;
        break;
    }
  }

  //    tile=dummy_int[1];
  tile+=foobyte;
  tile+=dummy_int[1];

  enemy::draw(dest);
}

eVire::eVire(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  dir = rand()&3;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+180;
    frate=16;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eVire::animate(int index)
{
  if(dying)
    return Dead();
  if(hp>0 && hp<guysbuf[eVIRE].hp && !fslide())
  {
    int kids = guys.Count();
    guys.add(new eKeese(x,y,eKEESE2,-24));
    guys.add(new eKeese(x,y,eKEESE2,-24));
    ((enemy*)guys.spr(kids))->count_enemy = false;
    ((enemy*)guys.spr(kids+1))->count_enemy = false;
    return true;
  }
  vire_hop();
  return enemy::animate(index);
}

void eVire::drawshadow(BITMAP *dest, bool translucent)
{
  int tempy=yofs;
  flip = 0;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  shadowtile = wpnsbuf[iwShadow].tile+f2;
  yofs+=8;
  if (dir>=left)
  {
    yofs+=(((int)y+17)&0xF0)-y;
    yofs-=3;
  }
  enemy::drawshadow(dest, translucent);
  yofs=tempy;
}

void eVire::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;

        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
  }
  tile+=f2;
  enemy::draw(dest);
}

void eVire::vire_hop()
{
  if(slide())
    return;
  if(clk<0 || dying || stunclk || watch)
    return;
  if(clk3<=0)
  {
    fix_coords();
    newdir(4,64,spw_none);
    clk3=32;
  }
  --clk3;
  move(step);
  if(dir>=left)
  {
    fix h = (fix)((31-clk3)*0.125 - 2.0);
    y+=h;
  }
}

eKeeseTrib::eKeeseTrib(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  clk4=0;
  dir=(rand()&7)+8;
  movestatus=1;
  c=0;
  hxofs=2;
  hxsz=12;
  hyofs=4;
  hysz=8;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+120;
  }
  else
  {
    dummy_int[0]=tile;
  }
  dummy_int[1]=0;
}

bool eKeeseTrib::animate(int index)
{
  ++clk4;
  if(dying)
    return Dead();
  floater_walk(2,8,(fix)0.625,(fix)0.0625,10,120,16);
  if (clk4==256)
  {
    if (!m_walkflag(x,y,0))
    {
      int kids = guys.Count();
      //        guys.add(new eVire(x,y,eVIRE,-24));
      //        keesetribgrow(x, y);
      if(get_bit(quest_rules, qr_OLDTRIBBLES))
      {
        guys.add(new eVire((fix)x,(fix)y,eVIRE,-24));
      }
      else
      {
        guys.add(new eVireTrib((fix)x,(fix)y,eVIRETRIB,-24));
      }
//      ((enemy*)guys.spr(kids))->count_enemy = false;
      ((enemy*)guys.spr(kids))->count_enemy = count_enemy;
      return true;
    }
    else
    {
      clk4=0;
    }
  }

  int nx = real_x(x);
  int ny = real_y(y);
  if(ox!=nx || oy!=ny)
    ++c;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    if (c==4)
    {
      c=0;
    }
    dummy_int[1] = c;                                       // fix for BSZ style...
    foobyte=0;
  }
  else
  {
    if(c>=2)
    {
      foobyte ^= 1;                                         // fix for BSZ style...
      c=0;
    }
  }
  ox = nx;
  oy = ny;

  return enemy::animate(index);
}

void eKeeseTrib::drawshadow(BITMAP *dest, bool translucent)
{
  int tempy=yofs;
  flip = 0;
  shadowtile = wpnsbuf[iwShadow].tile+foobyte+dummy_int[1];
  yofs+=8;
  yofs+=int(step/(fix)0.0625);
  enemy::drawshadow(dest, translucent);
  yofs=tempy;
}

void eKeeseTrib::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir-8)                                          //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
        flip=0;
        break;
      case l_up:                                            //d
        flip=0; tile+=4;
        break;
      case l_down:                                          //l
        flip=0; tile+=8;
        break;
      case left:                                            //r
        flip=0; tile+=12;
        break;
      case r_down:                                          //ul
        flip=0; tile+=20;
        break;
      case down:                                            //ur
        flip=0; tile+=24;
        break;
      case r_up:                                            //dl
        flip=0; tile+=28;
        break;
      case right:                                           //dr
        flip=0; tile+=32;
        break;
    }
  }

  //    tile=dummy_int[1];
  tile+=foobyte;
  tile+=dummy_int[1];

  enemy::draw(dest);
}

eVireTrib::eVireTrib(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  dir = rand()&3;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+160;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eVireTrib::animate(int index)
{
  if(dying)
    return Dead();
  if(hp>0 && hp<guysbuf[eKEESETRIB+1].hp && !fslide())
  {
    int kids = guys.Count();
    guys.add(new eKeeseTrib(x,y,eKEESETRIB,-24));
    guys.add(new eKeeseTrib(x,y,eKEESETRIB,-24));
    ((enemy*)guys.spr(kids))->count_enemy = false;
    ((enemy*)guys.spr(kids+1))->count_enemy = false;
    return true;
  }
  eVireTrib::vire_hop();
  return enemy::animate(index);
}

void eVireTrib::drawshadow(BITMAP *dest, bool translucent)
{
  int tempy=yofs;
  flip = 0;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  shadowtile = wpnsbuf[iwShadow].tile+f2;
  yofs+=8;
  if (dir>=left)
  {
    yofs+=(((int)y+17)&0xF0)-y;
  }
  enemy::drawshadow(dest, translucent);
  yofs=tempy;
}

void eVireTrib::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    tile+=f2;
  }
  else
  {
    if(dir==up) tile+=2;
    tile+=f2?1:0;
  }
  enemy::draw(dest);
}

void eVireTrib::vire_hop()
{
  if(slide())
    return;
  if(clk<0 || dying || stunclk || watch)
    return;
  if(clk3<=0)
  {
    fix_coords();
    newdir(4,64,spw_none);
    clk3=32;
  }
  --clk3;
  move(step);
  if(dir>=left)
  {
    fix h = (fix)((31-clk3)*0.125 - 2.0);
    y+=h;
  }
}

ePolsVoice::ePolsVoice(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  fy=y;
  shadowdistance=0;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=new_enemy_tile_start+580;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool ePolsVoice::animate(int index)
{
  if(dying)
    return Dead();
  polsvoice_hop();
  return enemy::animate(index);

}

void ePolsVoice::drawshadow(BITMAP *dest, bool translucent)
{
  int tempy=yofs;
  flip = 0;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  shadowtile = wpnsbuf[iwShadow].tile;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    shadowtile+=f2;
  }
  else
  {
    shadowtile+=f2?1:0;
  }
  yofs+=8;
  yofs-=shadowdistance;
  enemy::drawshadow(dest, translucent);
  yofs=tempy;
}

void ePolsVoice::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    tile+=f2;
    if (clk2>=0)
    {
      tile+=20;
    }
  }
  else
  {
    tile+=f2?1:0;
  }
  enemy::draw(dest);
}

int ePolsVoice::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying || clk<0 || hclk>0 || superman)
    return 0;

  switch(wpnId)
  {
    case wPhantom:
      return 0;
    case wBomb:
    case wSBomb:
    case wLitBomb:
    case wLitSBomb:
    case wFire:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:

    case wGSparkle:
    case wMSparkle:
      return 0;
    case wBrang:
      if (current_item(itype_brang,true)<3)
      {
        sfx(WAV_CHINK,pan(int(x)));
        return 1;
      }
      else
      {
        stunclk=160;
        break;
      }
    case wHookshot:
      stunclk=160;
      break;
    case wMagic:
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
    case wArrow:
      hp=0;
      break;
    default:
      hp-=power;
  }
  hclk=33;
  sfx(WAV_EHIT,pan(int(x)));
  return wpnId==wArrow ? 0 : 1;                             // arrow keeps going
}

void ePolsVoice::polsvoice_hop()
{
  if(slide())
    return;
  if(clk<0 || dying || stunclk || watch)
    return;
  swap(fy,y);
  if(clk3<=0)
  {
    fix_coords();
    if(clk2<=0 || !canmove(dir,(fix)1,spw_floater))
      newdir(4,32,spw_floater);
    if(clk2<=0)
    {
      fy=y;
      if(!canmove(dir,(fix)2,spw_none) || m_walkflag(x,y,spw_none) || !(rand()%3))
        clk2=32;
    }
    clk3=32;
  }
  --clk3;
  swap(fy,y);
  if(clk3&1)
  {
    if(dir==up)   fy-=step;
    if(dir==down) fy+=step;
    move(step);
    --clk2;
    if(clk2>0)
    {
      fix h = (fix)((32-clk2)*0.25 - 4.0);
      if(h<-2.5) h=-2.5;
      if(h> 2.5) h=2.5;
      y+=h;
      shadowdistance+=h;
    }
  }
}

eLikeLike::eLikeLike(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+280;
  }
  else
  {
    dummy_int[0]=tile;
  }
  haslink=false;
}

bool eLikeLike::animate(int index)
{
  if(hp<=0 && haslink)
  {
    Link.beatlikelike();
    haslink=false;
  }
  if(dying)
    return Dead();

  if(haslink)
  {
    ++clk2;
    if(clk2==95)
      game.items[itype_shield] &= ~i_largeshield;
    if((clk&0x18)==8)                                       // stop its animation on the middle frame
      --clk;
  }
  else
    constant_walk(4,64,spw_none);
  return enemy::animate(index);
}

void eLikeLike::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    tile+=f2;
    enemy::draw(dest);
  }
  else
  {
    switch(clk&0x18)
    {
      //        case 0:  tile=162; break;
      case 8:
      case 24: tile+=1; break;
      case 16: tile+=2; break;
    }
    enemy::draw(dest);
  }
}

void eLikeLike::eatlink()
{
  haslink=true;
  if (Link.isSwimming())
  {
    Link.setX(x);
    Link.setY(y);
  }
  else
  {
    x=Link.getX();
    y=Link.getY();
  }
  clk2=0;
}

eShooter::eShooter(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
}

bool eShooter::animate(int index)
{
  if(dying)
    return Dead();

  halting_walk(d->rate, d->homing, 0, d->hrate, 48);

  if(clk2==16 && sclk==0 && !stunclk && !watch)
    Ewpns.add(new weapon(x,y, d->weapon, 0, d->wdp, dir));

  if(clk2==1 && (misc < d->misc1) && !(rand()&15))
  {
    newdir(d->rate, d->homing, 0);
    clk2=48;
    ++misc;
  }

  if(clk2==0)
    misc=0;

  return enemy::animate(index);
}

void eShooter::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eOctorok::eOctorok(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+1840;

  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eOctorok::animate(int index)
{
  if(dying)
  {
    return Dead();
  }

  halting_walk(d->rate, d->homing, 0, d->hrate, 48);

  if(clk2==16 && sclk==0 && !stunclk && !watch)
    Ewpns.add(new weapon(x,y, d->weapon, 0, d->wdp, dir));

  if(clk2==1 && (misc < d->misc1) && !(rand()&15))
  {
    newdir(d->rate, d->homing, 0);
    clk2=48;
    ++misc;
  }

  if(clk2==0)
    misc=0;

  return enemy::animate(index);
}

void eOctorok::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    if (clk2>0)                                             //stopped to fire
    {
      tile+=20;
      if (clk2<17)                                          //firing
      {
        tile+=20;
      }
    }
    switch (id)
    {
      case eROCTO1:                                         //already the right tile
        break;
      case eROCTO2:
        tile+=60;
        break;
      case eBOCTO1:
        tile+=120;
        break;
      case eBOCTO2:
        tile+=180;
        break;
      case eCOCTO:
        tile+=240;
        break;
    }
    tile+=f2;
  }
  else
  {
    switch(dir)
    {
      case up:    flip = 2; break;
      case down:  flip = 0; break;
      case left:  flip = 0; tile += 2; break;
      case right: flip = 1; tile += 2; break;
    }
    tile+=(f2?1:0);
  }
  enemy::draw(dest);
}

eMoblin::eMoblin(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+2260;
    frate=16;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eMoblin::animate(int index)
{
  if(dying)
    return Dead();

  halting_walk(d->rate, d->homing, 0, d->hrate, 48);

  if(clk2==16 && sclk==0 && !stunclk && !watch)
    Ewpns.add(new weapon(x,y, d->weapon, 0, d->wdp, dir));

  if(clk2==1 && (misc < d->misc1) && !(rand()&15))
  {
    newdir(d->rate, d->homing, 0);
    clk2=48;
    ++misc;
  }

  if(clk2==0)
    misc=0;

  return enemy::animate(index);
}

void eMoblin::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    if (clk2>0)                                             //stopped to fire
    {
      tile+=20;
      if (clk2<17)                                          //firing
      {

        tile+=20;
      }
    }
    if (id==eBMOBLIN)
    {
      tile+=60;
    }

    tile+=f2;
  }
  else
  {
    switch(dir)
    {
      case up:    tile+=3; flip = f2 ? 1:0; break;
      case down:  tile+=2; flip = f2 ? 1:0; break;
      case left:  flip=1; tile += f2 ? 1:0; break;
      case right: flip=0; tile += f2 ? 1:0; break;
    }
  }
  enemy::draw(dest);
}

eLynel::eLynel(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+2520;
    frate=16;
  }
  else
  {
    dummy_int[0]=tile;

  }
}

bool eLynel::animate(int index)
{
  if(dying)
    return Dead();

  halting_walk(d->rate, d->homing, 0, d->hrate, 48);

  if(clk2==16 && sclk==0 && !stunclk && !watch)
    Ewpns.add(new weapon(x,y, d->weapon, 0, d->wdp, dir));

  if(clk2==1 && (misc < d->misc1) && !(rand()&15))
  {
    newdir(d->rate, d->homing, 0);
    clk2=48;
    ++misc;
  }

  if(clk2==0)
    misc=0;

  return enemy::animate(index);
}

void eLynel::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    if (clk2>0)                                             //stopped to fire
    {
      tile+=20;
      if (clk2<17)                                          //firing
      {
        tile+=20;
      }
    }
    if (id==eBLYNEL)
    {
      tile+=60;
    }
    tile+=f2;
  }
  else
  {
    switch(dir)
    {
      case up:    tile+=3; flip = f2 ? 1:0; break;
      case down:  tile+=2; flip = f2 ? 1:0; break;
      case left:  flip=1; tile += f2 ? 1:0; break;
      case right: flip=0; tile += f2 ? 1:0; break;
    }
  }
  enemy::draw(dest);
}

eGoriya::eGoriya(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+2140;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eGoriya::animate(int index)
{
  if(dying)
  {
    KillWeapon();
    return Dead();
  }
  if(!WeaponOut())
  {
    //      halting_walk(6,128,0,(id==eRGORIYA)?5:6,48);
    halting_walk(6,128,0,(id==eRGORIYA)?5:6,1);
    if(clk2==1 && sclk==0 && !stunclk && !watch)
    {
      misc=index+100;
      Ewpns.add(new weapon(x,y,ewBrang,misc,d->wdp,dir));
    }
  }
  else if(clk2>2)
    --clk2;
  return enemy::animate(index);
}

bool eGoriya::WeaponOut()
{
  for(int i=0; i<Ewpns.Count(); i++)
    if(((weapon*)Ewpns.spr(i))->type==misc && Ewpns.spr(i)->id==ewBrang)
      return true;
  return false;
}

void eGoriya::KillWeapon()
{
  for(int i=0; i<Ewpns.Count(); i++)
    if(((weapon*)Ewpns.spr(i))->type==misc && Ewpns.spr(i)->id==ewBrang)
      Ewpns.del(i);
}

void eGoriya::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    if (clk2>0)                                             //stopped to fire
    {

      tile+=20;
      if (clk2<17)                                          //firing
      {
        tile+=20;
      }
    }
    if (id==eBGORIYA)
    {
      tile+=60;
    }
    tile+=f2;
  }
  else
  {
    switch(dir)
    {
      case up:    tile+=1; flip=f2?1:0; break;

      case down:  flip=f2?1:0; break;
      case left:  flip=1; tile+=f2?3:2; break;
      case right: flip=0; tile+=f2?3:2; break;
    }
  }
  enemy::draw(dest);
}

eDarknut::eDarknut(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  noshield=false;
  fired=false;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+2640;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eDarknut::animate(int index)
{
  if(dying)
    return Dead();
  switch (id)
  {
    case eRDKNUT:
      constant_walk(5,144,0);
      break;
    case eBDKNUT:
      constant_walk(4,160,0);
      break;
    case eSDKNUT:
      constant_walk(3,200,0);
      if(hp<=0)
      {
        guys.add(new eDarknut(x,y,eBDKNUT,-25));
        guys.add(new eDarknut(x,y,eBDKNUT,-25));
      }
      break;
    case eDKNIGHT:
      //switch later to:  halting_walk(6,128,0,(id==eRGORIYA)?5:6,1);
      constant_walk(2,255,0);
      if(sclk==0 && !stunclk && !watch)
      {
        if (clk2>64)
        {
          clk2=0;
          fired=false;
        }
        clk2+=rand()&3;
        if ((clk2>24)&&(clk2<52))
        {
          tile+=20;                                         //firing
          if (!fired&&(clk2>=38))
          {
            Ewpns.add(new weapon(x,y, d->weapon, 0, d->wdp, dir));
            fired=true;
          }
        }
      }
      break;
  }
  return enemy::animate(index);
}

void eDarknut::draw(BITMAP *dest)

{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    switch (id)
    {
      case eRDKNUT:
        break;
      case eBDKNUT:
        tile+=20;
        break;
      case eSDKNUT:
        tile+=40;
        break;
      case eDKNIGHT:
        tile+=60;
        break;
    }
    if ((get_bit(quest_rules,qr_BRKNSHLDTILES)) && (noshield==true))
    {
      tile+=120;
    }
    tile+=f2;
  }
  else
  {
    if ((get_bit(quest_rules,qr_BRKNSHLDTILES)) && (noshield==true))
    {
      tile=860;
    }

    switch(dir)
    {
      case up:    tile+=2; flip=f2?1:0; break;
      case down:  flip=0; tile+=f2?0:1; break;
      case left:  flip=1; tile+=f2?3:4; break;
      case right: flip=0; tile+=f2?3:4; break;
    }
  }

  enemy::draw(dest);
}


int eDarknut::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying || clk<0 || hclk>0 || superman)
    return 0;

  switch(wpnId)
  {
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wFire:
    case wWind:
    case wSSparkle:
    case wGSparkle:
    case wMSparkle:
    case wFSparkle:
    case wPhantom:
      return 0;
    case wBomb:
    case wSBomb:
    case wRefMagic:
    case wHookshot:
    case wHammer:
      goto skip1;
  }


  if((wpnDir==(dir^1)) && !noshield)
  {
    sfx(WAV_CHINK,pan(int(x)));
    return 1;
  }

  skip1:

  switch(wpnId)
  {
    case wBrang:
      if ((current_item(itype_brang,true)<3) || ((dir==up)&&(wpny<y)) || ((dir==down)&&(wpny>y))
        || ((dir==left)&&(wpnx<x)) || ((dir==right)&&(wpnx>x)) )
      {
        sfx(WAV_CHINK,pan(int(x)));
        return 1;

      }
      else
      {
        stunclk=160;
        break;
      }

    case wHookshot:
      if ( (((dir==up)&&(wpny<y)) || ((dir==down)&&(wpny>y)) || ((dir==left)&&(wpnx<x)) || ((dir==right)&&(wpnx>x)))
        && (!noshield) )
      {
        sfx(WAV_CHINK,pan(int(x)));
        return 1;
      }
      else
      {
        stunclk=160;
        break;
      }

    case wArrow:
    case wMagic:
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
    case wBomb:
    case wSBomb:
      if((wpnDir==(dir^1)) && !noshield)
        break;
    case wHammer:
      if (wpnDir==(dir^1)&&get_bit(quest_rules,qr_BRKBLSHLDS))
        noshield=true;
    default:
      hp-=power;
  }

  hclk=33;
  if((dir&2)==(wpnDir&2))
    sclk=(wpnDir<<8)+16;
  sfx(WAV_EHIT,pan(int(x)));
  return 1;
}

eWizzrobe::eWizzrobe(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  switch(d->misc1)
  {
    case 0:
      hxofs=1000;
      fading=fade_invisible;
      clk+=220+14;
      break;
    default: dir=(loadside==right)?right:left; misc=-3; break;
  }
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {

    dummy_int[0]=new_enemy_tile_start+2880;
  }
  else
  {
    dummy_int[0]=tile;
  }
  charging=false;
  firing=false;
  fclk=0;
}

bool eWizzrobe::animate(int index)
{
  if(dying)
    return Dead();

  if(d->misc1)                                              //walking wizzrobe
    wizzrobe_attack();
  else
  {
    if(watch)
    {
      fading=0;
      hxofs=0;
    }
    else switch(clk)
    {
      case 0:
        if (id==eRWIZ)
        {
          place_on_axis(true);
        }
        else
        {
          int t=0;
          bool placed=false;
          while(!placed && t<160)
          {
            if (isdungeon())
            {
              x=((rand()%12)+2)*16;
              y=((rand()%7)+2)*16;
            }
            else
            {
              x=((rand()%14)+1)*16;
              y=((rand()%9)+1)*16;
            }
            if((!m_walkflag(x,y,1)) && ((abs((int)(x-Link.getX()))>=32) || (abs((int)(y-Link.getY()))>=32)))
            {
              //        if(iswater(tmpscr->data[pos]) && (pos&15)>0 && (pos&15)<15) {
              //               x=(pos&15)<<4;
              //               y=pos&0xF0;
              placed=true;
            }
            ++t;
          }
          if (abs((int)(x-Link.getX()))<abs((int)(y-Link.getY())))
          {
            if (y<Link.getY())
            {
              dir=down;
            }
            else
            {
              dir=up;
            }
          }
          else
          {
            if (x<Link.getX())
            {
              dir=right;
            }
            else
            {
              dir=left;
            }
          }

          if(!placed)                                       // can't place him, he's gone
            return true;
        }
        //         place_on_axis(true);
        fading=fade_flicker;
        hxofs=0;
        break;
      case 64:  fading=0; charging=true; break;
      case 73:  charging=false; firing=40; break;
      case 83:
        if (id==eRWIZ)
        {
          addEwpn(x,y,ewMagic,0,d->wdp,dir);
          sfx(WAV_WAND,pan(int(x)));
        }
        else if (id==eWWIZ)
        {
          addEwpn(x,y,ewWind,0,d->wdp,dir);
          sfx(WAV_WAND,pan(int(x)));
        }
        else
        {
          int bc=0;
          for (int gc=0; gc<guys.Count(); gc++)
          {
            if ((((enemy*)guys.spr(gc))->id) == eBAT)
            {
              ++bc;
            }
          }
          if (bc<=40)
          {
            int kids = guys.Count();
            int bats=(rand()%3)+1;
            for (int i=0; i<bats; i++)
            {
              guys.add(new eKeese(x,y,eBAT,-10));
              ((enemy*)guys.spr(kids+i))->count_enemy = false;
              sfx(WAV_FIRE,pan(int(x)));
            }
          }
        }
        break;
      case 119: firing=false; charging=true; break;
      case 128: fading=fade_flicker; charging=false; break;
      case 146: fading=fade_invisible; hxofs=1000; break;
      case 220: clk=-1; break;
    }
  }
  return enemy::animate(index);
}


void eWizzrobe::wizzrobe_attack()
{
  if(slide())
    return;
  if(clk<0 || dying || stunclk || watch)
    return;
  if(clk3<=0 || ((clk3&31)==0 && !canmove(dir,(fix)1,spw_door) && !misc))
  {
    fix_coords();
    switch(misc)
    {
      case 1:                                               //walking
        if(!m_walkflag(x,y,spw_door))
          misc=0;
        else
        {
          clk3=16;
          if(!canmove(dir,(fix)1,spw_wizzrobe))
            newdir(4,0,spw_wizzrobe);
        }
        break;

      case 2:                                               //phasing
      {
        int jx=x;
        int jy=y;
        int jdir=-1;
        switch(rand()&7)
        {
          case 0: jx-=32; jy-=32; jdir=15; break;
          case 1: jx+=32; jy-=32; jdir=9;  break;
          case 2: jx+=32; jy+=32; jdir=11; break;
          case 3: jx-=32; jy+=32; jdir=13; break;
        }
        if(jdir>0 && jx>=32 && jx<=208 && jy>=32 && jy<=128)
        {
          misc=3;
          clk3=32;
          dir=jdir;
          break;
        }
      }

      case 3:
        dir&=3;
        misc=0;

      case 0:
        newdir(4,64,spw_wizzrobe);

      default:
        if(!canmove(dir,(fix)1,spw_door))
        {
          if(canmove(dir,(fix)15,spw_wizzrobe))
          {
            misc=1;
            clk3=16;
          }
          else
          {
            newdir(4,64,spw_wizzrobe);
            misc=0;
            clk3=32;
          }
        }
        else
        {
          clk3=32;
        }
        break;
    }
    if(misc<0)
      ++misc;
  }
  --clk3;
  switch(misc)
  {
    case 1:
    case 3:  step=1.0; break;
    case 2:  step=0;   break;
    default: step=0.5; break;

  }

  move(step);
  if(d->misc1 && misc<=0 && clk3==28)
  {
    if ((id==eBWIZ)||(id==eMWIZ))
    {
      if(lined_up(8) == dir)
      {
        addEwpn(x,y,ewMagic,0,d->wdp,dir);
        sfx(WAV_WAND,pan(int(x)));
        fclk=30;
      }
    }
    else
    {
      if ((rand()%500)>=400)
      {
        addEwpn(x,y,ewFlame,0,d->wdp,up);
        addEwpn(x,y,ewFlame,0,d->wdp,down);
        addEwpn(x,y,ewFlame,0,d->wdp,left);
        addEwpn(x,y,ewFlame,0,d->wdp,right);
        addEwpn(x,y,ewFlame,0,d->wdp,l_up);
        addEwpn(x,y,ewFlame,0,d->wdp,r_up);
        addEwpn(x,y,ewFlame,0,d->wdp,l_down);
        addEwpn(x,y,ewFlame,0,d->wdp,r_down);

        sfx(WAV_FIRE,pan(int(x)));
        fclk=30;
      }
    }
  }
  if(misc==0 && (rand()&127)==0)
    misc=2;
  if(misc==2 && clk3==4)
    fix_coords();
}

void eWizzrobe::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  if(d->misc1 && (misc==1 || misc==3) && (clk3&1)
    && hp>0 && !watch && !stunclk)                          // phasing
    return;

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch(dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
      case 15:
        flip=0; tile+=20;
        break;
      case 9:
        flip=0; tile+=24;
        break;
      case 13:
        flip=0; tile+=28;
        break;
      case 11:
        flip=0; tile+=32;
        break;
    }
    if(d->misc1)                                            //walking wizzrobe
    {
      if (clk&8)
      {
        tile+=2;
      }
      if (clk&4)
      {
        tile+=1;
      }
      if (!(charging||firing))                              //should never be charging or firing for these wizzrobes
      {
        if (fclk>0)
        {
          tile+=40;
          --fclk;
        }
      }
    }
    else
    {
      if (charging||firing)
      {
        tile+=20;
        if (firing)
        {
          tile+=20;
        }
      }

      if (frame&4)
      {
        tile+=2;
      }
      if (frame&2)
      {
        tile+=1;
      }
    }

    switch (id)
    {
      case eBWIZ:
        break;
      case eFWIZ:
        tile+=80;
        break;
      case eMWIZ:
        tile+=160;
        break;
      case eRWIZ:
        tile+=240;
        break;
      case eWWIZ:
        tile+=300;
        break;
      case eBATROBE:
        tile+=360;
        break;
    }
  }
  else
  {

    if(d->misc1)
    {
      if (clk&8)
      {
        ++tile;
      }
    }
    else
    {
      if (frame&4)
      {
        ++tile;
      }
    }

    switch(dir)
    {
      case 9:
      case 15:
      case up:
        tile+=2;
        break;
      case down:
        break;
      case 13:
      case left:  flip=1; break;
      default:    flip=0; break;
    }
  }
  enemy::draw(dest);
}

int eWizzrobe::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying || clk<0 || hclk>0 || superman)
    return 0;

  switch(wpnId)

  {
    case wPhantom:
      return 0;
    case wFire:
    case wArrow:
    case wWand:
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    case wGSparkle:
    case wMSparkle:
    case wFSparkle:
      return 0;
    case wBrang:
      if (id==eMWIZ) return 0;

      if (current_item(itype_brang,true)<3)
      {
        sfx(WAV_CHINK,pan(int(x)));
        return 1;
      }
      else
      {
        stunclk=160;
        break;
      }
    case wHookshot:
      if (id==eMWIZ) return 0;
      stunclk=160;
      break;
    case wMagic:
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
    default:
      if ((id==eMWIZ)&&(wpnId!=wRefMagic)) return 0;
      hp-=power;
  }
  hclk=33;
  sfx(WAV_EHIT,pan(int(x)));
  return 1;
}

/*********************************/
/**********   Bosses   ***********/
/*********************************/

eDodongo::eDodongo(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if (get_bit(quest_rules,qr_DODONGOCOLORFIX))
  {
    loadpalset(csBOSS,pSprite(spDIG));
    cs=csBOSS;
  }
  fading=fade_flash_die;
  cont_sfx(WAV_DODONGO);
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+5120;
  }
  else
  {
    dummy_int[0]=tile;
  }

  if (dir==down&&y>=128)
  {
    dir=up;
  }
  if (dir==right&&x>=208)
  {
    dir=left;
  }
}

bool eDodongo::animate(int index)
{
  if(dying)
  {
    return Dead();
  }
  if(clk2)                                                  // ate a bomb
  {
    if(--clk2==0)
      hp-=misc;                                             // store bomb's power in misc
  }
  else
    constant_walk(4,64,spw_clipright);
  hxsz = (dir<=down) ? 16 : 32;
  //    hysz = (dir>=left) ? 16 : 32;
  return enemy::animate(index);
}

void eDodongo::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  if(clk<0)
  {
    enemy::draw(dest);
    return;
  }

  int fr = stunclk>0 ? 16 : 8;
  int fr4=0;

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    if(!dying && clk2>0 && clk2<=64)
    {
      // bloated
      if (clk2>=0)
      {
        fr4=3;
      }
      if (clk2>=16)
      {
        fr4=2;
      }

      if (clk2>=32)
      {
        fr4=1;
      }
      if (clk2>=48)
      {
        fr4=0;
      }
      switch(dir)
      {
        case up:    tile+=8+fr4; break;
        case down:  tile+=12+fr4; break;
        case left:
          tile+=29+(2*fr4);

          xofs=16;
          enemy::draw(dest);
          xofs=0;
          --tile;
          break;
        case right:
          tile+=49+(2*fr4);
          xofs=16;
          enemy::draw(dest);
          xofs=0;
          --tile;
          break;
      }
    }
    else if(!dying || clk2>19)
    {
      // normal
      switch(dir)
      {
        case up:    tile+=((clk&12)>>2); break;
        case down:  tile+=4+((clk&12)>>2); break;
        case left:

          tile+=21+((clk&12)>>1);
          xofs=16;
          enemy::draw(dest);
          xofs=0;
          --tile;
          break;
        case right:
          flip=0;
          tile+=41+((clk&12)>>1);
          xofs=16;
          enemy::draw(dest);
          xofs=0;
          --tile;
          break;
      }
    }
    enemy::draw(dest);
  }
  else
  {
    if(!dying && clk2>0 && clk2<=64)
    {
      // bloated
      switch(dir)
      {
        case up:    tile+=9; flip=0; break;
        case down:  tile+=7; flip=0; break;
        case left:
          flip=1;
          tile+=4;
          xofs=16;
          enemy::draw(dest);
          xofs=0;
          ++tile;
          break;
        case right:
          flip=0;
          tile+=5;
          xofs=16;
          enemy::draw(dest);
          xofs=0;
          --tile;
          break;
      }
    }
    else if(!dying || clk2>19)
    {
      // normal
      switch(dir)
      {
        case up:    tile+=8; flip=(clk&fr)?1:0; break;
        case down:  tile+=6; flip=(clk&fr)?1:0; break;
        case left:
          flip=1;
          tile+=(clk&fr)?2:0;
          xofs=16;
          enemy::draw(dest);
          xofs=0;
          ++tile;
          break;
        case right:
          flip=0;
          tile+=(clk&fr)?3:1;
          xofs=16;
          enemy::draw(dest);
          xofs=0;
          --tile;
          break;
      }
    }
    enemy::draw(dest);
  }
}

int eDodongo::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying || clk<0 || clk2>0 || superman)
    return 0;

  switch(wpnId)
  {
    case wPhantom:
      return 0;
    case wFire:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    case wGSparkle:
    case wMSparkle:
    case wFSparkle:
      return 0;
    case wLitBomb:
    case wLitSBomb:
      if(abs((int)(wpnx-((dir==right)?x+16:x))) > 7 || abs((int)(wpny-y)) > 7)
        return 0;
      clk2=96;
      misc=power;
      if(wpnId==wLitSBomb)
        item_set=isSBOMB100;
      return 1;
    case wBomb:
    case wSBomb:
      if(abs((int)(wpnx-((dir==right)?x+16:x))) > 8 || abs((int)(wpny-y)) > 8)
        return 0;
      stunclk=160;
      misc=wpnId;                                           // store wpnId
      return 1;
    case wSword:
    case wBeam:
      if(stunclk)
      {
        sfx(WAV_EHIT,pan(int(x)));
        hp=0;
        item_set = (misc==wSBomb) ? isSBOMB100 : isBOMB100;
        fading=0;                                           // don't flash
        return 1;
      }
    default:
      sfx(WAV_CHINK,pan(int(x)));
  }
  return 1;
}

void eDodongo::death_sfx()
{
  if(guys.idCount(id)<2)                                    // count self
    stop_sfx(WAV_DODONGO);
  sfx(WAV_GASP,pan(int(x)));
}

eDodongo2::eDodongo2(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  loadpalset(csBOSS,pSprite(spDIG));
  cs=csBOSS;
  fading=fade_flash_die;
  cont_sfx(WAV_DODONGO);
  dummy_int[0]=nets+5180;
  if (id==eDODONGOF)
  {
    dummy_int[0]+=80;
  }
  previous_dir=-1;

  if (dir==down&&y>=128)
  {
    dir=up;
  }
  if (dir==right&&x>=208)
  {
    dir=left;
  }
}

bool eDodongo2::animate(int index)
{
  if(dying)
  {
    return Dead();
  }
  if(clk2)                                                  // ate a bomb
  {
    if(--clk2==0)
      hp-=misc;                                             // store bomb's power in misc
  }
  else
    constant_walk(4,64,spw_clipbottomright);
  hxsz = (dir<=down) ? 16 : 32;
  return enemy::animate(index);
}

void eDodongo2::draw(BITMAP *dest)
{
  int fr4=0;
  tile=dummy_int[0];
  if(clk<0)
  {
    enemy::draw(dest);
    return;
  }

  if(!dying && clk2>0 && clk2<=64)
  {
    // bloated
    if (clk2>=0)
    {
      fr4=3;
    }
    if (clk2>=16)
    {
      fr4=2;
    }
    if (clk2>=32)
    {
      fr4=1;
    }
    if (clk2>=48)
    {
      fr4=0;
    }
    switch(dir)
    {
      case up:
        tile+=28+fr4;
        yofs+=16;
        enemy::draw(dest);
        yofs-=16;
        tile-=20;
        enemy::draw(dest);
        break;
      case down:
        tile+=12+fr4;
        yofs-=16;
        enemy::draw(dest);
        yofs+=16;
        tile+=20;
        enemy::draw(dest);
        break;
      case left:
        tile+=49+(2*fr4);
        xofs=16;
        enemy::draw(dest);
        xofs=0;
        --tile;
        enemy::draw(dest);
        break;
      case right:
        tile+=69+(2*fr4);
        xofs=16;
        enemy::draw(dest);
        xofs=0;
        --tile;
        enemy::draw(dest);
        break;
    }
  }
  else if(!dying || clk2>19)
  {
    // normal
    switch(dir)
    {
      case up:
        if (previous_dir==down)
        {
          y-=16;
        }
        tile+=20+((clk&24)>>3);
        yofs+=16;
        enemy::draw(dest);
        yofs-=16;
        tile-=20;
        enemy::draw(dest);
        break;
      case down:
        if (previous_dir!=down)
        {
          y+=16;
        }
        tile+=4+((clk&24)>>3);
        yofs-=16;
        enemy::draw(dest);
        yofs+=16;

        tile+=20;
        enemy::draw(dest);
        break;
      case left:
        tile+=40+((clk&24)>>2);
        enemy::draw(dest);
        xofs=16;
        ++tile;
        enemy::draw(dest);
        xofs=0;
        break;
      case right:
        tile+=61+((clk&24)>>2);
        xofs=16;
        enemy::draw(dest);
        xofs=0;
        --tile;
        enemy::draw(dest);
        break;
    }
  }
  previous_dir=dir;
  //    enemy::draw(dest);
}

int eDodongo2::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying || clk<0 || clk2>0 || superman)
    return 0;

  switch(wpnId)
  {
    case wPhantom:
      return 0;
    case wFire:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    case wGSparkle:
    case wMSparkle:
    case wFSparkle:
      return 0;
    case wLitBomb:
    case wLitSBomb:
      if(abs((int)(wpnx-((dir==right)?x+16:x))) > 7 || abs((int)(wpny-y)) > 7)
        return 0;
      clk2=96;
      misc=power;
      if(wpnId==wLitSBomb)
        item_set=isSBOMB100;
      return 1;
    case wBomb:
    case wSBomb:
      if(abs((int)(wpnx-((dir==right)?x+16:x))) > 8 || abs((int)(wpny-y)) > 8)
        return 0;
      stunclk=160;
      misc=wpnId;                                           // store wpnId
      return 1;
    case wSword:
    case wBeam:
      if(stunclk)
      {
        sfx(WAV_EHIT,pan(int(x)));
        hp=0;
        item_set = (misc==wSBomb) ? isSBOMB100 : isBOMB100;
        fading=0;                                           // don't flash
        return 1;
      }
    default:
      sfx(WAV_CHINK,pan(int(x)));
  }
  return 1;
}

void eDodongo2::death_sfx()
{
  if(guys.idCount(id)<2)                                    // count self
    stop_sfx(WAV_DODONGO);
  sfx(WAV_GASP,pan(int(x)));
}

eAquamentus::eAquamentus(fix X,fix Y,int Id,int Clk) : enemy((fix)176,(fix)64,Id,Clk)
{
  if (id==eLAQUAM)
  {
    x=64;
  }
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+5940;
    if (id==eRAQUAM)
    {
      dummy_int[0]+=8;
    }
  }
  else
  {
    dummy_int[0]=tile;
    if (id==eLAQUAM)
    {
      flip=1;
    }
  }

  loadpalset(csBOSS,pSprite(spAQUA));
  yofs=57;
  clk3=32;
  clk2=0;
  dir=left;
  cont_sfx(WAV_ROAR);
}

bool eAquamentus::animate(int index)
{
  if(dying)
    return Dead(index);
//  fbx=x+((id==eRAQUAM)?4:-4);
  fbx=x;
/*
  if (get_bit(quest_rules,qr_NEWENEMYTILES)&&id==eLAQUAM)
  {
    fbx+=16;
  }
*/
  if(--clk3==0)
  {
    addEwpn(fbx,y,ewFireball,0,d->wdp,up+1);
    addEwpn(fbx,y,ewFireball,0,d->wdp,0);
    addEwpn(fbx,y,ewFireball,0,d->wdp,down+1);
  }
  if(clk3<-80 && !(rand()&63))
  {
    clk3=32;
  }
  if(!((clk+1)&63))
  {
    int d=(rand()%3)+1;
    if(d>=left)
    {
      dir=d;
    }
    if (id==eLAQUAM)
    {
      if(x<=40)
      {
        dir=right;
      }
      if(x>=104)
      {
        dir=left;
      }
    }
    else
    {
      if(x<=136)
      {
        dir=right;
      }
      if(x>=200)
      {
        dir=left;
      }
    }
  }
  if(clk>=-1 && !((clk+1)&7))
  {
    if(dir==left)
    {
      x-=1;
    }
    else
    {
      x+=1;
    }
  }
  return enemy::animate(index);
}

void eAquamentus::draw(BITMAP *dest)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    xofs=(id==eLAQUAM?-16:0);
    tile=dummy_int[0]+((clk&24)>>2)+(clk3>-32?(clk3>0?40:80):0);
    if (dying)
    {
      xofs=0;
      enemy::draw(dest);
    }
    else
    {
      drawblock(dest,15);
    }
  }
  else
  {
    int xblockofs=((id==eLAQUAM)?-16:16);
    xofs=0;
    if(clk<0 || dying)
    {
      enemy::draw(dest);
      return;
    }
    // face (0=firing, 2=resting)
    tile=dummy_int[0]+((clk3>0)?0:2); enemy::draw(dest);
    // tail (
    tile=dummy_int[0]+((clk&16)?1:3); xofs=xblockofs;  enemy::draw(dest);
    // body
    yofs+=16;
    xofs=0;  tile=dummy_int[0]+((clk&16)?20:22); enemy::draw(dest);
    xofs=xblockofs; tile=dummy_int[0]+((clk&16)?21:23); enemy::draw(dest);
    yofs-=16;
  }
}

bool eAquamentus::hit(weapon *w)
{
  switch(w->id)
  {
    case wBeam:
    case wMagic: hysz=32;
  }
  bool ret = (dying || hclk>0) ? false : sprite::hit(w);
  hysz=16;
  return ret;

}

int eAquamentus::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying || clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wPhantom:
      return 0;
    case wFire:
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    case wGSparkle:
    case wMSparkle:
    case wFSparkle:
      return 0;
    case wHookshot:
    case wBrang:
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
    default:
      hp-=power;
      hclk=33;
  }
  sfx(WAV_EHIT,pan(int(x)));
  sfx(WAV_GASP,pan(int(x)));
  return 1;
}

void eAquamentus::death_sfx()
{
  if(guys.idCount(id)<2)                                    // count self
    stop_sfx(WAV_ROAR);
  sfx(WAV_GASP,pan(int(x)));
}

eGohma::eGohma(fix X,fix Y,int Id,int Clk) : enemy((fix)128,(fix)48,Id,0)
{
  hxofs=-16;
  hxsz=48;
  yofs=57;
  cont_sfx(WAV_DODONGO);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+5340;
    switch (id)
    {
      case eBGOHMA:
        dummy_int[0]+=20;
        break;
      case eGOHMA3:
        dummy_int[0]+=40;
        break;
      case eGOHMA4:
        dummy_int[0]+=60;
        break;
    }
  }
  else
  {
    dummy_int[0]=180;
  }

}

bool eGohma::animate(int index)
{
  if(dying)
    return Dead(index);
  if((clk&63)==0)
  {
    if(clk&64)
      dir^=1;
    else
      dir=rand()%3+1;
  }
  if((clk&63)==3)
  {
    switch (id)
    {
      case eRGOHMA:
      case eBGOHMA:
        addEwpn(x,y+2,ewFireball,1,d->wdp,0);
        break;
      case eGOHMA3:
        addEwpn(x,y+2,ewFireball,1,d->wdp,left+1);
        addEwpn(x,y+2,ewFireball,1,d->wdp,0);
        addEwpn(x,y+2,ewFireball,1,d->wdp,right+1);
        break;
    }
  }
  if ((id==eGOHMA4)&& clk3>=16 && clk3<116)
  {
    if (!(clk3%8))
    {
      addEwpn(x,y+4,ewFlame,0,4*DAMAGE_MULTIPLIER,255);
      sfx(WAV_FIRE,pan(int(x)));

      int i=Ewpns.Count()-1;
      weapon *ew = (weapon*)(Ewpns.spr(i));
      if (wpnsbuf[ewFLAME].frames>1)
      {
        ew->aframe=rand()%wpnsbuf[ewFLAME].frames;
      }
      for(int j=Ewpns.Count()-1; j>0; j--)
      {
        Ewpns.swap(j,j-1);
      }
    }
  }
  if(clk&1)
    move((fix)1);
  if(++clk3>=400)
    clk3=0;
  return enemy::animate(index);
}

void eGohma::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  if(clk<0 || dying)
  {
    enemy::draw(dest);
    return;
  }

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    // left side
    xofs=-16;
    flip=0;
    //      if(clk&16) tile=180;
    //      else { tile=182; flip=1; }
    tile+=(3*((clk&48)>>4));
    enemy::draw(dest);

    // right side
    xofs=16;
    //      tile=(180+182)-tile;
    tile=dummy_int[0];
    tile+=(3*((clk&48)>>4))+2;
    enemy::draw(dest);

    // body
    xofs=0;
    tile=dummy_int[0];
    //      tile+=(3*((clk&24)>>3))+2;
    if(clk3<16)
      tile+=7;
    else if(clk3<116)
      tile+=10;
    else if(clk3<132)
      tile+=7;
    else tile+=((clk3-132)&8)?4:1;
    enemy::draw(dest);

  }
  else
  {
    // left side
    xofs=-16;
    flip=0;
    if(!(clk&16)) { tile+=2; flip=1; }
    enemy::draw(dest);

    // right side
    tile=dummy_int[0];
    xofs=16;
    if((clk&16)) tile+=2;
    //      tile=(180+182)-tile;
    enemy::draw(dest);

    // body
    tile=dummy_int[0];
    xofs=0;
    if(clk3<16)
      tile+=4;
    else if(clk3<116)
      tile+=5;
    else if(clk3<132)
      tile+=4;
    else tile+=((clk3-132)&8)?3:1;
    enemy::draw(dest);

  }
}

int eGohma::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying || clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wFire:
    case wLitBomb:
    case wBomb:
    case wLitSBomb:
    case wSBomb:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    case wGSparkle:
    case wMSparkle:
    case wFSparkle:
    case wPhantom:
      return 0;
    case wArrow:
      if(wpnDir==up && abs(int(x)-wpnx)<=8 && clk3>=16 && clk3<116)
      {
        if ((id<eGOHMA3)||
          ((id==eGOHMA3)&&(current_item(itype_arrow,true)>=2))||
          ((id==eGOHMA4)&&(current_item(itype_arrow,true)==3)))
        {
          hp-=power;
          hclk=33;
          break;
          //        } else {
          //          return 0;
        }
      }
      // fall through
    default:
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
  }
  sfx(WAV_EHIT,pan(int(x)));
  sfx(WAV_GASP,pan(int(x)));
  return 1;
}

void eGohma::death_sfx()
{
  if(guys.idCount(id)<2)                                    // count self
    stop_sfx(WAV_DODONGO);
  sfx(WAV_GASP,pan(int(x)));
}

eLilDig::eLilDig(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  loadpalset(csBOSS,pSprite(spDIG));
  count_enemy=(id==(id&0xFF));
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+4360+(((id&0xFF)-eDIGPUP2)*40);
    if ((id&0xFF)==eDIGPUP1)

    {
      dummy_int[0]-=160;
    }
    frate=16;
  }
  else
  {
    dummy_int[0]=160;
  }

}

bool eLilDig::animate(int index)
{
  if(dying)
    return Dead(index);

  if(misc<=128)
  {
    if(!(++misc&31))
      step+=0.25;
  }
  variable_walk_8(2,16,spw_floater);
  return enemy::animate(index);
}

void eLilDig::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  //    tile = 160;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);


  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir-8)                                          //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
        flip=0;
        break;
      case l_up:                                            //d
        flip=0; tile+=4;
        break;
      case l_down:                                          //l
        flip=0; tile+=8;
        break;
      case left:                                            //r
        flip=0; tile+=12;
        break;
      case r_down:                                          //ul
        flip=0; tile+=20;
        break;
      case down:                                            //ur
        flip=0; tile+=24;
        break;
      case r_up:                                            //dl
        flip=0; tile+=28;
        break;
      case right:                                           //dr
        flip=0; tile+=32;
        break;
    }
    tile+=f2;
  }
  else
  {
    tile+=(clk>=6)?1:0;
  }
  enemy::draw(dest);
}

eBigDig::eBigDig(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  superman=1;
  hxofs=hyofs=-8;
  hxsz=hysz=32;
  loadpalset(csBOSS,pSprite(spDIG));
  cont_sfx(WAV_VADER);
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+4000;
    frate=8;
    if (id==eDIG3)
    {
      dummy_int[0]+=200;
    }
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eBigDig::animate(int index)
{
  if(dying)
    return Dead();
  switch(misc)
  {
    case 0: variable_walk_8(2,16,spw_floater,-8,-16,23,23); break;
    case 1: ++misc; break;
    case 2:
      if(id==eDIG3)
      {
        guys.add(new eLilDig(x,y,eDIGPUP2+0x1000,-15));
        guys.add(new eLilDig(x,y,eDIGPUP3+0x1000,-15));
        guys.add(new eLilDig(x,y,eDIGPUP4+0x1000,-15));
      }
      else
      {
        guys.add(new eLilDig(x,y,eDIGPUP1+0x1000,-15));
      }

      stop_sfx(WAV_VADER);
      sfx(WAV_GASP,pan(int(x)));
      return true;
  }
  return enemy::animate(index);
}

void eBigDig::draw(BITMAP *dest)
{
  tile = dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir-8)                                          //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
        flip=0;
        break;
      case l_up:                                            //d
        flip=0; tile+=8;
        break;
      case l_down:                                          //l
        flip=0; tile+=40;
        break;
      case left:                                            //r
        flip=0; tile+=48;
        break;
      case r_down:                                          //ul
        flip=0; tile+=80;
        break;
      case down:                                            //ur
        flip=0; tile+=88;

        break;
      case r_up:                                            //dl
        flip=0; tile+=120;
        break;
      case right:                                           //dr
        flip=0; tile+=128;
        break;
    }
    tile+=(f2*2);
  }
  else
  {
    tile+=(f2)?0:2;
    flip=(clk&1)?1:0;
  }
  xofs-=8; yofs-=8;
  drawblock(dest,15);
  xofs+=8; yofs+=8;
}

int eBigDig::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(wpnId==wWhistle && misc==0)
    misc=1;
  return 0;
}

eGanon::eGanon(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  loadpalset(csBOSS,pSprite(spGANON));
  hxofs=hyofs=8;

  clk2=70;
  misc=-1;
  mainguy=!getmapflag();
}

bool eGanon::animate(int index)
{
  if(dying)

    return Dead();

  switch(misc)
  {
    case -1: misc=0;
    case 0:
      if(++clk2>72 && !(rand()&3))
      {
        addEwpn(x,y,ewFireball,1,d->wdp,0);
        clk2=0;
      }
      Stunclk=0;
      constant_walk(5,72,spw_none);
      break;

    case 1:
    case 2:
      if(--Stunclk<=0)
      {
        int r=rand();
        if(r&1)
        {
          y=96;
          if(r&2)
            x=160;
          else
            x=48;
          if(tooclose(x,y,48))
            x=208-x;
        }
        loadpalset(csBOSS,pSprite(spGANON));
        misc=0;
      }
      break;
    case 3:
      if(hclk>0)
        break;
      misc=4;
      clk=0;
      hxofs=1000;
      loadpalset(9,pSprite(spPILE));
      music_stop();
      stop_sfx(WAV_ROAR);
      sfx(WAV_GASP,pan(int(x)));
      sfx(WAV_GANON);
      items.add(new item(x+8,y+8,iPile,ipDUMMY,0));
      break;

    case 4:
      if(clk>=80)
      {
        misc=5;
        if(getmapflag())
        {
          game.lvlitems[dlevel]|=liBOSS;
          play_DmapMusic();
          return true;
        }
        sfx(WAV_CLEARED);
        items.add(new item(x+8,y+8,iBigTri,ipBIGTRI,0));
      }
      break;
  }
  return enemy::animate(index);
}


int eGanon::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  switch(misc)
  {
    case 0:
      if(wpnId!=wSword)
        return 0;
      hp-=power;
      if(hp>0)
      {
        misc=1;
        Stunclk=64;
      }
      else
      {
        loadpalset(csBOSS,pSprite(spBROWN));
        misc=2;
        Stunclk=284;
        hp=guysbuf[eGANON].hp;                              //16*DAMAGE_MULTIPLIER;
      }
      sfx(WAV_EHIT,pan(int(x)));
      sfx(WAV_GASP,pan(int(x)));
      return 1;

    case 2:
      if(wpnId!=wArrow || current_item(itype_arrow,true)<2)
        return 0;
      misc=3;
      hclk=81;
      loadpalset(9,pSprite(spBROWN));
      return 1;
  }
  return 0;
}

void eGanon::draw(BITMAP *dest)
{
  switch(misc)
  {
    case 0:
      if((clk&3)==3)
        tile=(rand()%5)*2+260;
      if(db!=999)
        break;
    case 2:
      if(Stunclk<64 && (Stunclk&1))
        break;
    case -1:
    case 1:
    case 3:
      drawblock(dest,15);
      break;
    case 4:
      draw_guts(dest);
      draw_flash(dest);
      break;
  }
}

void eGanon::draw_guts(BITMAP *dest)
{
  int c = min(clk>>3,8);
  tile = clk<24 ? 74 : 75;
  overtile16(dest,tile,x+8,y+c+56,9,0);
  overtile16(dest,tile,x+8,y+16-c+56,9,0);
  overtile16(dest,tile,x+c,y+8+56,9,0);
  overtile16(dest,tile,x+16-c,y+8+56,9,0);
  overtile16(dest,tile,x+c,y+c+56,9,0);
  overtile16(dest,tile,x+16-c,y+c+56,9,0);
  overtile16(dest,tile,x+c,y+16-c+56,9,0);
  overtile16(dest,tile,x+16-c,y+16-c+56,9,0);
}

void eGanon::draw_flash(BITMAP *dest)
{

  int c = clk-(clk>>2);
  cs = (frame&3)+6;
  overtile16(dest,194,x+8,y+8-clk+56,cs,0);
  overtile16(dest,194,x+8,y+8+clk+56,cs,2);
  overtile16(dest,195,x+8-clk,y+8+56,cs,0);
  overtile16(dest,195,x+8+clk,y+8+56,cs,1);
  overtile16(dest,196,x+8-c,y+8-c+56,cs,0);
  overtile16(dest,196,x+8+c,y+8-c+56,cs,1);
  overtile16(dest,196,x+8-c,y+8+c+56,cs,2);
  overtile16(dest,196,x+8+c,y+8+c+56,cs,3);
}

void getBigTri()
{
  /*
  *************************
  * BIG TRIFORCE SEQUENCE *
  *************************
    0 BIGTRI out, WHITE flash in
    4 WHITE flash out, PILE cset white
    8 WHITE in
  ...
  188 WHITE out
  191 PILE cset red
  200 top SHUTTER opens
  209 bottom SHUTTER opens
  */
  sfx(WAV_CLEARED);
  guys.clear();

  draw_screen(tmpscr, 0, 0);

  for(int f=0; f<24*8 && !Quit; f++)
  {
    if(f==4)
    {
      for(int i=1; i<16; i++)
      {
        RAMpal[CSET(9)+i]=_RGB(63,63,63);
      }
    }
    if((f&7)==0)
    {
      for(int cs=2; cs<5; cs++)
      {
        for(int i=1; i<16; i++)
        {
          RAMpal[CSET(cs)+i]=_RGB(63,63,63);
        }
      }
      refreshpal=true;
    }
    if((f&7)==4)
    {
      loadlvlpal(DMaps[currdmap].color);
    }
    if(f==191)
    {
      loadpalset(9,pSprite(spPILE));
    }
    advanceframe();
  }

  play_DmapMusic();
  game.lvlitems[dlevel]|=liBOSS;
  setmapflag();
}

/**********************************/
/***  Multiple-Segment Enemies  ***/
/**********************************/

eMoldorm::eMoldorm(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  x=128; y=48;
  dir=(rand()&7)+8;
  superman=1;
  fading=fade_invisible;
  hxofs=1000;
  segcnt=clk;
  clk=0;
  id=guys.Count();
  yofs=56;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    tile=nets+1220;
  }
  else
  {
    tile=57;
  }
}

bool eMoldorm::animate(int index)
{

  if(clk2)
  {
    if(--clk2 == 0)
    {
      never_return(index);
      leave_item();
      return true;
    }
  }
  else
  {
    constant_walk_8(1,spw_floater);
    misc=dir;

    for(int i=index+1; i<index+segcnt+1; i++)
    {
      ((enemy*)guys.spr(i))->dummy_int[0]=tile;
      if ((i==index+segcnt)&&(i!=index+1))                  //tail
      {
        ((enemy*)guys.spr(i))->dummy_int[1]=2;
      }
      else
      {
        ((enemy*)guys.spr(i))->dummy_int[1]=1;
      }
      if (i==index+1)                                       //head
      {
        ((enemy*)guys.spr(i))->dummy_int[1]=0;
      }
      if(((enemy*)guys.spr(i))->hp <= 0)
      {
        for(int j=i; j<index+segcnt; j++)
        {
          swap(((enemy*)guys.spr(j))->hp,((enemy*)guys.spr(j+1))->hp);
          swap(((enemy*)guys.spr(j))->hclk,((enemy*)guys.spr(j+1))->hclk);
        }
        ((enemy*)guys.spr(i))->hclk=33;
        --segcnt;
      }
    }
    if(segcnt==0)
    {
      clk2=19;

      x=guys.spr(index+1)->x;
      y=guys.spr(index+1)->y;
    }
  }
  return false;
}

esMoldorm::esMoldorm(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  x=128; y=48;
  yofs=56;
  hyofs=4;
  hxsz=hysz=8;
  hxofs=1000;
  mainguy=count_enemy=false;
}

bool esMoldorm::animate(int index)
{
  if(dying)
    return Dead();
  if(clk>=0)
  {
    hxofs=4;
    if(!(clk&15))
    {
      misc=dir;
      dir=((enemy*)guys.spr(index-1))->misc;
    }
    if (!watch)
    {
      sprite::move(step);
    }
  }
  return enemy::animate(index);
}

int esMoldorm::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(enemy::takehit(wpnId,power,wpnx,wpny,wpnDir))
    return (wpnId==wSBomb) ? 1 : 2;                         // force it to wait a frame before checking sword attacks again
  return 0;
}

void esMoldorm::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    tile+=dummy_int[1]*40;
    switch (dir-8)                                          //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
        flip=0;
        break;
      case l_up:                                            //d
        flip=0; tile+=4;
        break;
      case l_down:                                          //l
        flip=0; tile+=8;
        break;
      case left:                                            //r
        flip=0; tile+=12;
        break;
      case r_down:                                          //ul
        flip=0; tile+=20;
        break;
      case down:                                            //ur
        flip=0; tile+=24;
        break;
      case r_up:                                            //dl
        flip=0; tile+=28;
        break;
      case right:                                           //dr
        flip=0; tile+=32;

        break;
    }
    tile+=f2;
  }

  if(clk>=0)
    enemy::draw(dest);
}

void esMoldorm::death_sfx()
{
  sfx(WAV_GASP,pan(int(x)));
}

eLanmola::eLanmola(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  x=64; y=80;
  dir=up;
  superman=1;
  fading=fade_invisible;
  hxofs=1000;
  segcnt=clk;
  clk=0;
  id=guys.Count();
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+((id==eRCENT)?1100:1160);
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eLanmola::animate(int index)
{
  if(clk2)
  {
    if(--clk2 == 0)
    {
      leave_item();
      return true;
    }
    return false;
  }


  constant_walk(8,0,spw_none);
  misc=dir;

  for(int i=index+1; i<index+segcnt+1; i++)
  {
    ((enemy*)guys.spr(i))->dummy_int[0]=dummy_int[0];
    if ((i==index+segcnt)&&(i!=index+1))
    {
      ((enemy*)guys.spr(i))->dummy_int[1]=1;                //tail
    }
    else
    {
      ((enemy*)guys.spr(i))->dummy_int[1]=0;
    }
    if(((enemy*)guys.spr(i))->hp <= 0)
    {
      for(int j=i; j<index+segcnt; j++)
      {
        swap(((enemy*)guys.spr(j))->hp,((enemy*)guys.spr(j+1))->hp);
        swap(((enemy*)guys.spr(j))->hclk,((enemy*)guys.spr(j+1))->hclk);
      }
      ((enemy*)guys.spr(i))->hclk=33;
      --segcnt;
    }
  }
  if(segcnt==0)
  {
    clk2=19;
    x=guys.spr(index+1)->x;
    y=guys.spr(index+1)->y;
    setmapflag(mTMPNORET);
  }
  return enemy::animate(index);
}

esLanmola::esLanmola(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  x=64; y=80;
  crate=(id&0xFFF)==eRCENT?7:3;
  hxofs=1000;
  hxsz=8;
  mainguy=false;
  count_enemy=(id<0x2000)?true:false;
}

bool esLanmola::animate(int index)
{
  if(dying)
  {
    xofs=0;
    return Dead();
  }
  if(clk>=0)
  {
    hxofs=4;
    if(!(clk&crate))
    {
      misc=dir;
      dir=((enemy*)guys.spr(index-1))->misc;
    }
    if (!watch)
    {
      sprite::move(step);
    }
  }
  return enemy::animate(index);
}

int esLanmola::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(enemy::takehit(wpnId,power,wpnx,wpny,wpnDir))
    return (wpnId==wSBomb) ? 1 : 2;                         // force it to wait a frame before checking sword attacks again
  return 0;
}

void esLanmola::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    if (id>=0x2000)
    {
      tile+=20;
      if (dummy_int[1]==1)
      {
        tile+=20;
      }
    }
    switch (dir)
    {
      case up:
        flip=0;
        break;
      case down:
        flip=0; tile+=4;
        break;
      case left:
        flip=0; tile+=8;
        break;
      case right:
        flip=0; tile+=12;
        break;
    }
    tile+=f2;
  }
  else
  {
    if (id>=0x2000)
    {
      tile+=1;
    }
  }

  if(clk>=0)
    enemy::draw(dest);
}

eManhandla::eManhandla(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,0)
{
  superman=1;
  dir=(rand()&7)+8;
  armcnt=((id==eMANHAN)?4:8);
  for(int i=0; i<armcnt; i++)
    arm[i]=i;
  cont_sfx(WAV_VADER);
  fading=fade_blue_poof;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    frate=16;
    dummy_int[0]=nets+4680;
    if (id==eMANHAN2)
    {
      dummy_int[0]+=160;
    }
  }
  else
  {
    dummy_int[0]=tile;
  }
  adjusted=false;
}

bool eManhandla::animate(int index)
{
  if(dying)
    return Dead(index);


  // check arm status, move dead ones to end of group
  for(int i=0; i<armcnt; i++)
  {
    if (!adjusted)
    {
      if (id==eMANHAN)
      {
        ((enemy*)guys.spr(index+i+1))->dummy_int[0]=dummy_int[0]+40;
      }
      else
      {
        ((enemy*)guys.spr(index+i+1))->dummy_int[0]=dummy_int[0]+160;
      }
    }
    if(((enemy*)guys.spr(index+i+1))->dying)
    {
      for(int j=i; j<armcnt-1; j++)
      {
        swap(arm[j],arm[j+1]);
        guys.swap(index+j+1,index+j+2);

      }
      --armcnt;
    }
  }
  adjusted=true;
  // move or die
  if(armcnt==0)
    hp=0;
  else
  {
    step=(((id==eMANHAN)?5:9)-armcnt)*0.5;
    int dx1=0, dy1=-8, dx2=15, dy2=15;
    if (id==eMANHAN)
    {
      for(int i=0; i<armcnt; i++)
      {
        switch(arm[i])
        {
          case 0: dy1=-24; break;
          case 1: dy2=31;  break;
          case 2: dx1=-16; break;
          case 3: dx2=31;  break;
        }
      }
    }
    else
    {
      dx1=-8, dy1=-16, dx2=23, dy2=23;
      for(int i=0; i<armcnt; i++)
      {
        switch(arm[i]&3)
        {
          case 0: dy1=-32; break;
          case 1: dy2=39;  break;
          case 2: dx1=-24; break;
          case 3: dx2=39;  break;
        }
      }
    }
    variable_walk_8(2,16,spw_floater,dx1,dy1,dx2,dy2);
    for(int i=0; i<armcnt; i++)
    {
      fix dx=(fix)0,dy=(fix)0;
      if (id==eMANHAN)
      {
        switch(arm[i])
        {
          case 0: dy=-16; break;
          case 1: dy=16;  break;
          case 2: dx=-16; break;
          case 3: dx=16;  break;
        }
      }
      else
      {
        switch(arm[i])
        {
          case 0: dy=-24; dx=-8; break;
          case 1: dy=24;  dx=8;  break;
          case 2: dx=-24; dy=8; break;
          case 3: dx=24;  dy=-8;  break;
          case 4: dy=-24; dx=8; break;
          case 5: dy=24;  dx=-8;  break;
          case 6: dx=-24; dy=-8; break;
          case 7: dx=24;  dy=8;  break;
        }
      }
      guys.spr(index+i+1)->x = x+dx;
      guys.spr(index+i+1)->y = y+dy;
    }
  }
  return enemy::animate(index);
}

void eManhandla::death_sfx()
{
  if(guys.idCount(id)<2)                                    // count self
    stop_sfx(WAV_VADER);
  sfx(WAV_GASP,pan(int(x)));
}

int eManhandla::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying)
    return 0;
  switch(wpnId)
  {
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wFire:
    case wWind:
    case wSSparkle:
    case wGSparkle:
    case wMSparkle:
    case wFSparkle:
    case wPhantom:
      return 0;
    case wHookshot:
    case wBrang: sfx(WAV_CHINK,pan(int(x))); break;
    default:     sfx(WAV_EHIT,pan(int(x)));

  }
  return 1;
}

void eManhandla::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    if (id==eMANHAN)
    {
      switch (dir-8)                                        //directions get screwed up after 8.  *shrug*
      {
        case up:                                            //u
          flip=0;
          break;
        case l_up:                                          //d
          flip=0; tile+=4;
          break;
        case l_down:                                        //l
          flip=0; tile+=8;
          break;
        case left:                                          //r
          flip=0; tile+=12;
          break;
        case r_down:                                        //ul
          flip=0; tile+=20;
          break;
        case down:                                          //ur
          flip=0; tile+=24;
          break;
        case r_up:                                          //dl
          flip=0; tile+=28;
          break;
        case right:                                         //dr
          flip=0; tile+=32;
          break;
      }
      tile+=f2;
      enemy::draw(dest);
    }                                                       //manhandla 2, big body
    else
    {

      switch (dir-8)                                        //directions get screwed up after 8.  *shrug*
      {
        case up:                                            //u
          flip=0;
          break;
        case l_up:                                          //d
          flip=0; tile+=8;
          break;
        case l_down:                                        //l
          flip=0; tile+=40;
          break;
        case left:                                          //r
          flip=0; tile+=48;
          break;
        case r_down:                                        //ul
          flip=0; tile+=80;
          break;
        case down:                                          //ur
          flip=0; tile+=88;
          break;
        case r_up:                                          //dl
          flip=0; tile+=120;
          break;
        case right:                                         //dr
          flip=0; tile+=128;
          break;
      }
      tile+=(f2*2);
      xofs-=8;  yofs-=8;  drawblock(dest,15);
      xofs+=8;  yofs+=8;
    }
  }
  else
  {
    if (id==eMANHAN)
    {
      enemy::draw(dest);
    }
    else
    {
      xofs-=8;  yofs-=8;  enemy::draw(dest);
      xofs+=16;           enemy::draw(dest);
      yofs+=16; enemy::draw(dest);
      xofs-=16;           enemy::draw(dest);
      xofs+=8;  yofs-=8;
    }
  }
}

esManhandla::esManhandla(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  id=misc=clk;
  clk=0;
  mainguy=count_enemy=false;
  dummy_bool[0]=false;
  frate=16;
}

bool esManhandla::animate(int index)

{
  if(dying)
    return Dead();
  if(--clk2<=0)
  {
    clk2=unsigned(rand())%5+5;
    clk3^=1;
  }
  if(!(rand()&127))
    addEwpn(x,y,ewFireball,1,d->wdp,0);
  return enemy::animate(index);
}

int esManhandla::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying || hclk>0)
    return 0;
  switch(wpnId)
  {
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wFire:
    case wWind:
    case wSSparkle:
    case wGSparkle:
    case wMSparkle:
    case wFSparkle:
    case wPhantom:
      return 0;
    case wHookshot:
    case wBrang: sfx(WAV_CHINK,pan(int(x))); break;
    default:
      hp-=power;
      hclk=33;
      sfx(WAV_EHIT,pan(int(x)));
      sfx(WAV_GASP,pan(int(x)));
  }
  return 1;
}

void esManhandla::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch(misc&3)
    {
      case up:             break;
      case down:  tile+=4; break;
      case left:  tile+=8; break;
      case right: tile+=12; break;
    }
    tile+=f2;
  }
  else
  {
    switch(misc&3)
    {
      case down:  flip=2;
      case up:    tile=(clk3)?188:189; break;
      case right: flip=1;
      case left:  tile=(clk3)?186:187; break;
    }
  }
  enemy::draw(dest);
}

eGleeok::eGleeok(fix X,fix Y,int Id,int Clk) : enemy((fix)120,(fix)48,Id,Clk)
{
  flameclk=0;
  misc=clk;                                                 // total head count
  clk3=clk;                                                 // live head count
  clk=0;
  clk2=60;                                                  // fire ball clock
  //    hp=(guysbuf[eGLEEOK2+(misc-2)].misc2)*(misc-1)*DAMAGE_MULTIPLIER+guysbuf[eGLEEOK2+(misc-2)].hp;
  hp=(guysbuf[id].misc2)*(misc)*DAMAGE_MULTIPLIER+guysbuf[id].hp;
  cs=csBOSS;
  if (id>=eGLEEOK1F)
  {
    loadpalset(csBOSS,pSprite(spGLEEOKF));
  }
  else
  {
    loadpalset(csBOSS,pSprite(spGLEEOK));
  }
  hxofs=4;
  hxsz=8;
  //    frate=17*4;
  cont_sfx(WAV_ROAR);
  fading=fade_blue_poof;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+5420;
    necktile=dummy_int[0]+8;
    if (id>=eGLEEOK1F)
    {
      dummy_int[0]+=12;
      necktile+=20;
    }
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool eGleeok::animate(int index)
{
  if(dying)
    return Dead(index);

  //fix for the "kill all enemies" item
  if (hp==-1000)
  {
    for (int i=0; i<clk3; ++i)
    {
      ((enemy*)guys.spr(index+i+1))->hp=1;                   // re-animate each head,
      ((enemy*)guys.spr(index+i+1))->misc = -1;              // disconnect it,
      ((enemy*)guys.spr(index+i+1))->animate(index+i+1);     // let it animate one frame,
      ((enemy*)guys.spr(index+i+1))->hp=-1000;               // and kill it for good
    }
    clk3=0;
    for(int i=0; i<misc; i++)
      ((enemy*)guys.spr(index+i+1))->misc = -2;             // give the signal to disappear
  }

  for(int i=0; i<clk3; i++)
  {
    enemy *head = ((enemy*)guys.spr(index+i+1));
    head->dummy_int[1]=necktile;
    if (id>=eGLEEOK1F)
    {
      head->dummy_bool[0]=true;
    }
    if(head->hclk)
    {
      if(hclk==0)
      {
        hp -= 1000 - head->hp;
        hclk = 33;
        sfx(WAV_GASP);
        sfx(WAV_EHIT,pan(int(head->x)));
      }
      head->hp = 1000;
      head->hclk = 0;
    }
  }

  if(hp<=(guysbuf[id].misc2)*(clk3-1)*DAMAGE_MULTIPLIER)
  {
    ((enemy*)guys.spr(index+clk3))->misc = -1;              // give signal to fly off
    hp=(guysbuf[id].misc2)*(--clk3)*DAMAGE_MULTIPLIER;
  }

  if (id<eGLEEOK1F)
  {
    if(++clk2>72 && !(rand()&3))
    {
      int i=rand()%misc;
      enemy *head = ((enemy*)guys.spr(index+i+1));
      addEwpn(head->x,head->y,ewFireball,1,d->wdp,0);
      clk2=0;
    }
  }
  else
  {
    if(++clk2>100 && !(rand()&3))
    {
      enemy *head = ((enemy*)guys.spr(rand()%misc+index+1));
      head->timer=rand()%50+50;
      clk2=0;
    }
  }

  if(hp<=0)
  {
    for(int i=0; i<misc; i++)
      ((enemy*)guys.spr(index+i+1))->misc = -2;             // give the signal to disappear
    never_return(index);
  }
  return enemy::animate(index);
}

int eGleeok::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  return 0;
}

void eGleeok::death_sfx()
{
  if(guys.idCount(id)<2)                                    // count self
    stop_sfx(WAV_ROAR);
  sfx(WAV_GASP,pan(int(x)));
}

void eGleeok::draw(BITMAP *dest)
{
  tile=dummy_int[0];

  if(dying)
  {
    enemy::draw(dest);
    return;
  }

  int f=clk/17;

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    // body
    xofs=-8; yofs=32;
    switch(f)

    {
      case 0: tile+=0; break;
      case 1: tile+=2; break;
      case 2: tile+=4; break;
      default: tile+=6; break;
    }
  }
  else
  {
    // body
    xofs=-8; yofs=32;
    switch(f)
    {
      case 0: tile+=0; break;
      case 2: tile+=4; break;
      default: tile+=2; break;
    }
  }
  enemy::drawblock(dest,15);
}

void eGleeok::draw2(BITMAP *dest)
{
  // the neck stub
  tile=necktile;
  xofs=0;
  yofs=56;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    tile+=((clk&24)>>3);
  }
  else
  {
    tile=145;
  }
/*
  if(hp>0 && !dont_draw())
    sprite::draw(dest);
*/
  if(hp > 0 && !dont_draw())
  {
    if ((tmpscr->flags3&fINVISROOM)&& !(current_item(itype_amulet,true)))
      sprite::drawcloaked(dest);
    else
      sprite::draw(dest);
  }
}

esGleeok::esGleeok(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  dummy_bool[0]=false;
  timer=0;
  /*  fixing */
  hp=1000;
  cs=csBOSS;
  step=1;
  item_set=0;
  //*/
  x=120; y=70;
  hxofs=4;
  hxsz=8;
  yofs=56;
  clk2=clk;                                                 // how long to wait before moving first time
  clk=0;
  mainguy=count_enemy=false;
  dir=rand();
  clk3=((dir&2)>>1)+2;                                      // left or right
  dir&=1;                                                   // up or down
  for(int i=0; i<4; i++)
  {
    nx[i]=124;
    ny[i]=i*6+48;
  }
}

bool esGleeok::animate(int index)
{
  //  set up the head tiles
  headtile=nets+5588;                                       //5580, actually.  must adjust for direction later on
  if (dummy_bool[0])                                        //if this is a flame gleeok
  {
    headtile+=180;
  }
  flyingheadtile=headtile+60;

  //  set up the neck tiles
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    necktile=dummy_int[1]+((clk&24)>>3);
  }
  else
  {
    necktile=145;
  }
  //    ?((dummy_bool[0])?(nets+4052+(16+((clk&24)>>3))):(nets+4040+(8+((clk&24)>>3)))):145)

  switch(misc)
  {
    case 0:                                                 // live head
      //  set up the attached head tiles
      if (get_bit(quest_rules,qr_NEWENEMYTILES))
      {
        tile=headtile;
        tile+=((clk&24)>>3);
        /*
                if (dummy_bool[0]) {
                  tile+=1561;
                }
        */
      }
      else
      {
        tile=146;
      }
      if(++clk2>=0 && !(clk2&3))
      {
        if(y<=56) dir=down;
        if(y>=80) dir=up;
        if(y<=58 && !(rand()&31))                           // y jig
        {
          dir^=1;
        }
        sprite::move(step);
        if(clk2>=4)
        {
          clk3^=1;
          clk2=-4;
        }
        else
        {
          if(x<=96)
          {
            clk3=right;
          }
          if(x>=144)
          {
            clk3=left;
          }
          if(y<=72 && !(rand()&15))
          {
            clk3^=1;                                        // x jig
          }
          else
          {
            if(y<=64 && !(rand()&31))
            {
              clk3^=1;                                      // x switch back
            }
            clk2=-4;
          }
        }
        swap(dir,clk3);
        sprite::move(step);
        swap(dir,clk3);

        for(int i=1; i<4; i++)
        {
          nx[i] = ( ( ( (i*x) + (4-i)*120) ) >>2 ) + (rand()%3) + 3;
          ny[i] = ( ( ( (i*y) + (4-i)*48) ) >>2 ) + (rand()%3);
        }
      }
      break;
    case 1:                                                 // flying head
      if(clk>=0)

      {
        variable_walk_8(2,9,spw_floater);
        //        if(++clk2>=80 && !(rand()%63)) {
        //          addEwpn(x,y,ewFireball,1,d->wdp,0);
        //          clk2=0;
        //          }
      }

      break;
      // the following are messages sent from the main guy...
    case -1:                                                // got chopped off
    {
      misc=1;
      superman=1;
      hxofs=xofs=0;
      hxsz=16;
      cs=8;
      clk=-24;
      clk2=40;
      dir=(rand()&7)+8;
      step=8.0/9.0;
    } break;
    case -2:                                                // the big guy is dead
      return true;
  }

  if (timer)
  {
    if (!(timer%8))
    {
      addEwpn(x,y+4,ewFlame,0,4*DAMAGE_MULTIPLIER,255);
      sfx(WAV_FIRE,pan(int(x)));

      int i=Ewpns.Count()-1;
      weapon *ew = (weapon*)(Ewpns.spr(i));
      if (wpnsbuf[ewFLAME].frames>1)
      {
        ew->aframe=rand()%wpnsbuf[ewFLAME].frames;
      }
      for(int j=Ewpns.Count()-1; j>0; j--)
      {
        Ewpns.swap(j,j-1);
      }
    }
    --timer;
  }

  return enemy::animate(index);
}

int esGleeok::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wPhantom:
      return 0;
    case wArrow:
    case wMagic:
    case wRefMagic:
    case wHookshot:
    case wBrang:
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
    case wWand:
    case wBeam:
    case wHammer:
    case wSword:
      hp-=power;
      hclk=33;
      return 2;                                             // force it to wait a frame before checking sword attacks again
  }
  return 0;
}

void esGleeok::draw(BITMAP *dest)
{
  switch(misc)
  {
    case 0:                                                 //neck
/*
      if(!dont_draw())
      {
        for(int i=1; i<4; i++)                              //draw the neck
        {
          if (get_bit(quest_rules,qr_NEWENEMYTILES))
          {
            overtile16(dest,necktile+(i*40),nx[i]-4,ny[i]+56,cs,0);
          }
          else
          {
            overtile16(dest,necktile,nx[i]-4,ny[i]+56,cs,0);
          }
        }
      }
*/

      if(!dont_draw())
      {
        for(int i=1; i<4; i++)                              //draw the neck
        {
          if (get_bit(quest_rules,qr_NEWENEMYTILES))
          {
            if ((tmpscr->flags3&fINVISROOM)&& !(current_item(itype_amulet,true)))
              overtilecloaked16(dest,necktile+(i*40),nx[i]-4,ny[i]+56,0);
            else
              overtile16(dest,necktile+(i*40),nx[i]-4,ny[i]+56,cs,0);
          }
          else
          {
            if ((tmpscr->flags3&fINVISROOM)&& !(current_item(itype_amulet,true)))
              overtilecloaked16(dest,necktile,nx[i]-4,ny[i]+56,0);
            else
              overtile16(dest,necktile,nx[i]-4,ny[i]+56,cs,0);
          }
        }
      }

      break;

    case 1:                                                 //flying head
      if (get_bit(quest_rules,qr_NEWENEMYTILES))
      {
        tile=flyingheadtile;
        tile+=((clk&24)>>3);
        /*
                if (dummy_bool[0]) {
                  tile+=1561;
                }
        */
        break;
      }
      else
      {
        tile=(clk&1)?147:148;
        break;
      }
  }
}

void esGleeok::draw2(BITMAP *dest)
{
  enemy::draw(dest);
}

ePatra::ePatra(fix X,fix Y,int Id,int Clk) : enemy((fix)128,(fix)48,Id,Clk)
{
  adjusted=false;
  dir=(rand()&7)+8;
  step=0.25;
  cont_sfx(WAV_VADER);
  flycnt=8; flycnt2=0;
  if ((id==ePATRAL2)||(id==ePATRAL3))
  {
    flycnt2=8;
    hp=22*DAMAGE_MULTIPLIER;
  }
  loopcnt=0;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+3440;
    switch (id)
    {
      case ePATRA1:
        break;
      case ePATRA2:
        dummy_int[0]+=80;
        break;
      case ePATRAL2:
        dummy_int[0]+=160;
        break;
      case ePATRAL3:
        dummy_int[0]+=360;
        break;
    }
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool ePatra::animate(int index)
{
  if(dying)
    return Dead(index);
  variable_walk_8(3,8,spw_floater);
  if(++clk2>84)
  {
    clk2=0;
    if(loopcnt)
      --loopcnt;
    else
    {
      if(id==ePATRA1||id==ePATRAL2||id==ePATRAL3)
      {
        if((misc%6)==0)
          loopcnt=3;
      }
      else
      {
        if((misc&7)==0)
          loopcnt=4;
      }
    }
    ++misc;
  }
  double size=1;;
  for(int i=index+1; i<index+flycnt+1; i++)
  {                                                         //outside ring
    if (!adjusted)
    {
      if (get_bit(quest_rules,qr_NEWENEMYTILES))
      {
        switch (id)
        {
          case ePATRA1:
            ((enemy*)guys.spr(i))->dummy_int[0]=dummy_int[0]+40;
            break;
          case ePATRA2:
            ((enemy*)guys.spr(i))->dummy_int[0]=dummy_int[0]+40;
            break;
          case ePATRAL2:
            ((enemy*)guys.spr(i))->dummy_int[0]=dummy_int[0]+160;
            break;
          case ePATRAL3:
            ((enemy*)guys.spr(i))->dummy_int[0]=dummy_int[0]+160;
            break;
        }
      }
      else
      {
        ((enemy*)guys.spr(i))->dummy_int[0]=dummy_int[0]+1;
      }
      //        ((enemy*)guys.spr(i))->dummy_int[0]=192;
      if(id==ePATRAL2||id==ePATRAL3)
      {
        ((enemy*)guys.spr(i))->hp=9*DAMAGE_MULTIPLIER;
      }
      else
      {
        ((enemy*)guys.spr(i))->hp=6*DAMAGE_MULTIPLIER;
      }
    }
    if(((enemy*)guys.spr(i))->hp <= 0)
    {
      for(int j=i; j<index+flycnt+flycnt2; j++)
      {
        guys.swap(j,j+1);
      }
      --flycnt;
    }
    else
    {
      int pos = ((enemy*)guys.spr(i))->misc;
      double a = (clk2-pos*10.5)*PI/42;

      if(id==ePATRA1||id==ePATRAL2||id==ePATRAL3)
      {
        if(loopcnt>0)
        {
          guys.spr(i)->x =  cos(a+PI/2)*56*size - sin(pos*PI/4)*28*size;
          guys.spr(i)->y = -sin(a+PI/2)*56*size + cos(pos*PI/4)*28*size;
        }
        else
        {
          guys.spr(i)->x =  cos(a+PI/2)*28*size;
          guys.spr(i)->y = -sin(a+PI/2)*28*size;
        }
        temp_x=guys.spr(i)->x;
        temp_y=guys.spr(i)->y;
      }
      else
      {
        circle_x =  cos(a+PI/2)*42;
        circle_y = -sin(a+PI/2)*42;

        if(loopcnt>0)
        {
          guys.spr(i)->x =  cos(a+PI/2)*42;
          guys.spr(i)->y = (-sin(a+PI/2)-cos(pos*PI/4))*21;
        }
        else
        {
          guys.spr(i)->x = circle_x;
          guys.spr(i)->y = circle_y;
        }
        temp_x=circle_x;
        temp_y=circle_y;
      }
      double ddir=atan2(double(temp_y),double(temp_x));
      if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
      {
        guys.spr(i)->dir=l_down;
      }
      else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
      {
        guys.spr(i)->dir=left;
      }
      else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
      {
        guys.spr(i)->dir=l_up;
      }
      else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
      {
        guys.spr(i)->dir=up;
      }
      else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
      {
        guys.spr(i)->dir=r_up;
      }
      else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
      {
        guys.spr(i)->dir=right;
      }
      else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
      {

        guys.spr(i)->dir=r_down;
      }
      else
      {
        guys.spr(i)->dir=down;
      }
      guys.spr(i)->x += x;
      guys.spr(i)->y += y;
    }
  }

  size=.5;

  if ((id==ePATRAL2)||(id==ePATRAL3))
  {
    if (id==ePATRAL2)
    {
      if(!(rand()&127))
      {
        addEwpn(x,y,ewFireball,1,d->wdp,0);
      }
    }
    for(int i=index+flycnt+1; i<index+flycnt+flycnt2+1; i++)//inner ring
    {
      if (!adjusted)
      {
        ((enemy*)guys.spr(i))->hp=12*DAMAGE_MULTIPLIER;
        if (get_bit(quest_rules,qr_NEWENEMYTILES))
        {
          switch (id)
          {
            case ePATRAL2:
              ((enemy*)guys.spr(i))->dummy_int[0]=dummy_int[0]+120;
              break;
            case ePATRAL3:
              ((enemy*)guys.spr(i))->dummy_int[0]=dummy_int[0]+40;
              break;
          }
        }
        else
        {
          ((enemy*)guys.spr(i))->dummy_int[0]=dummy_int[0]+1;
        }
      }
      if (flycnt>0)
      {
        ((enemy*)guys.spr(i))->superman=true;
      }
      else
      {
        ((enemy*)guys.spr(i))->superman=false;
      }
      if(((enemy*)guys.spr(i))->hp <= 0)
      {
        for(int j=i; j<index+flycnt+flycnt2; j++)
        {
          guys.swap(j,j+1);
        }
        --flycnt2;
      }
      else
      {
        if (id==ePATRAL3)
        {
          if(!(rand()&127))
          {
            addEwpn(guys.spr(i)->x,guys.spr(i)->y,ewFireball,1,d->wdp,0);
          }
        }
        int pos = ((enemy*)guys.spr(i))->misc;
        double a = ((clk2-pos*10.5)*PI/(42));

        if(loopcnt>0)
        {
          guys.spr(i)->x =  cos(a+PI/2)*56*size - sin(pos*PI/4)*28*size;
          guys.spr(i)->y = -sin(a+PI/2)*56*size + cos(pos*PI/4)*28*size;
        }
        else
        {
          guys.spr(i)->x =  cos(a+PI/2)*28*size;
          guys.spr(i)->y = -sin(a+PI/2)*28*size;
        }

        temp_x=guys.spr(i)->x;
        temp_y=guys.spr(i)->y;

        double ddir=atan2(double(temp_y),double(temp_x));
        if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
        {
          guys.spr(i)->dir=l_down;
        }
        else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
        {
          guys.spr(i)->dir=left;

        }
        else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
        {
          guys.spr(i)->dir=l_up;
        }
        else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
        {
          guys.spr(i)->dir=up;
        }
        else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
        {
          guys.spr(i)->dir=r_up;
        }
        else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
        {
          guys.spr(i)->dir=right;
        }
        else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
        {
          guys.spr(i)->dir=r_down;
        }
        else
        {
          guys.spr(i)->dir=down;
        }
        guys.spr(i)->x += x;
        guys.spr(i)->y = y-guys.spr(i)->y;

      }
    }
  }
  adjusted=true;
  return enemy::animate(index);
}

void ePatra::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
    if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
    {
      lookat=l_down;
    }
    else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
    {
      lookat=down;
    }
    else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
    {
      lookat=r_down;
    }
    else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
    {
      lookat=right;
    }
    else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
    {
      lookat=r_up;
    }
    else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
    {
      lookat=up;
    }
    else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
    {
      lookat=l_up;
    }
    else
    {
      lookat=left;
    }
    switch (lookat)                                         //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
        flip=0;
        break;
      case down:                                            //d
        flip=0; tile+=4;
        break;
      case left:                                            //l
        flip=0; tile+=8;
        break;
      case right:                                           //r
        flip=0; tile+=12;
        break;
      case l_up:                                            //ul
        flip=0; tile+=20;
        break;
      case r_up:                                            //ur
        flip=0; tile+=24;
        break;
      case l_down:                                          //dl
        flip=0; tile+=28;
        break;
      case r_down:                                          //dr
        flip=0; tile+=32;
        break;
    }
    tile+=((clk&3));
  }
  else
  {
    flip = (clk&2)>>1;
  }
  enemy::draw(dest);
}

int ePatra::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wPhantom:
      return 0;
    case wArrow:
    case wMagic:
    case wRefMagic:
    case wHookshot:
    case wBrang:
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
    case wWand:
    case wBeam:
    case wHammer:
    case wSword:
      if(flycnt||flycnt2)
        return 0;
      hp-=power;
      hclk=33;
      sfx(WAV_EHIT,pan(int(x)));
      sfx(WAV_GASP,pan(int(x)));
      return 1;
  }
  return 0;
}

void ePatra::death_sfx()
{
  if(guys.idCount(id)<2)                                    // count self
    stop_sfx(WAV_VADER);
  sfx(WAV_GASP,pan(int(x)));
}

esPatra::esPatra(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  cs=8;
  item_set=0;
  misc=clk;
  clk = -((misc*21)>>1)-1;
  yofs=56;
  hyofs=2;
  hxsz=hysz=12;
  hxofs=1000;
  mainguy=count_enemy=false;
  dummy_int[0]=0;
}

bool esPatra::animate(int index)
{
  if(dying)
    return Dead();
  hxofs=4;
  return enemy::animate(index);
}

int esPatra::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wPhantom:
      return 0;
    case wArrow:
    case wMagic:
    case wRefMagic:
    case wHookshot:
    case wBrang:
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
    case wWand:
    case wBeam:
    case wHammer:
    case wSword:
      hp-=power;
      hclk=33;
      sfx(WAV_EHIT,pan(int(x)));
      return 1;
  }
  return 0;
}

void esPatra::draw(BITMAP *dest)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    tile = dummy_int[0]+(clk&3);
    switch (dir)                                            //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
        flip=0;
        break;
      case down:                                            //d
        flip=0; tile+=4;
        break;
      case left:                                            //l
        flip=0; tile+=8;
        break;
      case right:                                           //r
        flip=0; tile+=12;
        break;
      case l_up:                                            //ul
        flip=0; tile+=20;
        break;
      case r_up:                                            //ur
        flip=0; tile+=24;
        break;
      case l_down:                                          //dl
        flip=0; tile+=28;
        break;
      case r_down:                                          //dr
        flip=0; tile+=32;
        break;
    }
  }
  else
  {
    tile = dummy_int[0]+((clk&2)>>1);
  }
  if(clk>=0)
    enemy::draw(dest);
}

ePatraBS::ePatraBS(fix X,fix Y,int Id,int Clk) : enemy((fix)128,(fix)48,Id,Clk)
{
  adjusted=false;
  dir=(rand()&7)+8;
  step=0.25;
  cont_sfx(WAV_VADER);
  flycnt=6; flycnt2=0;
  loopcnt=0;
  hxsz = 32;
  loadpalset(csBOSS,pSprite(spDIG));
  cs=csBOSS;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    dummy_int[0]=nets+4480;
  }
  else
  {
    dummy_int[0]=tile;
  }
}

bool ePatraBS::animate(int index)
{
  if(dying)
    return Dead(index);
  variable_walk_8(3,8,spw_floater);
  if(++clk2>90)
  {
    clk2=0;

    if(loopcnt)
      --loopcnt;
    else
    {
      if((misc&7)==0)
        loopcnt=4;
    }
    ++misc;
  }
  //    double size=1;;
  for(int i=index+1; i<index+flycnt+1; i++)
  {
    if (!adjusted)
    {
      ((enemy*)guys.spr(i))->hp=6*DAMAGE_MULTIPLIER;
      if (get_bit(quest_rules,qr_NEWENEMYTILES))
      {
        ((enemy*)guys.spr(i))->dummy_int[0]=dummy_int[0]+160;
      }
      else
      {
        ((enemy*)guys.spr(i))->dummy_int[0]=dummy_int[0]+1;
      }
    }
    if(((enemy*)guys.spr(i))->hp <= 0)
    {
      for(int j=i; j<index+flycnt+flycnt2; j++)
      {
        guys.swap(j,j+1);
      }
      --flycnt;
    }
    else
    {
      int pos = ((enemy*)guys.spr(i))->misc;
      double a = (clk2-pos*15)*PI/45;
      temp_x =  cos(a+PI/2)*45;
      temp_y = -sin(a+PI/2)*45;

      if(loopcnt>0)
      {
        guys.spr(i)->x =  cos(a+PI/2)*45;
        guys.spr(i)->y = (-sin(a+PI/2)-cos(pos*PI/3))*22.5;
      }
      else
      {
        guys.spr(i)->x = temp_x;
        guys.spr(i)->y = temp_y;
      }
      double ddir=atan2(double(temp_y),double(temp_x));
      if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
      {
        guys.spr(i)->dir=l_down;
      }
      else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
      {
        guys.spr(i)->dir=left;
      }
      else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
      {
        guys.spr(i)->dir=l_up;
      }
      else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
      {
        guys.spr(i)->dir=up;
      }
      else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
      {
        guys.spr(i)->dir=r_up;
      }
      else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
      {
        guys.spr(i)->dir=right;
      }
      else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
      {
        guys.spr(i)->dir=r_down;
      }
      else
      {
        guys.spr(i)->dir=down;
      }
      guys.spr(i)->x += x;
      guys.spr(i)->y += y;
    }
  }

  adjusted=true;
  return enemy::animate(index);
}

void ePatraBS::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
    if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
    {
      lookat=l_down;
    }
    else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
    {
      lookat=down;
    }
    else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
    {
      lookat=r_down;
    }
    else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
    {
      lookat=right;
    }
    else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
    {
      lookat=r_up;
    }
    else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
    {
      lookat=up;
    }
    else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
    {
      lookat=l_up;
    }
    else
    {
      lookat=left;
    }
    switch (lookat)                                         //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
        flip=0;
        break;
      case down:                                            //d
        flip=0; tile+=8;
        break;
      case left:                                            //l
        flip=0; tile+=40;
        break;
      case right:                                           //r
        flip=0; tile+=48;
        break;
      case l_up:                                            //ul
        flip=0; tile+=80;
        break;
      case r_up:                                            //ur
        flip=0; tile+=88;
        break;
      case l_down:                                          //dl
        flip=0; tile+=120;
        break;
      case r_down:                                          //dr
        flip=0; tile+=128;
        break;
    }
    tile+=(2*(clk&3));
    xofs-=8;  yofs-=8;  drawblock(dest,15);
    xofs+=8;  yofs+=8;
  }
  else
  {
    flip=(clk&1);
    xofs-=8;  yofs-=8;  enemy::draw(dest);
    xofs+=16;           enemy::draw(dest);
    yofs+=16; enemy::draw(dest);
    xofs-=16;           enemy::draw(dest);
    xofs+=8;  yofs-=8;
  }
}

int ePatraBS::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wPhantom:
      return 0;
    case wArrow:
    case wMagic:
    case wRefMagic:
    case wHookshot:
    case wBrang:
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
    case wWand:
    case wBeam:
    case wHammer:
    case wSword:
      if(flycnt||flycnt2)
        return 0;
      hp-=power;
      hclk=33;
      sfx(WAV_EHIT,pan(int(x)));
      sfx(WAV_GASP,pan(int(x)));
      return 1;
  }
  return 0;
}

void ePatraBS::death_sfx()
{
  if(guys.idCount(id)<2)                                    // count self
    stop_sfx(WAV_VADER);
  sfx(WAV_GASP,pan(int(x)));
}

esPatraBS::esPatraBS(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  cs=csBOSS;
  item_set=0;
  misc=clk;
  clk = -((misc*21)>>1)-1;
  yofs=56;
  hyofs=2;
  hxsz=hysz=16;
  hxofs=1000;
  mainguy=count_enemy=false;
}

bool esPatraBS::animate(int index)
{
  if(dying)
    return Dead();
  hxofs=4;
  return enemy::animate(index);
}

int esPatraBS::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wPhantom:
      return 0;
    case wArrow:
    case wMagic:
    case wRefMagic:
    case wHookshot:
    case wBrang:
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
    case wWand:
    case wBeam:
    case wHammer:
    case wSword:
      hp-=power;
      hclk=33;
      sfx(WAV_EHIT,pan(int(x)));
      return 1;
  }
  return 0;
}

void esPatraBS::draw(BITMAP *dest)
{
  tile=dummy_int[0];
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)                                            //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
        flip=0;
        break;
      case down:                                            //d
        flip=0; tile+=4;
        break;
      case left:                                            //l
        flip=0; tile+=8;
        break;
      case right:                                           //r
        flip=0; tile+=12;
        break;
      case l_up:                                            //ul
        flip=0; tile+=20;
        break;
      case r_up:                                            //ur
        flip=0; tile+=24;
        break;
      case l_down:                                          //dl
        flip=0; tile+=28;
        break;
      case r_down:                                          //dr
        flip=0; tile+=32;
        break;
    }
    tile += ((clk&6)>>1);
  }
  else
  {
    tile += (clk&4)?1:0;
  }
  if(clk>=0)
    enemy::draw(dest);
}

/**********************************/
/**********  Misc Code  ***********/
/**********************************/

void addEwpn(int x,int y,int id,int type,int power,int dir)
{
  Ewpns.add(new weapon((fix)x,(fix)y,id,type,power,dir));
}

int enemy_dp(int index)
{
  return (((enemy*)guys.spr(index))->d->dp)<<2;
}

int ewpn_dp(int index)
{
  return (((weapon*)Ewpns.spr(index))->power)<<2;
}

int lwpn_dp(int index)
{
  return (((weapon*)Lwpns.spr(index))->power)<<2;
}

int hit_enemy(int index,int wpnId,int power,int wpnx,int wpny,int dir)
{
  return ((enemy*)guys.spr(index))->takehit(wpnId,power,wpnx,wpny,dir);
}

void enemy_scored(int index)
{
  ((enemy*)guys.spr(index))->scored=true;
}

void addguy(int x,int y,int id,int clk,bool mainguy)
{
  guys.add(new guy((fix)x,(fix)(y+(isdungeon()?1:0)),id,clk,mainguy));
}

void additem(int x,int y,int id,int pickup)
{
  items.add(new item((fix)x,(fix)y,id,pickup,0));
}

void additem(int x,int y,int id,int pickup,int clk)
{
  items.add(new item((fix)x,(fix)y,id,pickup,clk));
}

void kill_em_all()
{
  for(int i=0; i<guys.Count(); i++)
    ((enemy*)guys.spr(i))->kickbucket();
}

// For Link's hit detection. Don't count them if they are stunned or are guys.
int GuyHit(int tx,int ty,int txsz,int tysz)
{
  for(int i=0; i<guys.Count(); i++)
  {
    if(guys.spr(i)->hit(tx,ty,txsz,tysz))
      if( ((enemy*)guys.spr(i))->stunclk==0 &&
      ( ((enemy*)guys.spr(i))->d->family != eeGUY || ((enemy*)guys.spr(i))->d->misc1 ) )
    {
      return i;
    }
  }
  return -1;
}

// For Link's hit detection. Count them if they are dying.
int GuyHit(int index,int tx,int ty,int txsz,int tysz)
{
  enemy *e = (enemy*)guys.spr(index);
  if(e->hp > 0)
    return -1;

  bool d = e->dying;
  int hc = e->hclk;
  e->dying = false;
  e->hclk = 0;
  bool hit = e->hit(tx,ty,txsz,tysz);
  e->dying = d;
  e->hclk = hc;

  return hit ? index : -1;
}

bool hasMainGuy()

{
  for(int i=0; i<guys.Count(); i++)
    if(((enemy*)guys.spr(i))->mainguy)
      return true;
  return false;
}

void EatLink(int index)
{
  ((eLikeLike*)guys.spr(index))->eatlink();
}

void GrabLink(int index)
{
  ((eWallM*)guys.spr(index))->grablink();
}

bool CarryLink()
{
  for(int i=0; i<guys.Count(); i++)
    if(guys.spr(i)->id==eWALLM)
      if(((eWallM*)guys.spr(i))->haslink)
      {
        Link.x=guys.spr(i)->x;
        Link.y=guys.spr(i)->y;
        return ((eWallM*)guys.spr(i))->misc > 0;
      }
  return false;
}

void movefairy(fix &x,fix &y,int misc)
{
  int i = guys.idFirst(eITEMFAIRY+0x1000+misc);
  x = guys.spr(i)->x;
  y = guys.spr(i)->y;
}

void killfairy(int misc)
{
  int i = guys.idFirst(eITEMFAIRY+0x1000+misc);
  guys.del(i);
}

void addenemy(int x,int y,int id,int clk)
{
  sprite *e;
  switch(id)
  {
    case eROCTO1:
    case eROCTO2:
    case eBOCTO1:
    case eBOCTO2:
    case eCOCTO:     e = new eOctorok((fix)x,(fix)y,id,clk); break;
    case eRMOBLIN:
    case eBMOBLIN:  e = new eMoblin((fix)x,(fix)y,id,clk); break;
    case eRLYNEL:

    case eBLYNEL:    e = new eLynel((fix)x,(fix)y,id,clk); break;
    // case eBLYNEL:    e = new eShooter((fix)x,(fix)y,id,clk); break;
    case eRLEV:
    case eBLEV:      e = new eLeever((fix)x,(fix)y,id,clk); break;
    case eRTEK:
    case eBTEK:      e = new eTektite((fix)x,(fix)y,id,clk); break;
    case ePEAHAT:    e = new ePeahat((fix)x,(fix)y,id,clk); break;
    case eZORA:      e = new eZora((fix)x,(fix)y,id,clk); break;
    case eROCK:      e = new eRock((fix)x,(fix)y,id,clk); break;
    case eARMOS:     e = new eArmos((fix)x,(fix)y,id,clk); break;
    case eGHINI1:
    case eGHINI2:    e = new eGhini((fix)x,(fix)y,id,clk); break;
    case eBAT:
    case eKEESE1:
    case eKEESE2:
    case eKEESE3:    e = new eKeese((fix)x,(fix)y,id,clk); break;
    case eTRAP:
    case eTRAP_H:
    case eTRAP_V:    e = new eTrap((fix)x,(fix)y,id,clk); break;
    case eSTALFOS:

    case eSTALFOS2:
    case eSTALFOS3:   e = new eStalfos((fix)x,(fix)y,id,clk); break;
    // case eSTALFOS2:  e = new eShooter((fix)x,(fix)y,id,clk); break;
    case eGEL:       e = new eGel((fix)x,(fix)y,id,clk); break;
    case eZOL:       e = new eZol((fix)x,(fix)y,id,clk); break;
    case eROPE:
    case eROPE2:     e = new eRope((fix)x,(fix)y,id,clk); break;
    case eVIRE:      e = new eVire((fix)x,(fix)y,id,clk); break;
    case eIBUBBLE:
    case eBIBUBBLE:
    case eRIBUBBLE:
    case eBBUBBLE:
    case eRBUBBLE:
    case eBUBBLE:    e = new eBubble((fix)x,(fix)y,id,clk); break;
    case eLIKE:      e = new eLikeLike((fix)x,(fix)y,id,clk); break;
    case eGIBDO:     e = new eGibdo((fix)x,(fix)y,id,clk); break;
    case ePOLSV:     e = new ePolsVoice((fix)x,(fix)y,id,clk); break;
    case eRGORIYA:
    case eBGORIYA:   e = new eGoriya((fix)x,(fix)y,id,clk); break;
    case eSDKNUT:
    case eDKNIGHT:
    case eRDKNUT:
    case eBDKNUT:    e = new eDarknut((fix)x,(fix)y,id,clk); break;
    case eMWIZ:
    case eBATROBE:
    case eFWIZ:
    case eWWIZ:
    case eRWIZ:
    case eBWIZ:      e = new eWizzrobe((fix)x,(fix)y,id,clk); break;
    case eFBALL:     e = new eFBall((fix)x,(fix)y,id,clk); break;
    case eWALLM:     e = new eWallM((fix)x,(fix)y,id,clk); break;
    case eDODONGO:   e = new eDodongo((fix)x,(fix)y,id,clk); break;
    case eDODONGOBS:
    case eDODONGOF:  e = new eDodongo2((fix)x,(fix)y,id,clk); break;
    case eLAQUAM:
    case eRAQUAM:    e = new eAquamentus((fix)x,(fix)y,id,clk); break;
    case eMOLDORM:   e = new eMoldorm((fix)x,(fix)y,id,5); break;
    case eMANHAN:
    case eMANHAN2:   e = new eManhandla((fix)x,(fix)y,id,clk); break;
    case eGLEEOK1F:
    case eGLEEOK1:   e = new eGleeok((fix)x,(fix)y,id,1); break;
    case eGLEEOK2F:
    case eGLEEOK2:   e = new eGleeok((fix)x,(fix)y,id,2); break;
    case eGLEEOK3F:
    case eGLEEOK3:   e = new eGleeok((fix)x,(fix)y,id,3); break;
    case eGLEEOK4F:
    case eGLEEOK4:   e = new eGleeok((fix)x,(fix)y,id,4); break;
    case eDIGPUP1:
    case eDIGPUP2:
    case eDIGPUP3:
    case eDIGPUP4:   e = new eLilDig((fix)x,(fix)y,id,clk); break;
    case eDIG1:
    case eDIG3:      e = new eBigDig((fix)x,(fix)y,id,clk); break;
    case eRGOHMA:
    case eBGOHMA:
    case eGOHMA3:
    case eGOHMA4:    e = new eGohma((fix)x,(fix)y,id,clk); break;
    case eRCENT:
    case eBCENT:     e = new eLanmola((fix)x,(fix)y,id,5); break;
    case ePATRABS:   e = new ePatraBS((fix)x,(fix)y,id,clk); break;
    case ePATRAL2:
    case ePATRAL3:
    case ePATRA1:
    case ePATRA2:    e = new ePatra((fix)x,(fix)y,id,clk); break;
    case eGANON:     e = new eGanon((fix)x,(fix)y,id,clk); break;
    case eITEMFAIRY: e = new eItemFairy((fix)x,(fix)y,id+clk,0); break;
    case eFIRE:      e = new eFire((fix)x,(fix)y,id,clk); break;
    case eKEESETRIB: e = new eKeeseTrib((fix)x,(fix)y,id,clk); break;
    case eVIRETRIB:  e = new eVireTrib((fix)x,(fix)y,id,clk); break;
    case eGELTRIB:   e = new eGelTrib((fix)x,(fix)y,id,clk); break;
    case eZOLTRIB:   e = new eZolTrib((fix)x,(fix)y,id,clk); break;
    case eTRAP_LR:
    case eTRAP_UD:   e = new eTrap2((fix)x,(fix)y,id,clk); break;
    case eTRIGGER:   e = new eTrigger((fix)x,(fix)y,id,clk); break;
    case eNPCSTAND1:
    case eNPCSTAND2:
    case eNPCSTAND3:
    case eNPCSTAND4:
    case eNPCSTAND5:
    case eNPCWALK1:
    case eNPCWALK2:
    case eNPCWALK3:
    case eNPCWALK4:
    case eNPCWALK5:  e = new eNPC((fix)x,(fix)y,id,clk); break;
    default:         e = new enemy((fix)x,(fix)y,-1,clk); break;
  }
  guys.add(e);

  // add segments of segmented enemies
  int c=0;
  switch(id)
  {

    case eMOLDORM:
      for(int i=0; i<5; i++)
        guys.add(new esMoldorm((fix)x,(fix)y,id+0x1000,-(i<<4)));
      break;

    case eRCENT:

    case eBCENT:
    {
      int shft = (id==eRCENT)?3:2;
      guys.add(new esLanmola((fix)x,(fix)y,id+0x1000,0));
      for(int i=1; i<5; i++)
        guys.add(new esLanmola((fix)x,(fix)y,id+0x2000,-(i<<shft)));
    } break;

    case eMANHAN:
    case eMANHAN2:
      for(int i=0; i<((id==eMANHAN)?4:8); i++)
        guys.add(new esManhandla((fix)x,(fix)y,id+0x1000,i));
      break;

    case eGLEEOK4F:
    case eGLEEOK4: guys.add(new esGleeok((fix)x,(fix)y,id+0x1000,c)); c-=48;
    case eGLEEOK3F:
    case eGLEEOK3: guys.add(new esGleeok((fix)x,(fix)y,id+0x1000,c)); c-=48;
    case eGLEEOK2F:
    case eGLEEOK2: guys.add(new esGleeok((fix)x,(fix)y,id+0x1000,c)); c-=48;
    case eGLEEOK1F:
    case eGLEEOK1: guys.add(new esGleeok((fix)x,(fix)y,id+0x1000,c)); c-=48;
    break;


    case ePATRAL2:
    case ePATRAL3:
      for(int i=0; i<8; i++)
      {
        guys.add(new esPatra((fix)x,(fix)y,id+0x1000,i));
      }
    case ePATRA1:
    case ePATRA2:
      for(int i=0; i<8; i++)
      {

        guys.add(new esPatra((fix)x,(fix)y,id+0x1000,i));
      }
      break;
    case ePATRABS:
      for(int i=0; i<6; i++)
      {
        guys.add(new esPatraBS((fix)x,(fix)y,id+0x1000,i));
      }
      break;
  }
}

bool checkpos(int id)
{
  switch(id)
  {
    case eRTEK:
    case eBTEK: return false;
  }
  return true;
}

bool isjumper(int id)
{
  switch(id)
  {
    case eRTEK:
    case eBTEK: return true;
  }
  return false;
}

void addfires()
{
  if (!get_bit(quest_rules,qr_NOGUYFIRES))
  {
    int bs = get_bit(quest_rules,qr_BSZELDA);
    addguy(bs? 64: 72,64,gFIRE,-17,false);
    addguy(bs?176:168,64,gFIRE,-18,false);
  }
}

void loadguys()
{
  if(loaded_guys)
    return;

  loaded_guys=true;

  byte Guy=0,Item=0;
  repaircharge=false;
  adjustmagic=false;
  learnslash=false;
  itemindex=-1;
  hasitem=0;

  if(currscr>=128)
  {
    if (!isdungeon())
    {
      Guy=tmpscr[1].guy;
    }
  }
  else
  {
    Guy=tmpscr->guy;
    if(Guy==gFAIRY)
    {
      sfx(WAV_SCALE);
      addguy(120,62,gFAIRY,-14,false);
      game.maps[(currmap<<7)+currscr] |= mVISITED;          // mark as visited
      return;
    }
    if(dlevel==0)
    {
      game.maps[(currmap<<7)+currscr] |= mVISITED;          // mark as visited
      Guy=0;
    }
  }

  if(tmpscr->room==rZELDA)
  {
    addguy(120,72,Guy,-15,true);
    guys.spr(0)->hxofs=1000;
    addenemy(128,96,eFIRE,-15);
    addenemy(112,96,eFIRE,-15);
    addenemy(96,120,eFIRE,-15);
    addenemy(144,120,eFIRE,-15);
    return;
  }

  if(Guy)
  {
    addfires();
    if(currscr>=128)
      if(getmapflag())
        Guy=0;
    switch(tmpscr->room)
    {
      case rSP_ITEM:
      case rGRUMBLE:
      case rBOMBS:
      case rSWINDLE:
      case rMUPGRADE:
      case rLEARNSLASH:
        if(getmapflag())
          Guy=0;
        break;
      case rTRIFORCE:
      {
        int tc = TriforceCount();
        if(get_bit(quest_rules,qr_4TRI))
        {
          if((get_bit(quest_rules,qr_3TRI) && tc>=3) || tc>=4)
            Guy=0;
        }
        else
        {
          if((get_bit(quest_rules,qr_3TRI) && tc>=6) || tc>=8)
            Guy=0;
        }
      }
      break;
    }
    if(Guy)
    {
      blockpath=true;

      if(currscr<128)
        sfx(WAV_SCALE);
      addguy(120,64,Guy, (dlevel||BSZ)?-15:startguy[rand()&7], true);
      Link.Freeze();
    }
  }

  if(currscr<128)
  {
    Item=tmpscr->item;
    if(getmapflag())
      Item=0;
    if(Item)
    {
      if(tmpscr->flags&fITEM)
        hasitem=1;
      else if(tmpscr->enemyflags&efCARRYITEM)
        hasitem=2;
      else
        additem(tmpscr->itemx,tmpscr->itemy+1,Item, ipONETIME | ipBIGRANGE
          | ((Item==iTriforce) ? ipHOLDUP : 0)
        | ((tmpscr->flags3&fHOLDITEM) ? ipHOLDUP : 0)
        );
    }
  }
  else if(dlevel)
  {
    Item=tmpscr[1].catchall;
    if(getmapflag())
      Item=0;
    if(Item)
      additem(tmpscr->itemx,tmpscr->itemy+1,Item, ipBIGRANGE | ipONETIME | ipHOLDUP);
  }

  if(tmpscr->room==r10RUPIES && !getmapflag())
  {
    setmapflag();
    for(int i=0; i<10; i++)
      additem(ten_rupies_x[i],ten_rupies_y[i],0,ipBIGRANGE,-14);
  }
}

void never_return(int index)
{
  if(!get_bit(quest_rules,qr_KILLALL))
    goto doit;

  for(int i=0; i<guys.Count(); i++)
    if(((((enemy*)guys.spr(i))->d->flags)&guy_neverret) && i!=index)
      goto dontdoit;

  doit:
  setmapflag(mNEVERRET);

  dontdoit:

  return;
}

bool hasBoss()
{
  for(int i=0; i<guys.Count(); i++)
  {
    switch(guys.spr(i)->id)
    {
      case eLAQUAM:
      case eRAQUAM:
      case eGLEEOK1:
      case eGLEEOK2:
      case eGLEEOK3:
      case eGLEEOK4:
      case eGLEEOK1F:
      case eGLEEOK2F:
      case eGLEEOK3F:
      case eGLEEOK4F:
      case eRGOHMA:
      case eBGOHMA:
      case eGOHMA3:
      case eGOHMA4:
      case eMOLDORM:
      case eMANHAN:
      case eMANHAN2:
      case eRCENT:
      case eBCENT:
      case ePATRA1:
      case ePATRA2:
      case ePATRAL2:
      case ePATRAL3:
      case ePATRABS:
      case eDIG1:
      case eDIG3:
        return true;
    }
  }
  return false;
}

bool slowguy(int id)
{
  switch(id)
  {
    case eROCTO1:
    case eBOCTO1:
    case eROCTO2:
    case eBOCTO2:
    case eRLEV:
    case eBLEV:
    case eROCK:
      return true;
  }
  return false;
}

bool countguy(int id)
{
  switch(id)
  {
    case eFBALL:
    case eROCK:

    case eZORA:
    case eTRAP:
      return false;
    case eBUBBLE:
    case eRBUBBLE:
    case eBBUBBLE:
      return dlevel;
  }
  return true;
}

bool ok2add(int id)
{
  switch(id)
  {
    /*
      case eARMOS:
      case eGHINI2:
      case eTRAP:
      case eGANON:
        return false;
    */
    //  case eGHINI2:
    case eGANON:
    case eTRAP:
      return false;

      /*
        case eGLEEOK2:
        case eGLEEOK3:
        case eGLEEOK4:
        case eMOLDORM:
        case eMANHAN:
        case eMANHAN2:
        case eLAQUAM:
        case eRAQUAM:
        case eRGOHMA:
        case eBGOHMA:
      case eDIG1:
      case eDIG3:
      case ePATRA1:
      case ePATRA2:
      */
    case eLAQUAM:
    case eRAQUAM:
    case eGLEEOK1:
    case eGLEEOK2:
    case eGLEEOK3:
    case eGLEEOK4:
    case eGLEEOK1F:
    case eGLEEOK2F:
    case eGLEEOK3F:
    case eGLEEOK4F:
    case eRGOHMA:
    case eBGOHMA:
    case eMOLDORM:
    case eMANHAN:
    case eMANHAN2:
    case eRCENT:
    case eBCENT:
    case eDIG1:
    case eDIG3:
    case ePATRA1:
    case ePATRA2:
    case ePATRAL2:
    case ePATRAL3:
    case ePATRABS:
      return !getmapflag(mNEVERRET);
  }

  if(!get_bit(quest_rules,qr_NOTMPNORET))
    return !getmapflag(mTMPNORET);
  return true;
}

void load_default_enemies()
{
  wallm_load_clk=frame-80;

  if(tmpscr->enemyflags&efZORA)
    addenemy(-16,-16,eZORA,0);

  if(tmpscr->enemyflags&efTRAP4)
  {
    addenemy(32,32,eTRAP,-14);
    addenemy(208,32,eTRAP,-14);
    addenemy(32,128,eTRAP,-14);
    addenemy(208,128,eTRAP,-14);
  }

  for(int y=0; y<176; y+=16)
  {
    for(int x=0; x<256; x+=16)
    {
      int ctype = combobuf[MAPDATA(x,y)].type;
      int cflag = MAPFLAG(x, y);
      if((ctype==cTRAP_H)||(cflag==mfTRAP_H))
        addenemy(x,y,eTRAP_H,-14);
      if((ctype==cTRAP_V)||(cflag==mfTRAP_V))
        addenemy(x,y,eTRAP_V,-14);
      if((ctype==cTRAP_4)||(cflag==mfTRAP_4))
      {
        addenemy(x,y,eTRAP,-14);
        guys.spr(guys.Count()-1)->dummy_int[1]=2;
      }
      if((ctype==cTRAP_LR)||(cflag==mfTRAP_LR))
        addenemy(x,y,eTRAP_LR,-14);
      if((ctype==cTRAP_UD)||(cflag==mfTRAP_UD))
        addenemy(x,y,eTRAP_UD,-14);

    }
  }
  if(tmpscr->enemyflags&efTRAP2)
  {
    addenemy(64,80,eTRAP,-14);
    guys.spr(guys.Count()-1)->dummy_int[1]=1;
    addenemy(176,80,eTRAP,-14);
    guys.spr(guys.Count()-1)->dummy_int[1]=1;
  }

  if(tmpscr->enemyflags&efROCKS)
  {
    addenemy(0,0,eROCK,-14);
    addenemy(0,0,eROCK,-14);
    addenemy(0,0,eROCK,-14);
  }

  if(tmpscr->enemyflags&efFIREBALLS)
    for(int y=0; y<176; y+=16)
      for(int x=0; x<256; x+=16)
      {
        int ctype = combobuf[MAPDATA(x,y)].type;
        if(ctype==cL_STATUE)
          addenemy(x+4,y+7,eFBALL,24);
        if(ctype==cR_STATUE)
          addenemy(x-8,y-1,eFBALL,24);
        if(ctype==cC_STATUE)
          addenemy(x,y,eFBALL,24);
      }
}

void nsp()
// moves sle_x and sle_y to the next position
{
  if(sle_x==0)
  {
    if(sle_y<160)
      sle_y+=16;
    else
      sle_x+=16;
  }
  else if(sle_y==160)
  {
    if(sle_x<240)
      sle_x+=16;
    else
      sle_y-=16;
  }
  else if(sle_x==240)
  {
    if(sle_y>0)
      sle_y-=16;
    else
      sle_x-=16;
  }
  else if(sle_y==0)
  {
    if(sle_x>0)
      sle_x-=16;
    else
      sle_y+=16;
  }
}

int next_side_pos()
// moves sle_x and sle_y to the next available position
// returns the direction the enemy should face
{
  bool blocked;
  int c=0;
  do
  {
    nsp();
    blocked = _walkflag(sle_x,sle_y,2) || _walkflag(sle_x,sle_y+8,2);
    if(++c>50)
      return -1;
  } while(blocked);

  int dir=0;
  if(sle_x==0)    dir=right;
  if(sle_y==0)    dir=down;
  if(sle_x==240)  dir=left;
  if(sle_y==168)  dir=up;
  return dir;
}

bool can_side_load(int id)
{
  switch (id&0x0FFF)
  {
    case eRTEK:
    case eBTEK:
    case eRLEV:
    case eBLEV:
    case eRAQUAM:
    case eDODONGO:
    case eMANHAN:
    case eGLEEOK1:
    case eGLEEOK2:
    case eGLEEOK3:
    case eGLEEOK4:
    case eDIG1:
    case eDIG3:
    case eRGOHMA:
    case eBGOHMA:
    case eRCENT:
    case eBCENT:
    case ePATRA1:
    case ePATRA2:
    case eGANON:
    case eLAQUAM:
    case eMANHAN2:
    case eCEILINGM:
    case eFLOORM:
    case ePATRABS:
    case ePATRAL2:
    case ePATRAL3:
    case eGLEEOK1F:
    case eGLEEOK2F:
    case eGLEEOK3F:
    case eGLEEOK4F:
    case eDODONGOBS:
    case eDODONGOF:
    case eGOHMA3:
    case eGOHMA4:
      return false;
      break;
  }
  return true;
}


void side_load_enemies()
{
  if(sle_clk==0)
  {
    sle_cnt = 0;
    int guycnt = 0;
    short s = (currmap<<7)+currscr;
    bool beenhere=false;
    bool reload=true;

    load_default_enemies();

    for(int i=0; i<6; i++)
      if(visited[i]==s)
        beenhere=true;

    if(!beenhere)
    {
      visited[vhead]=s;
      vhead = (vhead+1)%6;
    }
    else if(game.guys[s]==0)
    {
      sle_cnt=0;
      reload=false;
    }

    if(reload)
    {
      sle_cnt = game.guys[s];
      if(sle_cnt==0)
      {
        while(sle_cnt<10 && tmpscr->enemy[sle_cnt]!=0)
          ++sle_cnt;
      }
    }

    if((get_bit(quest_rules,qr_ALWAYSRET)) || (tmpscr->flags3&fENEMIESRETURN))
    {
      sle_cnt = 0;
      while(sle_cnt<10 && tmpscr->enemy[sle_cnt]!=0)
        ++sle_cnt;
    }

    for(int i=0; !countguy(tmpscr->enemy[i]) && sle_cnt<10; i++)
      ++sle_cnt;

    for(int i=0; i<sle_cnt; i++)
      if(countguy(tmpscr->enemy[i]))
        ++guycnt;

    game.guys[s] = guycnt;
  }

  if( (++sle_clk+8)%24 == 0)
  {
    int dir = next_side_pos();
    if(dir==-1 || tooclose(sle_x,sle_y,32))
    {
      return;
    }
    int enemy_slot=guys.Count();
    addenemy(sle_x,sle_y,tmpscr->enemy[--sle_cnt],0);
    guys.spr(enemy_slot)->dir = dir;
  }

  if(sle_cnt<=0)
    loaded_enemies=true;
}

void loadenemies()
{
  if(loaded_enemies)
    return;

  if(tmpscr->pattern==pSIDES)                               // enemies enter from sides
  {
    side_load_enemies();
    return;
  }

  loaded_enemies=true;

  // do enemies that are always loaded
  load_default_enemies();

  // dungeon basements

  static byte dngn_enemy_x[4] = {32,96,144,208};

  if(currscr>=128)
  {
    if(dlevel==0) return;
    for(int i=0; i<4; i++)
      addenemy(dngn_enemy_x[i],96,tmpscr->enemy[i]?tmpscr->enemy[i]:eKEESE1,-14-i);
    return;
  }

  // check if it's been long enough to reload all enemies

  int loadcnt = 10;
  short s = (currmap<<7)+currscr;
  bool beenhere = false;
  bool reload = true;

  for(int i=0; i<6; i++)
    if(visited[i]==s)
      beenhere = true;

  if(!beenhere)
  {
    visited[vhead]=s;
    vhead = (vhead+1)%6;
  }
  else if(game.guys[s]==0)
  {
    loadcnt = 0;
    reload  = false;
  }

  if(reload)
  {
    loadcnt = game.guys[s];
    if(loadcnt==0)
      loadcnt = 10;
  }

  if(get_bit(quest_rules,qr_ALWAYSRET))
    loadcnt = 10;

  for(int i=0; !countguy(tmpscr->enemy[i]) && loadcnt<10; i++)
    ++loadcnt;

  // check if it's the dungeon boss and it has been beaten before
  if(tmpscr->enemyflags&efBOSS && game.lvlitems[dlevel]&liBOSS)
    return;

  // load enemies

  if(tmpscr->pattern==pRANDOM)                              // enemies appear at random places
  {
    //int set=loadside*9;
    int pos=rand()%9;
    int clk=-15,x,y,fastguys=0;
    int i=0,guycnt=0;

    for( ; i<loadcnt && tmpscr->enemy[i]>0; i++)            /* i=0 */
    {
      bool placed=false;
      int t=0;
      for(int sy=0; sy<176; sy+=16)
      {
        for(int sx=0; sx<256; sx+=16)
        {
          int cflag = MAPFLAG(sx, sy);
          if((cflag==mfENEMY0+i) && (!placed))
          {
            if(!ok2add(tmpscr->enemy[i]))
              ++loadcnt;
            else
            {
              addenemy(sx,sy,tmpscr->enemy[i],-15);
              if(countguy(tmpscr->enemy[i]))
                ++guycnt;

              placed=true;
              goto placed_enemy;
            }
          }
        }
      }
      do
      {
        if(pos>=9)
          pos-=9;
        x=stx[loadside][pos];
        y=sty[loadside][pos];
        ++pos;
        ++t;
      } while(
        t<20 &&
        (
          (
            (
              isflier(tmpscr->enemy[i])&&
              (
                (
                  COMBOTYPE(x+8,y+8)==cNOFLYZONE
                )||
                (
                  _walkflag(x,y+8,2)&&
                  !get_bit(quest_rules,qr_WALLFLIERS)
                )
              )
            ) ||
            (
              isjumper(tmpscr->enemy[i])&&
              COMBOTYPE(x+8,y+8)==cNOJUMPZONE
            ) ||
            (
              (
                !isflier(tmpscr->enemy[i])&&
                !isjumper(tmpscr->enemy[i])&&
                (
                  _walkflag(x,y+8,2)||
                  COMBOTYPE(x+8,y+8)==cPIT
                )
              )&&
              tmpscr->enemy[i]!=eZORA
            )
          ) ||
          (
            tooclose(x,y,40) &&
            t<11
          )
        )
      );

      if(t < 20)
      {
        int c=0;
        c=clk;
        if(!slowguy(tmpscr->enemy[i]))
          ++fastguys;
        else if(fastguys>0)
          c=-15*(i-fastguys+2);
        else
          c=-15*(i+1);

        if(BSZ)
          c=-15;

        if(!ok2add(tmpscr->enemy[i]))
          ++loadcnt;
        else
        {
          addenemy(x,y,tmpscr->enemy[i],c);
          if(countguy(tmpscr->enemy[i]))
            ++guycnt;
        }

        if(i==0 && tmpscr->enemyflags&efLEADER)
        {
          int index = guys.idFirst(tmpscr->enemy[i],0xFFF);
          if (index!=-1)
          {
            ((enemy*)guys.spr(index))->leader = true;
          }
        }
        if(i==0 && hasitem==2)
        {
          int index = guys.idFirst(tmpscr->enemy[i],0xFFF);
          if (index!=-1)
          {
            ((enemy*)guys.spr(index))->itemguy = true;
          }
        }
      }                                                     // if(t < 20)
      placed_enemy:
      if (placed)
      {
        if(i==0 && tmpscr->enemyflags&efLEADER)
        {
          int index = guys.idFirst(tmpscr->enemy[i],0xFFF);
          if (index!=-1)
          {
            ((enemy*)guys.spr(index))->leader = true;
          }
        }
        if(i==0 && hasitem==2)
        {
          int index = guys.idFirst(tmpscr->enemy[i],0xFFF);
          if (index!=-1)
          {

            ((enemy*)guys.spr(index))->itemguy = true;
          }
        }
      }
      --clk;
    }                                                       // for
    game.guys[s] = guycnt;
  }
}

void moneysign()
{
  additem(48,108,iRupy,ipDUMMY);
  //  textout(scrollbuf,zfont,"X",64,112,CSET(0)+1);
  set_clip_state(pricesdisplaybuf, 0);
  textout_ex(pricesdisplaybuf,zfont,"X",64,112,CSET(0)+1,-1);
}

void putprices(bool sign)
{
  // refresh what's under the prices
  // for(int i=5; i<12; i++)
  //   putcombo(scrollbuf,i<<4,112,tmpscr->data[112+i],tmpscr->cpage);

  rectfill(pricesdisplaybuf, 72, 112, pricesdisplaybuf->w-1, pricesdisplaybuf->h-1, 0);  int step=32;
  int x=80;
  if(prices[2][0]==0&&prices[2][1]==0)
  {
    step<<=1;
    if(prices[1][0]==0&&prices[1][1]==0)
    {
      x=112;
    }
  }
  for(int i=0; i<3; i++)
  {
    if(prices[i][0])
    {
      char buf[8];
      sprintf(buf,sign?"%+3d":"%3d",prices[i][0]);

      int l=strlen(buf);
      set_clip_state(pricesdisplaybuf, 0);
      textout_ex(pricesdisplaybuf,zfont,buf,x-(l>3?(l-3)<<3:0),112,CSET(0)+1,-1);
    }
    x+=step;
  }
}

void setupscreen()
{
  boughtsomething=false;
  int t=currscr<128?0:1;
  word str=tmpscr[t].str;

  for(int i=0; i<3; i++)
  {
    prices[i][0]=0;
    prices[i][1]=0;
  }

  switch(tmpscr[t].room)
  {
    case rSP_ITEM:                                          // special item
      additem(120,89,tmpscr[t].catchall,ipONETIME+ipHOLDUP+ipCHECK);
      break;

    case rINFO:                                             // pay for info
    {
      int count = 0;
      int base  = 88;
      int step  = 5;

      moneysign();
      for(int i=0; i<3; i++)
      {
        if(QMisc.info[tmpscr[t].catchall].str[i])
        {
          ++count;
        }
        else
          break;
      }
      if(count)
      {
        if(count==1)
        {
          base = 88+32;
        }
        if(count==2)
        {
          step = 6;
        }

        for(int i=0; i < count; i++)
        {
          additem((i << step)+base, 89, iRupy, ipMONEY + ipDUMMY);
          prices[i][0] = -(QMisc.info[tmpscr[t].catchall].price[i]);
        }
      }
      break;
    }

    case rMONEY:                                            // secret money
      additem(120,89,iRupy,ipONETIME+ipDUMMY+ipMONEY);
      break;

    case rGAMBLE:                                           // gambling
      prices[0][0]=prices[1][0]=prices[2][0]=-10;
      moneysign();
      additem(88,89,iRupy,ipMONEY+ipDUMMY);
      additem(120,89,iRupy,ipMONEY+ipDUMMY);
      additem(152,89,iRupy,ipMONEY+ipDUMMY);
      break;

    case rREPAIR:                                           // door repair
      //  if (!get_bit(quest_rules,qr_REPAIRFIX)) {
      setmapflag();
      //  }
      repaircharge=true;
      break;

    case rMUPGRADE:                                         // upgrade magic
      adjustmagic=true;
      break;

    case rLEARNSLASH:                                       // learn slash attack
      learnslash=true;
      break;

    case rRP_HC:                                            // heart container or red potion
      additem(88,89,iRPotion,ipONETIME+ipHOLDUP+ipFADE);
      additem(152,89,iHeartC,ipONETIME+ipHOLDUP+ipFADE);
      break;

    case rP_SHOP:                                           // potion shop
      if(!(game.items[itype_letter]&i_letter_used))
      {
        str=0;
        break;
      }
      // fall through

    case rSHOP:                                             // shop
    {
      int count = 0;
      int base  = 88;
      int step  = 5;

      moneysign();
      //count and align the stuff
      for(int i=0; i<3; ++i)
      {
        if (QMisc.shop[tmpscr[t].catchall].item[count])
        {
          prices[count][0] = QMisc.shop[tmpscr[t].catchall].price[count];
          prices[count][1] = QMisc.shop[tmpscr[t].catchall].item[count];
          ++count;
        }
        else
        {
          QMisc.shop[tmpscr[t].catchall].item[count]=QMisc.shop[tmpscr[t].catchall].item[i+1];
          QMisc.shop[tmpscr[t].catchall].price[count]=QMisc.shop[tmpscr[t].catchall].price[i+1];
          QMisc.shop[tmpscr[t].catchall].item[i+1]=0;
          QMisc.shop[tmpscr[t].catchall].price[i+1]=0;
        }
      }

      if(count==1)
      {
        base = 88+32;
      }
      if(count==2)
      {
        step = 6;
      }

      for(int i=0; i<count; i++)
      {
        additem((i<<step)+base, 89+(QMisc.shop[tmpscr[t].catchall].item[i]?0:0), QMisc.shop[tmpscr[t].catchall].item[i], ipHOLDUP+ipCHECK+ipFADE);
      }
    }
    break;

    case rBOMBS:                                            // more bombs
      additem(120,89,iRupy,ipDUMMY+ipMONEY);
      prices[0][0]=-tmpscr[t].catchall;
      break;

    case rSWINDLE:                                          // leave heart container or money
      additem(88,89,iHeartC,ipDUMMY+ipMONEY);
      additem(152,89,iRupy,ipDUMMY+ipMONEY);
      prices[0][0]=-1;
      prices[1][0]=-tmpscr[t].catchall;
      break;
  }

  putprices(false);

  if(str)
  {
    msgstr=str;
    msgclk=msgpos=0;
  }
  else

  {
    Link.unfreeze();
  }
}

void putmsg()
{
  if (linkedmsgclk>0)
  {
    if (linkedmsgclk==1)
    {
      if (cAbtn()||cBbtn())
      {
        linkedmsgclk=0;
        msgpos=0; msgstr=MsgStrings[msgstr].nextstring;
        clear_bitmap(msgdisplaybuf);
        set_clip_state(msgdisplaybuf, 1);
        putprices(false);
      }
    }
    else
    {
      --linkedmsgclk;
    }
  }
  if(!msgstr || msgpos>=72)
    return;

  if(((msgclk++)%6<5)&&((!cAbtn()&&!cBbtn())||(!get_bit(quest_rules,qr_ALLOWFASTMSG))))
    return;

  if(msgpos == 0)
  {
    while(MsgStrings[msgstr].s[msgpos]==' ')
      ++msgpos;
  }

  sfx(WAV_MSG);

  //using the clip value to indicate the bitmap is "dirty"
  //rather than add yet another global variable
  set_clip_state(msgdisplaybuf, 0);
  textprintf_ex(msgdisplaybuf,zfont,((msgpos%24)<<3)+32,((msgpos/24)<<3)+40,CSET(0)+1,-1,
    "%c",MsgStrings[msgstr].s[msgpos]);

  ++msgpos;

  if(MsgStrings[msgstr].s[msgpos]==' ' && MsgStrings[msgstr].s[msgpos+1]==' ')
    while(MsgStrings[msgstr].s[msgpos]==' ')
      ++msgpos;

  if(msgpos>=72)
  {
    if (MsgStrings[msgstr].nextstring!=0)
    {
      linkedmsgclk=51;
    }
    else
    {
      //these are to cancel out any keys that Link may
      //be pressing so he doesn't attack at the end of
      //a message if he was scrolling through it quickly.
      rAbtn();
      rBbtn();

      Link.unfreeze();
      if(repaircharge)
      {
        //       if (get_bit(quest_rules,qr_REPAIRFIX)) {
        //         fixed_door=true;
        //       }

        game.drupy-=tmpscr[1].catchall;
      }

      if(adjustmagic)
      {
        game.magicdrainrate=1;
        sfx(WAV_SCALE);
        setmapflag();
      }
      if(learnslash)
      {
        game.canslash=1;
        sfx(WAV_SCALE);
        setmapflag();
      }
    }
  }
}

void domoney()
{
  static bool sfxon = false;

  if(game.drupy==0)
  {
    sfxon = false;
    return;
  }

  if(frame&1)
  {
    sfxon = true;
    if(game.drupy>0)
    {
      int max = 255;
      if(current_item(itype_wallet,true)==1)
        max = 500;
      if(get_bit(quest_rules,qr_999R) || current_item(itype_wallet,true)==2)
        max = 999;
      if(game.rupies < max)
      {
        ++game.rupies;
        --game.drupy;
      }
      else game.drupy=0;
    }
    else
    {
      if(game.rupies>0)
      {
        --game.rupies;
        ++game.drupy;
      }
      else game.drupy=0;
    }
  }

  if(sfxon && !lensclk)
    sfx(WAV_MSG);
}

void domagic()                                              //basically a copy of domoney()
{
  if (magicdrainclk==32767)
  {
    magicdrainclk=-1;
  }
  ++magicdrainclk;

  static bool sfxon = false;

  if(game.dmagic==0)
  {
    sfxon = false;
    return;
  }

  if (game.dmagic>0)
  {
    if(frame&1)
    {
      sfxon = true;
      if(game.magic < game.maxmagic)
      {
        game.magic+=MAGICPERBLOCK/4;
        game.dmagic-=MAGICPERBLOCK/4;
      }
      else
      {
        game.dmagic=0;
        game.magic=game.maxmagic;
      }
    }
    if(sfxon && !lensclk)
      sfx(WAV_MSG);
  }
  else
  {
    if(frame&1)
    {
      sfxon = true;
      if(game.magic>0)
      {
        game.magic-=2*game.magicdrainrate;
        game.dmagic+=2*game.magicdrainrate;
      }
      else
      {

        game.dmagic=0;
        game.magic=0;
      }
    }
  }
}

/***  Collision detection & handling  ***/

void check_collisions()
{
  for(int i=0; i<Lwpns.Count(); i++)
  {
    weapon *w = (weapon*)Lwpns.spr(i);
    if(!(w->Dead()))
    {
      for(int j=0; j<guys.Count(); j++)
      {
        if(((enemy*)guys.spr(j))->hit(w))
        {
          int h = ((enemy*)guys.spr(j)) -> takehit(w->id,w->power,w->x,w->y,w->dir);
          if(h)
          {
            w->onhit(false);
          }
          if(h==2)
          {
            break;
          }
        }
        if(w->Dead())
        {
          break;
        }
      }
      if (get_bit(quest_rules,qr_Z3BRANG_HSHOT))
      {
        if(w->id == wBrang || w->id==wHookshot)
        {
          for(int j=0; j<items.Count(); j++)
          {
            if(items.spr(j)->hit(w))
            {
              if(((item*)items.spr(j))->pickup & ipTIMER)
              {
                if(((item*)items.spr(j))->clk2 >= 32)
                {
                  if (w->dragging==-1)
                  {
                    w->dead=1;
                    if (w->id==wBrang&&w->dummy_bool[0]&&(w->misc==1)&&current_item(itype_brang,true)>1)
                    {
                      add_grenade(w->x,w->y,current_item(itype_brang,true)>2);
                      w->dummy_bool[0]=false;
                    }
                    ((item*)items.spr(j))->clk2=256;
                    w->dragging=j;
                  }
                }
              }
            }
          }
        }
      }
      else
      {
        if(w->id == wBrang || w->id == wArrow || w->id==wHookshot)
        {
          for(int j=0; j<items.Count(); j++)
          {
            if(items.spr(j)->hit(w))
            {
              if(((item*)items.spr(j))->pickup & ipTIMER)
              {
                if(((item*)items.spr(j))->clk2 >= 32)
                {
                  getitem(items.spr(j)->id);
                  items.del(j);
                  --j;
                }
              }
            }
          }
        }
      }
    }
  }
}

// messy code to do the enemy-carrying-the-item thing

void dragging_item()
{
  if (get_bit(quest_rules,qr_Z3BRANG_HSHOT))
  {
    for(int i=0; i<Lwpns.Count(); i++)
    {
      weapon *w = (weapon*)Lwpns.spr(i);
      if(w->id == wBrang || w->id==wHookshot)
      {
        if (w->dragging>=0 && w->dragging<items.Count())
        {
          items.spr(w->dragging)->x=w->x;
          items.spr(w->dragging)->y=w->y;
        }
      }
    }
  }
}

void roaming_item()
{
  if(hasitem!=2 || !loaded_enemies)
    return;

  if(guys.Count()==0)                                       // Only case is when you clear all the guys and
  {                                                         // leave w/out getting item, then return to room.
    return;                                                 // We'll let LinkClass::checkspecial() handle it.
  }

  if(itemindex==-1)
  {
    guyindex = -1;
    for(int i=0; i<guys.Count(); i++)
    {
      if(((enemy*)guys.spr(i))->itemguy)
      {
        guyindex = i;
      }
    }
    if (guyindex == -1)                                     //This happens when "default enemies" such as
    {
      return;                                               //eFBALL are alive but enemies from the list
    }                                                       //are not. Defer to LinkClass::checkspecial().

    int Item=tmpscr->item;
    if(getmapflag())
    {
      Item=0;
    }
    if(Item)
    {
      itemindex=items.Count();
      additem(0,0,Item,ipENEMY+ipONETIME+ipBIGRANGE
        + ( ((tmpscr->flags3&fHOLDITEM) || (Item==iTriforce)) ? ipHOLDUP : 0)
        );
    }
    else
    {
      hasitem=0;
      return;
    }
  }
  if (get_bit(quest_rules,qr_HIDECARRIEDITEMS))
  {
    items.spr(itemindex)->x = -1000;
    items.spr(itemindex)->y = -1000;
  }
  else
  {
    items.spr(itemindex)->x = guys.spr(guyindex)->x;
    items.spr(itemindex)->y = guys.spr(guyindex)->y - 2;
  }
}

/*** end of guys.cc ***/
