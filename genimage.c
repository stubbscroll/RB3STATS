#include <stdio.h>
#include <string.h>
#include <SDL/SDL.h>
#include "sdlfont.h"

#define WIDTH 384 /* banner width */
#define GAP 4     /* gap between neighbouring fields */
#define YSTRIDE 8 /* distance between the top of two consecutive lines */
#define YOFFS 1   /* text starts at top of line + this offset */
#define YSTART 15 /* y-coordinate of top of text in banner */

typedef unsigned char uchar;

#define MAXSTR 1000
char username[MAXSTR];
char instrname[MAXSTR];
int entry[3][9];
int highest[3];
int instr;

int iconw;

sdl_font *font;
SDL_Surface *banner;

void loadstats() {
	FILE *f=fopen("stats.txt","r");
	int i,j;
	static char t[MAXSTR];
	if(!f) exit(1);
	snprintf(t,MAXSTR-1,"%%%ds",MAXSTR);
	fscanf(f,t,username);
	fscanf(f,t,instrname);
	instr=-1;
	if(!strcmp(instrname,"prokeys")) instr=0;
	else if(!strcmp(instrname,"progtr")) instr=1;
	else if(!strcmp(instrname,"probass")) instr=2;
	for(i=0;i<3;i++) {
		for(j=0;j<8;j++) fscanf(f,"%d",&entry[i][j]);
		fscanf(f,"%d",&highest[i]);
	}
	for(i=0;username[i];i++) username[i]=toupper(username[i]);
	fclose(f);
}

/* create gradient along entire width, from 0 to y (inclusive),
   with r1,g1,b1 as top colour and r2,g2,b2 at bottom colour */
void gradient(SDL_Surface *surface,int w,int y,int r1,int g1,int b1,int r2,int g2,int b2) {
	int i,pitch=surface->pitch,j,r,g,b;
	double d;
	Uint8 *p;
	for(i=0;i<=y;i++) {
		p=surface->pixels+pitch*i;
		d=i*1./y;
		r=r1+(r2-r1)*d;
		g=g1+(g2-g1)*d;
		b=b1+(b2-b1)*d;
		for(j=0;j<w;j++) {
			*p++=b;
			*p++=g;
			*p++=r;
			*p++=255;
		}
	}
}

/* draw icon on left side of banner, colour scaled by gradient */
/* assume icon is 24-bit */
void impose(SDL_Surface *surface) {
	SDL_Surface *icon=0;
	int i,j;
	Uint8 *p,*q;
	if(!instr) icon=SDL_LoadBMP("rb3prokeys.bmp");
	else if(instr==1) icon=SDL_LoadBMP("rb3progtr.bmp");
	else if(instr==2) icon=SDL_LoadBMP("rb3probass.bmp");
	if(!icon) exit(1);
	iconw=icon->w;
	for(i=0;i<icon->h;i++) {
		p=surface->pixels+i*surface->pitch;
		q=icon->pixels+i*icon->pitch;
		for(j=0;j<icon->w;j++) {
			*p=*p**q++/255; p++;
			*p=*p**q++/255; p++;
			*p=*p**q++/255; p+=2;
		}
	}
	SDL_FreeSurface(icon);
}

void center(int x1,int x2,int y,Uint32 col,char *fmt,...) {
  static char t[MAXSTR];
	int w;
  va_list argptr;
  va_start(argptr,fmt);
  vsprintf(t,fmt,argptr);
  va_end(argptr);
	w=sdl_font_width(font,"%s",t);
	sdl_font_printf(banner,font,x1+(x2-x1-w)/2,y+YOFFS,col,0x888888,"%s",t);
}

void centerbox(int x1,int x2,int y,Uint32 col,char *fmt,...) {
  static char t[MAXSTR];
	int w,i,j;
  va_list argptr;
  va_start(argptr,fmt);
  vsprintf(t,fmt,argptr);
  va_end(argptr);
	w=sdl_font_width(font,"%s",t);
	for(i=y;i<y+YSTRIDE-1;i++) {
		Uint8 *p=banner->pixels+banner->pitch*i;
		for(j=x1;j<x2;j++) p[j*4]=p[j*4+1]=p[j*4+2]=p[j*4+3]=0;
	}
	sdl_font_printf(banner,font,x1+(x2-x1-w)/2,y+YOFFS,col,0x888888,"%s",t);
}

void genrank(char *u,int r) {
	sprintf(u,"%d",r);
	if(r%10==1 && r%100!=11) strcat(u,"ST");
	else if(r%10==2 && r%100!=12) strcat(u,"ND");
	else if(r%10==3 && r%100!=13) strcat(u,"RD");
	else strcat(u,"TH");
}

/* name rank/score clr 3* 4* 5* gs 100% */
int w[]={30,110,20,20,20,20,20,20};
char header[6][10]={"CLR","3*","4*","5*","GS","100%%"};

void create(char *filename) {
	int atx,i=0,j;
	char u[20];
	if(!(banner=SDL_CreateRGBSurface(0,WIDTH,70,32,0,0,0,0))) exit(1);
	gradient(banner,WIDTH,63,0,0,255,130,130,255);
	impose(banner);
	sdl_font_printf(banner,font,0,65,0xFFFFFF,0x888888,"PLAYER: %s",username);
	atx=iconw+GAP;
	/* song chunk name */
	sdl_font_printf(banner,font,atx+w[i]-sdl_font_width(font,"RB3"  ),YSTART+YOFFS+YSTRIDE*1,0xFFFFFF,0x888888,"RB3"  );
	sdl_font_printf(banner,font,atx+w[i]-sdl_font_width(font,"BLITZ"),YSTART+YOFFS+YSTRIDE*2,0xFFFFFF,0x888888,"BLITZ");
	sdl_font_printf(banner,font,atx+w[i]-sdl_font_width(font,"DLC"  ),YSTART+YOFFS+YSTRIDE*3,0xFFFFFF,0x888888,"DLC"  );
	atx+=w[i++]+GAP;
	/* rank/rating */
	center(atx,atx+w[i],YSTART,0xFFFFFF,"RANK/SCORE/RATING");
	if(entry[0][1]) {
		genrank(u,entry[0][0]);
		centerbox(atx,atx+w[i],YSTART+YSTRIDE*1,0xFFFFFF,"%s %d (%.2f%%)",u,entry[0][1],entry[0][1]*100.0/highest[0]);
	} else centerbox(atx,atx+w[i],YSTART+YSTRIDE*1,0xFFFFFF,"N/A");
	if(entry[1][1]) {
		genrank(u,entry[1][0]);
		centerbox(atx,atx+w[i],YSTART+YSTRIDE*2,0xFFFFFF,"%s %d (%.2f%%)",u,entry[1][1],entry[1][1]*100.0/highest[1]);
	} else centerbox(atx,atx+w[i],YSTART+YSTRIDE*2,0xFFFFFF,"N/A");
	if(entry[2][1]) {
		genrank(u,entry[2][0]);
		centerbox(atx,atx+w[i],YSTART+YSTRIDE*3,0xFFFFFF,"%s %d (%.2f%%)",u,entry[2][1],entry[2][1]*100.0/highest[2]);
	} else centerbox(atx,atx+w[i],YSTART+YSTRIDE*3,0xFFFFFF,"N/A");
	atx+=w[i++]+GAP;
	/* clear 3* 4* 5* gs 100% */
	for(j=0;j<6;j++) {
		center(atx,atx+w[i],YSTART,0xFFFFFF,header[j]);
		if(entry[0][1]) centerbox(atx,atx+w[i],YSTART+YSTRIDE*1,0xFFFFFF,"%d",entry[0][j+2]);
		else centerbox(atx,atx+w[i],YSTART+YSTRIDE*1,0xFFFFFF,"");
		if(entry[1][1]) centerbox(atx,atx+w[i],YSTART+YSTRIDE*2,0xFFFFFF,"%d",entry[1][j+2]);
		else centerbox(atx,atx+w[i],YSTART+YSTRIDE*2,0xFFFFFF,"");
		if(entry[2][1]) centerbox(atx,atx+w[i],YSTART+YSTRIDE*3,0xFFFFFF,"%d",entry[2][j+2]);
		else centerbox(atx,atx+w[i],YSTART+YSTRIDE*3,0xFFFFFF,"");
		atx+=w[i++]+GAP;
	}
	SDL_SaveBMP(banner,filename);
	SDL_FreeSurface(banner);
}

/* arguments: out-file.bmp
   a stats.txt generated by rb3stats.exe must exist */
int main(int argc,char **argv) {
	if(argc<3) return 0;
  if(SDL_Init(SDL_INIT_VIDEO)<0) exit(1);
  if(!(font=sdl_font_load("font.bmp"))) exit(1);
	loadstats();
	create(argv[2]);
  sdl_font_free(font);
  SDL_Quit();
	return 0;
}
