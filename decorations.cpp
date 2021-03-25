//--------------------------------------------------------
//  Zelda Classic
//  by Jeremy Craner, 1999-2000
//
//  decorations.cpp
//
//--------------------------------------------------------

#include "sprite.h"
#include "decorations.h"
#include "zelda.h"
#include "maps.h"

/***************************************/
/*******  Decoration Base Class  *******/
/***************************************/

decoration::decoration(fix X, fix Y, int Id, int Clk) : sprite() {
	x = X;
	y = Y;
	id = Id;
	clk = Clk;
	misc = 0;
	yofs = 54;

}

decoration::~decoration() {}

/*******************************/
/*******   Decorations   *******/
/*******************************/

dBushLeaves::dBushLeaves(fix X, fix Y, int Id, int Clk) : decoration(X, Y, Id, Clk) {
	ox = X;
	oy = Y;
	id = Id;
	clk = Clk;
	ft[0][0][0] = 6;
	ft[0][0][1] = 6;
	ft[0][0][2] = 0;
	ft[0][1][0] = 6;
	ft[0][1][1] = 9;
	ft[0][1][2] = 0;
	ft[0][2][0] = 7;
	ft[0][2][1] = 11;
	ft[0][2][2] = 1;
	ft[0][3][0] = 7;
	ft[0][3][1] = 13;
	ft[0][3][2] = 0;
	ft[0][4][0] = 9;
	ft[0][4][1] = 15;
	ft[0][4][2] = 0;
	ft[0][5][0] = 10;
	ft[0][5][1] = 16;
	ft[0][5][2] = 0;
	ft[0][6][0] = 11;
	ft[0][6][1] = 18;
	ft[0][6][2] = 0;
	ft[0][7][0] = 4;
	ft[0][7][1] = 19;
	ft[0][7][2] = 1;

	ft[1][0][0] = -4;
	ft[1][0][1] = 3;
	ft[1][0][2] = 0;
	ft[1][1][0] = -1;
	ft[1][1][1] = 4;
	ft[1][1][2] = 0;
	ft[1][2][0] = 0;
	ft[1][2][1] = 5;
	ft[1][2][2] = 0;
	ft[1][3][0] = 1;
	ft[1][3][1] = 5;
	ft[1][3][2] = 1;
	ft[1][4][0] = -2;
	ft[1][4][1] = 5;
	ft[1][4][2] = 1;
	ft[1][5][0] = -6;
	ft[1][5][1] = 5;
	ft[1][5][2] = 0;
	ft[1][6][0] = -7;
	ft[1][6][1] = 4;
	ft[1][6][2] = 0;
	ft[1][7][0] = -9;
	ft[1][7][1] = 2;
	ft[1][7][2] = 1;

	ft[2][0][0] = 10;
	ft[2][0][1] = 2;
	ft[2][0][2] = 1;
	ft[2][1][0] = 7;
	ft[2][1][1] = 3;
	ft[2][1][2] = 1;
	ft[2][2][0] = 4;
	ft[2][2][1] = 5;
	ft[2][2][2] = 1;
	ft[2][3][0] = 5;
	ft[2][3][1] = 5;
	ft[2][3][2] = 0;
	ft[2][4][0] = 8;
	ft[2][4][1] = 9;
	ft[2][4][2] = 0;
	ft[2][5][0] = 9;
	ft[2][5][1] = 9;
	ft[2][5][2] = 0;
	ft[2][6][0] = 12;
	ft[2][6][1] = 9;
	ft[2][6][2] = 1;
	ft[2][7][0] = 6;
	ft[2][7][1] = 16;
	ft[2][7][2] = 0;

	ft[3][0][0] = 4;
	ft[3][0][1] = -4;
	ft[3][0][2] = 0;
	ft[3][1][0] = 4;
	ft[3][1][1] = -6;
	ft[3][1][2] = 0;
	ft[3][2][0] = 2;
	ft[3][2][1] = -7;
	ft[3][2][2] = 0;
	ft[3][3][0] = 1;
	ft[3][3][1] = -8;
	ft[3][3][2] = 3;
	ft[3][4][0] = 0;
	ft[3][4][1] = -9;
	ft[3][4][2] = 0;
	ft[3][5][0] = -1;
	ft[3][5][1] = -11;
	ft[3][5][2] = 0;
	ft[3][6][0] = -2;
	ft[3][6][1] = -14;
	ft[3][6][2] = 0;
	ft[3][7][0] = -3;
	ft[3][7][1] = -16;
	ft[3][7][2] = 0;
}

bool dBushLeaves::animate(int index) {
	return (clk++ >= 24);
}

void dBushLeaves::draw(BITMAP *dest) {
	int t = wpnsbuf[iwBushLeaves].tile;
	cs = wpnsbuf[iwBushLeaves].csets & 15;
	for (int i = 0; i < 4; ++i) {
		x = ox + ft[i][int(float(clk - 1) / 3)][0];
		y = oy + ft[i][int(float(clk - 1) / 3)][1];
		flip = ft[i][int(float(clk - 1) / 3)][2];
		tile = t * 4 + i;
		decoration::draw8(dest);
	}
}

dFlowerClippings::dFlowerClippings(fix X, fix Y, int Id, int Clk) : decoration(X, Y, Id, Clk) {
	ox = X;
	oy = Y;
	id = Id;
	clk = Clk;
	ft[0][0][0] = 6;
	ft[0][0][1] = 6;
	ft[0][0][2] = 0;
	ft[0][1][0] = 6;
	ft[0][1][1] = 9;
	ft[0][1][2] = 0;
	ft[0][2][0] = 7;
	ft[0][2][1] = 11;
	ft[0][2][2] = 1;
	ft[0][3][0] = 7;
	ft[0][3][1] = 13;
	ft[0][3][2] = 0;
	ft[0][4][0] = 9;
	ft[0][4][1] = 15;
	ft[0][4][2] = 0;
	ft[0][5][0] = 10;
	ft[0][5][1] = 16;
	ft[0][5][2] = 0;
	ft[0][6][0] = 11;
	ft[0][6][1] = 18;
	ft[0][6][2] = 0;
	ft[0][7][0] = 4;
	ft[0][7][1] = 19;
	ft[0][7][2] = 1;

	ft[1][0][0] = -4;
	ft[1][0][1] = 3;
	ft[1][0][2] = 0;
	ft[1][1][0] = -1;
	ft[1][1][1] = 4;
	ft[1][1][2] = 0;
	ft[1][2][0] = 0;
	ft[1][2][1] = 5;
	ft[1][2][2] = 0;
	ft[1][3][0] = 1;
	ft[1][3][1] = 5;
	ft[1][3][2] = 1;
	ft[1][4][0] = -2;
	ft[1][4][1] = 5;
	ft[1][4][2] = 1;
	ft[1][5][0] = -6;
	ft[1][5][1] = 5;
	ft[1][5][2] = 0;
	ft[1][6][0] = -7;
	ft[1][6][1] = 4;
	ft[1][6][2] = 0;
	ft[1][7][0] = -9;
	ft[1][7][1] = 2;
	ft[1][7][2] = 1;

	ft[2][0][0] = 10;
	ft[2][0][1] = 2;
	ft[2][0][2] = 1;
	ft[2][1][0] = 7;
	ft[2][1][1] = 3;
	ft[2][1][2] = 1;
	ft[2][2][0] = 4;
	ft[2][2][1] = 5;
	ft[2][2][2] = 1;
	ft[2][3][0] = 5;
	ft[2][3][1] = 5;
	ft[2][3][2] = 0;
	ft[2][4][0] = 8;
	ft[2][4][1] = 9;
	ft[2][4][2] = 0;
	ft[2][5][0] = 9;
	ft[2][5][1] = 9;
	ft[2][5][2] = 0;
	ft[2][6][0] = 12;
	ft[2][6][1] = 9;
	ft[2][6][2] = 1;
	ft[2][7][0] = 6;
	ft[2][7][1] = 16;
	ft[2][7][2] = 0;

	ft[3][0][0] = 4;
	ft[3][0][1] = -4;
	ft[3][0][2] = 0;
	ft[3][1][0] = 4;
	ft[3][1][1] = -6;
	ft[3][1][2] = 0;
	ft[3][2][0] = 2;
	ft[3][2][1] = -7;
	ft[3][2][2] = 0;
	ft[3][3][0] = 1;
	ft[3][3][1] = -8;
	ft[3][3][2] = 3;
	ft[3][4][0] = 0;
	ft[3][4][1] = -9;
	ft[3][4][2] = 0;
	ft[3][5][0] = -1;
	ft[3][5][1] = -11;
	ft[3][5][2] = 0;
	ft[3][6][0] = -2;
	ft[3][6][1] = -14;
	ft[3][6][2] = 0;
	ft[3][7][0] = -3;
	ft[3][7][1] = -16;
	ft[3][7][2] = 0;
}

bool dFlowerClippings::animate(int index) {
	return (clk++ >= 24);
}

void dFlowerClippings::draw(BITMAP *dest) {
	int t = wpnsbuf[iwFlowerClippings].tile;
	cs = wpnsbuf[iwFlowerClippings].csets & 15;
	for (int i = 0; i < 4; ++i) {
		x = ox + ft[i][int(float(clk - 1) / 3)][0];
		y = oy + ft[i][int(float(clk - 1) / 3)][1];
		flip = ft[i][int(float(clk - 1) / 3)][2];
		tile = t * 4 + i;
		decoration::draw8(dest);
	}
}

dGrassClippings::dGrassClippings(fix X, fix Y, int Id, int Clk) : decoration(X, Y, Id, Clk) {
	ox = X;
	oy = Y;
	id = Id;
	clk = Clk;
	ft[0][0][0] = 1;
	ft[0][0][1] = 0;
	ft[0][0][2] = 1;
	ft[0][0][3] = 0;
	ft[0][1][0] = -1;
	ft[0][1][1] = 4;
	ft[0][1][2] = 1;
	ft[0][1][3] = 0;
	ft[0][2][0] = -5;
	ft[0][2][1] = 2;
	ft[0][2][2] = 0;
	ft[0][2][3] = 1;
	ft[0][3][0] = -8;
	ft[0][3][1] = 0;
	ft[0][3][2] = 0;
	ft[0][3][3] = 1;

	ft[1][0][0] = 3;
	ft[1][0][1] = -4;
	ft[1][0][2] = 0;
	ft[1][0][3] = 1;
	ft[1][1][0] = 8;
	ft[1][1][1] = -5;
	ft[1][1][2] = 3;
	ft[1][1][3] = 1;
	ft[1][2][0] = 8;
	ft[1][2][1] = -5;
	ft[1][2][2] = 3;
	ft[1][2][3] = 0;
	ft[1][3][0] = 8;
	ft[1][3][1] = -5;
	ft[1][3][2] = 0;
	ft[1][3][3] = 1;

	ft[2][0][0] = 8;
	ft[2][0][1] = 1;
	ft[2][0][2] = 0;
	ft[2][0][3] = 1;
	ft[2][1][0] = 10;
	ft[2][1][1] = 4;
	ft[2][1][2] = 1;
	ft[2][1][3] = 1;
	ft[2][2][0] = 15;
	ft[2][2][1] = 3;
	ft[2][2][2] = 0;
	ft[2][2][3] = 0;
	ft[2][3][0] = 16;
	ft[2][3][1] = 5;
	ft[2][3][2] = 0;
	ft[2][3][3] = 1;
}

bool dGrassClippings::animate(int index) {
	return (clk++ >= 12);
}

void dGrassClippings::draw(BITMAP *dest) {
	int t = wpnsbuf[iwGrassClippings].tile;
	cs = wpnsbuf[iwGrassClippings].csets & 15;
	for (int i = 0; i < 3; ++i) {
		x = ox + ft[i][int(float(clk - 1) / 3)][0];
		y = oy + ft[i][int(float(clk - 1) / 3)][1];
		flip = ft[i][int(float(clk - 1) / 3)][2];
		tile = (t + (ft[i][int(float(clk - 1) / 3)][3])) * 4 + i;
		decoration::draw8(dest);
	}
}

dHammerSmack::dHammerSmack(fix X, fix Y, int Id, int Clk) : decoration(X, Y, Id, Clk) {
	ox = X;
	oy = Y;
	id = Id;
	clk = Clk;
	ft[0][0][0] = -5;
	ft[0][0][1] = -4;
	ft[0][0][2] = 0;
	ft[0][1][0] = -4;
	ft[0][1][1] = -3;
	ft[0][1][2] = 1;
	ft[0][2][0] = -5;
	ft[0][2][1] = -4;
	ft[0][2][2] = 1;
	ft[0][3][0] = -8;
	ft[0][3][1] = -7;
	ft[0][3][2] = 1;

	ft[1][0][0] = 3;
	ft[1][0][1] = -4;
	ft[1][0][2] = 0;
	ft[1][1][0] = 5;
	ft[1][1][1] = -3;
	ft[1][1][2] = 1;
	ft[1][2][0] = 6;
	ft[1][2][1] = -4;
	ft[1][2][2] = 1;
	ft[1][3][0] = 9;
	ft[1][3][1] = -7;
	ft[1][3][2] = 1;
}

bool dHammerSmack::animate(int index) {
	return (clk++ >= 12);
}

void dHammerSmack::draw(BITMAP *dest) {
	int t = wpnsbuf[iwHammerSmack].tile;
	cs = wpnsbuf[iwHammerSmack].csets & 15;
	flip = 0;
	for (int i = 0; i < 2; ++i) {
		x = ox + ft[i][int(float(clk - 1) / 3)][0];
		y = oy + ft[i][int(float(clk - 1) / 3)][1];
		tile = t * 4 + i + (ft[i][int(float(clk - 1) / 3)][2] * 2);
		decoration::draw8(dest);
	}
}

dTallGrass::dTallGrass(fix X, fix Y, int Id, int Clk) : decoration(X, Y, Id, Clk) {
	id = Id;
	clk = Clk;
}

bool dTallGrass::animate(int index) {
	return ((COMBOTYPE(LinkX(), LinkY() + 15) != cTALLGRASS) ||
	        (COMBOTYPE(LinkX() + 15, LinkY() + 15) != cTALLGRASS));
}

void dTallGrass::draw(BITMAP *dest) {
	int t = wpnsbuf[iwTallGrass].tile * 4;
	cs = wpnsbuf[iwTallGrass].csets & 15;
	flip = 0;
	x = LinkX();
	y = LinkY() + 10;
	if (BSZ) {
		tile = t + (anim_3_4(LinkLStep(), 7) * 2);
	} else {
		tile = t + ((LinkLStep() >= 6) ? 2 : 0);
	}
	decoration::draw8(dest);
	x += 8;
	++tile;
	decoration::draw8(dest);
}

dRipples::dRipples(fix X, fix Y, int Id, int Clk) : decoration(X, Y, Id, Clk) {
	id = Id;
	clk = Clk;
}

bool dRipples::animate(int index) {
	clk++;
	return ((COMBOTYPE(LinkX(), LinkY() + 15) != cSHALLOWWATER) ||
	        (COMBOTYPE(LinkX() + 15, LinkY() + 15) != cSHALLOWWATER));
}

void dRipples::draw(BITMAP *dest) {
	int t = wpnsbuf[iwRipples].tile * 4;
	cs = wpnsbuf[iwRipples].csets & 15;
	flip = 0;
	x = LinkX();
	y = LinkY() + 10;
	tile = t + (((clk / 8) % 3) * 2);
	decoration::draw8(dest);
	x += 8;
	++tile;
	decoration::draw8(dest);
}

dNayrusLoveShield::dNayrusLoveShield(fix X, fix Y, int Id, int Clk) : decoration(X, Y, Id, Clk) {
	id = Id;
	clk = Clk;
}

bool dNayrusLoveShield::animate(int index) {
	clk++;
	return LinkNayrusLoveShieldClk() <= 0;
}



void dNayrusLoveShield::realdraw(BITMAP *dest, int draw_what) {
	if (misc != draw_what) {
		return;
	}
	int fb = (misc == 0 ? iwNayrusLoveShieldFront : iwNayrusLoveShieldBack);
	int t = wpnsbuf[fb].tile;
	int fr = wpnsbuf[fb].frames;
	int sp = wpnsbuf[fb].speed;
	cs = wpnsbuf[fb].csets & 15;
	flip = 0;
	if (((LinkNayrusLoveShieldClk() & 0x20) || (LinkNayrusLoveShieldClk() & 0xF00)) && ((!get_bit(quest_rules, qr_FLICKERINGNAYRUSLOVESHIELD)) || ((misc == 1) ? (frame & 1) : (!(frame & 1))))) {
		drawstyle = get_bit(quest_rules, qr_TRANSLUCENTNAYRUSLOVESHIELD) ? 1 : 0;
		x = LinkX() - 8;
		y = LinkY() - 8;
		tile = t;
		if (fr > 0 && sp > 0) {
			tile += ((clk / sp) % fr);
		}
		decoration::draw(dest);
		x += 16;
		tile += fr;
		decoration::draw(dest);
		x -= 16;
		y += 16;
		tile += fr;
		decoration::draw(dest);
		x += 16;
		tile += fr;
		decoration::draw(dest);
	}
}

void dNayrusLoveShield::draw(BITMAP *dest) {
	realdraw(dest, 0);
}

void dNayrusLoveShield::draw2(BITMAP *dest) {
	realdraw(dest, 1);
}


/*** end of decorations.cpp ***/
