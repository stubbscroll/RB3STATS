#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>
#include <curl/easy.h>

/* stuff that happens:
   - load score page and get score, plus count passes, 3*, 4* etc
   - the url is:
     http://rockband.scorehero.com/scores.php?user=2546&game=9&platform=2&size=1&team=0&group=25&diff=4&vox=solo
   - groups in url above:
     25 pro guitar, 26 pro bass, 28 pro keys, 27 pro drums
     1 guitar, 2 bass, 3 drums, 4 vocals, 24 keys
     also, vox=solo or vox=harmonies
   - platform: 2 360, 3 ps3, 4 wii
   - read all overall lists for all platforms:
     http://rockband.scorehero.com/rankings.php?game=9&platform=2&size=1&team=0&group=28&diff=4&vox=solo&page=0
     rb3 on-disc: song=
     blitz: song=-195
     dlc: song=-250
   - to calculate percentage, get song top scores from:
     http://rockband.scorehero.com/top_scores.php?game=9&platform=2&size=1&group=28&diff=4&vox=solo
   - platforms and instruments as above
   - from these lists, calculate our rank
   - generate fancy image (insert logo based on instrument)
*/

/* dubious assumption: webpage doesn't exceed BUF bytes */
#define BUF 50000000
char buffer[BUF];
int bptr;

void error(char *s) {
	puts(s);
	exit(1);
}

size_t webline(void *ptr,size_t size,size_t n,void *userdata) {
	memcpy(buffer+bptr,ptr,size*n);
	bptr+=size*n;
	buffer[bptr]=0;
	return n;
}

void loadwebpage(char *url) {
	CURL *curl=curl_easy_init();
	curl_easy_setopt(curl,CURLOPT_URL,url);
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,webline);
	bptr=0;
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
}

char *strstr2(char *hay,char *needle) {
	char *p=strstr(hay,needle);
	if(!p) return 0;
	return p+strlen(needle);
}

/* grab string from p ending in z and store it in s (max n-1 chars) */
void grabuntil(char *s,int n,char *p,char z) {
	while(p && *p!=z && --n) *s++=*p++;
	*s=0;
}

/* grab string from p ending in z and store it in s (max n-1 chars) */
void grabuntils(char *s,int n,char *p,char *z) {
	int i;
	while(*p && --n) {
		for(i=0;z[i];i++) if(*p==z[i]) goto done;
		*s++=*p++;
	}
done:
	*s=0;
}

int grabnumuntil(char *p,char z) {
	int v=0;
	while(p && *p!=z) {
		if(isdigit(*p)) v=v*10+*p-'0';
		p++;
	}
	return v;
}

#define MAXSTR 1000
void getscores(char *userid,char *platform,char *instrument) {
	int instrno=-1,platno=-1,user;
	static char url[MAXSTR];
	user=strtol(userid,0,10);
	if(!strcmp(instrument,"prokeys")) instrno=28;
	else if(!strcmp(instrument,"progtr")) instrno=25;
	else if(!strcmp(instrument,"probass")) instrno=26;
	if(!strcmp(platform,"360")) platno=2;
	else if(!strcmp(platform,"ps3")) platno=3;
	else if(!strcmp(platform,"wii")) platno=4;
	if(platno<0) error("platform not supported");
	if(instrno<0) error("instrument not supported");
	snprintf(url,MAXSTR-1,"http://rockband.scorehero.com/scores.php?user=%d&game=9&platform=%d&size=1&team=0&group=%d&diff=4&vox=solo",user,platno,instrno);
	printf("load scores for user %d\n",user);
	loadwebpage(url);
}

void grabtopscores(int platno,char *instrument) {
	int instrno=-1;
	static char url[MAXSTR];
	if(!strcmp(instrument,"prokeys")) instrno=28;
	else if(!strcmp(instrument,"progtr")) instrno=25;
	else if(!strcmp(instrument,"probass")) instrno=26;
	if(instrno<0) error("instrument not supported");
	snprintf(url,MAXSTR-1,"http://rockband.scorehero.com/top_scores.php?game=9&platform=%d&size=1&group=%d&diff=4&vox=solo",platno,instrno);
	printf("load top scores for platform %d\n",platno);
	loadwebpage(url);
}


void grablist(char *instrument,int platno,int listno) {
	static char url[MAXSTR];
	char liststr[3][10]={"","-195","-250"};
	int instrno=-1;
	if(!strcmp(instrument,"prokeys")) instrno=28;
	else if(!strcmp(instrument,"progtr")) instrno=25;
	else if(!strcmp(instrument,"probass")) instrno=26;
	if(instrno<0) error("instrument not supported");
	snprintf(url,MAXSTR-1,"http://rockband.scorehero.com/rankings.php?game=9&platform=%d&size=1&team=0&group=%d&diff=4&vox=solo&song=%s&page=0",platno,instrno,liststr[listno]);
	printf("load rankings for platform %d game %d\n",platno,listno);
	loadwebpage(url);
}

int entry[3][8]; /* 3: each subrank (rb3, blitz, dlc)
                    6: rank score pass 3* 4* 5* GS 100% */
int highest[3];  /* highest score for subgame */

#define MAXID 13333
int topscore[MAXID]; /* highest score for song */
int topwhere[MAXID]; /* where the song belongs */

void process(char *userid,char *platform,char *instrument) {
	int where=-1,i,r,j,k;
	char *p=buffer,*q,*u,*v;
	static char s[MAXSTR];
	static char username[MAXSTR];
	FILE *f;
	memset(entry,0,sizeof(entry));
	/* warning, string parsing code ahead */
	/* find percentage by finding top score for each song */
	memset(topscore,0,sizeof(topscore));
	memset(topwhere,0,sizeof(topwhere));
	for(i=2;i<5;i++) {
		grabtopscores(i,instrument);
		p=buffer;
		while((p=strstr2(p,"tr height"))) {
			q=strstr2(p,"class=\"");
			if(!q) error("expected class");
			if(!strncmp(q,"headrow",7)) continue;
			if(!strncmp(q,"tier1",5)) {
				q=strstr2(q,">");
				if(!q) error("expected >");
				grabuntils(s,MAXSTR-1,q,"<(");
				while(s[0] && s[strlen(s)-1]==' ') s[strlen(s)-1]=0;
				if(!strcmp(s,"Warmup")) where=0;
				else if(!strcmp(s,"Apprentice")) where=0;
				else if(!strcmp(s,"Solid")) where=0;
				else if(!strcmp(s,"Moderate")) where=0;
				else if(!strcmp(s,"Challenging")) where=0;
				else if(!strcmp(s,"Nightmare")) where=0;
				else if(!strcmp(s,"Impossible")) where=0;
				else if(!strcmp(s,"Rock Band Blitz Soundtrack")) where=1;
				else if(!strcmp(s,"Downloaded Songs - Rock Band 3")) where=2;
				else printf("section [%s] not supported, put as dlc\n",s),where=2;
			} else if(!strncmp(q,"whitecol",8)) {
				u=strstr2(q,"<tr height");
				v=strstr2(q,"<span class=\"error\">NO SCORES SUBMITTED</span>");
				if((u || v) && (!u || (v && v<u))) continue;
				/* get song id */
				q=strstr2(q,"song=");      if(!q) error("expected entry");
				j=grabnumuntil(q,'&');
				if(j<0 || j>=MAXID) error("id too high, inclease MAXID and recompile");
				q=strstr2(q,"col\">");     if(!q) error("expected entry");
				/* get score */
				if(q[0]=='<') q=strstr2(q,"_blank\">");
				k=grabnumuntil(q,'<');
				if(topscore[j]<k) topscore[j]=k,topwhere[j]=where;
			}
		}
	}
	getscores(userid,platform,instrument);
	/* find username */
	p=strstr2(buffer,"name=\"username\" value=\"");
	grabuntil(username,MAXSTR-1,p,'\"');
	/* find scores and stats */
	p=buffer;
	i=0;
	while((p=strstr2(p,"tr height"))) {
		q=strstr2(p,"class=\"");
		if(!q) error("expected class");
		if(!strncmp(q,"headrow",7)) continue;
		if(!strncmp(q,"tier1",5)) {
			q=strstr2(q,">");
			if(!q) error("expected >");
			grabuntils(s,MAXSTR-1,q,"<(");
			while(s[0] && s[strlen(s)-1]==' ') s[strlen(s)-1]=0;
			if(!strcmp(s,"Warmup")) where=0;
			else if(!strcmp(s,"Apprentice")) where=0;
			else if(!strcmp(s,"Solid")) where=0;
			else if(!strcmp(s,"Moderate")) where=0;
			else if(!strcmp(s,"Challenging")) where=0;
			else if(!strcmp(s,"Nightmare")) where=0;
			else if(!strcmp(s,"Impossible")) where=0;
			else if(!strcmp(s,"Rock Band Blitz Soundtrack")) where=1;
			else if(!strcmp(s,"Downloaded Songs - Rock Band 3")) where=2;
			else printf("section [%s] not supported, put as dlc\n",s),where=2;
		} else if(!strncmp(q,"whitecol",8)) {
			u=strstr2(q,"<tr height");
			v=strstr2(q,"<span class=\"error\">NO SCORES SUBMITTED</span>");
			if((u || v) && (!u || (v && v<u))) continue;
			entry[where][2]++;
			q=strstr2(q,"col\">");     if(!q) error("expected entry");
			q=strstr2(q,"col\">");     if(!q) error("expected entry");
			q=strstr2(q,"col\">");     if(!q) error("expected entry");
			q=strstr2(q,"col\">");     if(!q) error("expected entry");
			q=strstr2(q,"col\">");     if(!q) error("expected entry");
			/* get score */
			if(q[0]=='<') q=strstr2(q,"_blank\">");
			entry[where][1]+=grabnumuntil(q,'<');
			/* get star rating */
			q=strstr2(q,"col\">");     if(!q) error("expected entry");
			if(strncmp(q+10,"/images/rating",14)) r=0;
			else r=q[25]-'0';
			if(r>=3) entry[where][3]++;
			if(r>=4) entry[where][4]++;
			if(r>=5) entry[where][5]++;
			if(r>=6) entry[where][6]++;
			/* get percentage */
			q=strstr2(q,"col\">");     if(!q) error("expected entry");
			u=strstr2(q,"percent");
			if(u && *u=='1') entry[where][7]++;
		}
	}
	/* find rank */
	for(i=0;i<3;i++) entry[i][0]=1,highest[i]=0;
	for(i=2;i<5;i++) for(j=0;j<3;j++) {
		/* for each platform, for each subrank */
		grablist(instrument,i,j);
		p=buffer;
		while((p=strstr2(p,"tr height"))) {
			q=strstr2(p,"class=\"");
			if(!q) error("expected class");
			if(!strncmp(q,"headrow",7)) continue;
			q=strstr2(q,"center\">");     if(!q) break; /* none submitted */
			q=strstr2(q,"center\">");     if(!q) break;
			q=strstr2(q,"center\">");     if(!q) break;
			r=grabnumuntil(q,'<');
			if(r>entry[j][1]) entry[j][0]++;
		}
	}
	/* find highest */
	for(j=0;j<3;j++) highest[j]=0;
	for(j=0;j<MAXID;j++) highest[topwhere[j]]+=topscore[j];
	/* output! */
	if(!(f=fopen("stats.txt","w"))) error("couldn't open temp file for writing");
	fprintf(f,"%s %s\n",username,instrument);
	for(i=0;i<3;i++) {
		for(j=0;j<8;j++) fprintf(f,"%d ",entry[i][j]);
		fprintf(f,"%d\n",highest[i]);
	}
	if(fclose(f)) error("something bad happened when writing to temp file");
}

void usage() {
	puts("usage: rb3stats userid platform instr\n");
	puts("platform is wii, 360 or ps3");
	puts("instr is prokeys, progtr or probass or TODO");
	exit(0);
}

int main(int argc,char**argv) {
	puts("rb3 stats generator by stubbscroll in 2013\n");
	if(argc<4) usage();
	process(argv[1],argv[2],argv[3]);
	return 0;
}
