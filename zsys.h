//--------------------------------------------------------
//  Zelda Classic
//  by Jeremy Craner, 1999-2000
//
//  zsys.h
//
//  System functions, etc.
//
//--------------------------------------------------------

#ifdef ALLEGRO_WINDOWS
#include <conio.h>
#endif

#ifndef _ZSYS_H_
#define _ZSYS_H_

#include "zdefs.h"

#define FILENAME8_3   0
#define FILENAME8__   1
#define FILENAMEALL   2

char* time_str(dword time);

int  vbound(int x, int low, int high);
float vbound(float x, float low, float high);
int  used_switch(int argc, char* argv[], const char* s);
char* get_cmd_arg(int argc, char* argv[]);
bool isinRect(int x, int y, int rx1, int ry1, int rx2, int ry2);

extern char datapwd[8];
void resolve_password(char* pwd);

bool decode_007(byte* buf, dword size, dword key, word check1, word check2, int method);
void encode_007(byte* buf, dword size, dword key, word* check1, word* check2, int method);
int encode_file_007(char* srcfile, char* destfile, unsigned int key, const char* header, int method);
int decode_file_007(char* srcfile, char* destfile, const char* header, int method, bool packed);
void copy_file(char* src, char* dest);

int  get_bit(byte* bitstr, int bit);
void set_bit(byte* bitstr, int bit, byte val);

void Z_error(const char* format, ...);
void Z_message(const char* format, ...);

int anim_3_4(int clk, int speed);
#endif                                                      // _ZSYS_H_
