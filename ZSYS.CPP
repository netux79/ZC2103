//--------------------------------------------------------
//  Zelda Classic
//  by Jeremy Craner, 1999-2000
//
//  zsys.cc
//
//  System functions, etc.
//
//--------------------------------------------------------

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "zdefs.h"
#include "zsys.h"
#include "zc_sys.h"

char *time_str_long(dword time)
{
  static char s[16];

  dword decs = (time%60)*100/60;
  dword secs = (time/60)%60;
  dword mins = (time/3600)%60;
  dword hours = time/216000;

  sprintf(s,"%ld:%02ld:%02ld.%02ld",hours,mins,secs,decs);
  return s;
}

char *time_str_med(dword time)
{
  static char s[16];

  dword secs = (time/60)%60;
  dword mins = (time/3600)%60;
  dword hours = time/216000;

  sprintf(s,"%ld:%02ld:%02ld",hours,mins,secs);
  return s;
}

char *time_str_short(dword time)
{
  static char s[16];

  dword mins = (time/3600)%60;
  dword hours = time/216000;

  sprintf(s,"%ld:%02ld",hours,mins);
  return s;
}

/*
void extract_name(char *path,char *name,int type)
{
  int l=strlen(path);
  int i=l;
  while(i>0 && path[i-1]!='/' && path[i-1]!='\\')
    --i;
  int n=0;
  if(type==FILENAME8__)
  {
    while(i<l && n<8 && path[i]!='.')
      name[n++]=path[i++];
  }
  else if (type==FILENAME8_3)
  {
    while(i<l && n<12 )
      name[n++]=path[i++];
  }
  else
  {
    while(i<l)
      name[n++]=path[i++];
  }
  name[n]=0;
}

void chop_path(char *path)
{
  int p = strlen(path);
  int f = strlen(get_filename(path));
  if(f<p)
    path[p-f]=0;
}

char *temp_name(char *s)
{
  int tempnum;
  static char *temporaryname=(char*)malloc(L_tmpnam);
  //  sprintf(temporaryname, "tempfile.qsu");
  //  return temporaryname;


  for (int i=0; i<1000; ++i)
  {
    sprintf(temporaryname, "00000000.tmp");
    for (int j=0; j<8; ++j)
    {
      tempnum=rand()%62;
      if (tempnum<26)
      {
        temporaryname[j]='A'+tempnum;
      }
      else if (tempnum<52)
        {
          temporaryname[j]='A'+tempnum-26;
        }
        else
        {
          temporaryname[j]='0'+tempnum-52;
        }
    }
    if (!exists(temporaryname))
    {
      break;
    }
  }
  if (s!=NULL)
  {
    sprintf(s, "%s", temporaryname);
  }
  return temporaryname;
}*/

int vbound(int x,int low,int high)
{
  if(x<low) x=low;
  if(x>high) x=high;
  return x;
}

float vbound(float x,float low,float high)
{
  if(x<low) x=low;
  if(x>high) x=high;
  return x;
}

int used_switch(int argc,char *argv[],char *s)
{
  // assumes a switch won't be in argv[0]
  for(int i=1; i<argc; i++)
    if(stricmp(argv[i],s)==0)
      return i;
  return 0;
}

// Returns the first no switch (-) argv param
char *get_cmd_arg(int argc,char *argv[])
{
  // assumes a switch won't be in argv[0] since it is the exe name.
  for(int i=1; i<argc; i++)
    if(argv[i][0]!='-')
      return argv[i];
  return NULL;
}

char datapwd[8] = { 'l'+11,'o'+22,'n'+33,'g'+44,'t'+55,'a'+66,'n'+77,0+88 };

void resolve_password(char *pwd)
{
  for(int i=0; i<8; i++)
    pwd[i]-=(i+1)*11;
}

void set_bit(byte *bitstr,int bit,byte val)
{
  bitstr += bit>>3;
  byte mask = 1 << (bit&7);

  if(val)
    *bitstr |= mask;
  else
    *bitstr &= ~mask;
}

int get_bit(byte *bitstr,int bit)
{
  bitstr += bit>>3;
  return ((*bitstr) >> (bit&7))&1;
}

void Z_error(char *format,...)
{
  char buf[256];

  va_list ap;
  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);

  #ifdef ALLEGRO_DOS
  printf("%s\n",buf);
  #elif defined(ALLEGRO_WINDOWS)
  al_trace("%s\n",buf);
  #elif defined(ALLEGRO_LINUX)
  al_trace("%s\n",buf);
  #elif defined(ALLEGRO_MACOSX)
  al_trace("%s\n",buf);
  #endif
  exit(1);
}

void Z_message(char *format,...)
{
  char buf[2048];

  va_list ap;
  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);

  #ifdef ALLEGRO_DOS
  printf("%s",buf);
  #elif defined(ALLEGRO_WINDOWS)
  al_trace("%s",buf);
  #elif defined(ALLEGRO_LINUX)
  al_trace("%s",buf);
  #elif defined(ALLEGRO_MACOSX)
  al_trace("%s",buf);
  #endif
}

void Z_title(char *format,...)
{
  char buf[256];
  va_list ap;
  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);

  #ifdef ALLEGRO_DOS
  printf("%s\n",buf);
  al_trace("%s\n",buf);  
  #elif defined(ALLEGRO_WINDOWS)
  al_trace("%s\n",buf);
  #elif defined(ALLEGRO_LINUX)
  al_trace("%s\n",buf);
  #elif defined(ALLEGRO_MACOSX)
  al_trace("%s\n",buf);
  #endif
}

int anim_3_4(int clk, int speed)
{
  clk /= speed;
  switch(clk&3)
  {
    case 0:
    case 2: clk = 0; break;
    case 1: clk = 1; break;
    case 3: clk = 2; break;
  }
  return clk;
}

/**********  Encryption Stuff  *****************/

//#define MASK 0x4C358938
static int seed = 0;
//#define MASK 0x91B2A2D1
//static int seed = 7351962;
static int enc_mask[3]={0x4C358938,0x91B2A2D1,0x4A7C1B87};
static int pvalue[3]={0x62E9,0x7D14,0x1A82};
static int qvalue[3]={0x3619,0xA26B,0xF03C};

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
  //  CX += 0x62E9;
  //  BX += 0x3619 + D;
  CX += pvalue[method];
  BX += qvalue[method] + D;
  seed = (BX << 16) + CX;
  return (CX << 16) + BX;
}

void encode_007(byte *buf, dword size, dword key, word *check1, word *check2, int method)
{
  dword i;
  byte *p;

  *check1 = 0;
  *check2 = 0;

  p = buf;
  for(i=0; i<size; i++)
  {
    *check1 += *p;
    *check2 = (*check2 << 4) + (*check2 >> 12) + *p;
    ++p;
  }

  p = buf;
  seed = key;
  for(i=0; i<size; i+=2)
  {
    byte q = rand_007(method);
    *p ^= q;
    ++p;
    if(i+1 < size)
    {
      *p += q;
      ++p;
    }
  }
}

bool decode_007(byte *buf, dword size, dword key, word check1, word check2, int method)
{
  dword i;
  word c1 = 0, c2 = 0;
  byte *p;

  p = buf;
  seed = key;
  for(i=0; i<size; i+=2)
  {
    char q = rand_007(method);
    *p ^= q;
    ++p;
    if(i+1 < size)
    {
      *p -= q;
      ++p;
    }
  }

  p = buf;
  for(i=0; i<size; i++)
  {
    c1 += *p;
    c2 = (c2 << 4) + (c2 >> 12) + *p;
    ++p;
  }

  return (c1 == check1) && (c2 == check2);
}

//
// RETURNS:
//   0 - OK
//   1 - srcfile not opened
//   2 - destfile not opened
//
int encode_file_007(char *srcfile, char *destfile, int key, char *header, int method)
{
  FILE *src, *dest;
  int tog = 0, c, r=0;
  short c1 = 0, c2 = 0;

  seed = key;
  src = fopen(srcfile, "rb");
  if(!src)
    return 1;

  dest = fopen(destfile, "wb");
  if(!dest)
  {
    fclose(src);
    return 2;
  }

  // write the header
  if(header)
  {
    for(c=0; header[c]; c++)
      fputc(header[c], dest);
  }

  // write the key, XORed with MASK
  key ^= enc_mask[method];
  fputc(key>>24, dest);
  fputc((key>>16)&255, dest);
  fputc((key>>8)&255, dest);
  fputc(key&255, dest);

  // encode the data
  while((c=fgetc(src)) != EOF)
  {
    c1 += c;
    c2 = (c2 << 4) + (c2 >> 12) + c;
    if(tog)
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
  fputc(c1>>8, dest);
  fputc(c1&255, dest);
  fputc(c2>>8, dest);
  fputc(c2&255, dest);

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
int decode_file_007(char *srcfile, char *destfile, char *header, int method, bool packed)
{
  FILE *normal_src=NULL, *dest=NULL;
  PACKFILE *packed_src=NULL;
  int tog = 0, c, r=0, err;
  long size, i;
  short c1 = 0, c2 = 0, check1, check2;

  // open files
  size = file_size(srcfile);
  if(size < 1)
  {
    return 1;
  }
  size -= 8;                                                // get actual data size, minus key and checksums
  if(size < 1)
  {
    return 3;
  }

  if (!packed)
  {
    normal_src = fopen(srcfile, "rb");
    if(!normal_src)
    {
      return 1;
    }
  }
  else
  {
    packed_src = pack_fopen(srcfile, F_READ_PACKED);
    if (errno==EDOM)
    {
      packed_src = pack_fopen(srcfile, F_READ);
    }
    if(!packed_src)
    {
      return 1;
    }
  }

  dest = fopen(destfile, "wb");
  if(!dest)
  {
    if (packed)
    {
      pack_fclose(packed_src);
    }
    else
    {
      fclose(normal_src);
    }
    return 2;
  }

  // read the header
  err = 4;
  if(header)
  {
    for(i=0; header[i]; i++)
    {
      if(packed)
      {
        if((c=pack_getc(packed_src)) == EOF)
        {
          goto error;
        }
      }
      else
      {
        if((c=fgetc(normal_src)) == EOF)
        {
          goto error;
        }
      }
      if((c&255) != header[i])
      {
        err = 6;
        goto error;
      }
      --size;
    }
  }

  // read the key
  if(packed)
  {
    if((c=pack_getc(packed_src)) == EOF)
    {
      goto error;
    }
  }
  else
  {
    if((c=fgetc(normal_src)) == EOF)
    {
      goto error;
    }
  }
  seed = c << 24;
  if(packed)
  {
    if((c=pack_getc(packed_src)) == EOF)
    {
      goto error;
    }
  }
  else
  {
    if((c=fgetc(normal_src)) == EOF)
    {
      goto error;
    }
  }
  seed += (c & 255) << 16;
  if(packed)
  {
    if((c=pack_getc(packed_src)) == EOF)
    {
      goto error;
    }
  }
  else
  {
    if((c=fgetc(normal_src)) == EOF)
    {
      goto error;
    }
  }
  seed += (c & 255) << 8;
  if(packed)
  {
    if((c=pack_getc(packed_src)) == EOF)
    {
      goto error;
    }
  }
  else
  {
    if((c=fgetc(normal_src)) == EOF)
    {
      goto error;
    }
  }
  seed += c & 255;
  seed ^= enc_mask[method];

  // decode the data
  for(i=0; i<size; i++)
  {
    if(packed)
    {
      if((c=pack_getc(packed_src)) == EOF)
      {
        goto error;
      }
    }
    else
    {
      if((c=fgetc(normal_src)) == EOF)
      {
        goto error;
      }
    }

    if(tog)
    {
      c -= r;
    }
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
  if(packed)
  {
    if((c=pack_getc(packed_src)) == EOF)
    {
      goto error;
    }
  }
  else
  {
    if((c=fgetc(normal_src)) == EOF)
    {
      goto error;
    }
  }
  check1 = c << 8;
  if(packed)
  {
    if((c=pack_getc(packed_src)) == EOF)
    {
      goto error;
    }
  }
  else
  {
    if((c=fgetc(normal_src)) == EOF)
    {
      goto error;
    }
  }
  check1 += c & 255;
  if(packed)
  {
    if((c=pack_getc(packed_src)) == EOF)
    {
      goto error;
    }
  }
  else
  {
    if((c=fgetc(normal_src)) == EOF)
    {
      goto error;
    }
  }
  check2 = c << 8;
  if(packed)
  {
    if((c=pack_getc(packed_src)) == EOF)
    {
      goto error;
    }
  }
  else
  {
    if((c=fgetc(normal_src)) == EOF)
    {
      goto error;
    }
  }
  check2 += c & 255;

  // verify checksums
  r = rand_007(method);
  check1 ^= r;
  check2 -= r;
  check1 &= 0xFFFF;
  check2 &= 0xFFFF;
  if(check1 != c1 || check2 != c2)
  {
    err = 5;
    goto error;
  }

  if (packed)
  {
    pack_fclose(packed_src);
  }
  else
  {
    fclose(normal_src);
  }
  fclose(dest);
  return 0;

  error:
  if (packed)
  {
    pack_fclose(packed_src);
  }
  else
  {
    fclose(normal_src);
  }
  fclose(dest);
  delete_file(destfile);
  return err;
}

void copy_file(char *src, char *dest)
{
  int c;
  FILE *fin, *fout;
  fin = fopen(src, "rb");
  fout = fopen(dest, "wb");
  while((c=fgetc(fin)) != EOF)
    fputc(c, fout);
  fclose(fin);
  fclose(fout);
}
