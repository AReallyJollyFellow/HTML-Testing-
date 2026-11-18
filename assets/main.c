/* FORSKN3D - 3D asymmetric chase survival for TI-84 Plus CE (CEdev / C)
 * Integer-only 3D: 6-bit trig table, per-box rigs, backface culling,
 * painter's sort, flat shading from a 52-entry custom palette. */
#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "gfx/gfx.h"

#define F 170
#define CY0 100
#define NEAR 6
#define CAMD 55
#define CAMH 30
#define MAXQ 48
#define NPIL 14
#define GENREQ 240
#define STAMMAX 150
#define COL(b,s) ((b)*4+(s))

static int8_t sintab[256];
#define S(a) ((int)sintab[(uint8_t)(a)])
#define C(a) ((int)sintab[(uint8_t)((a)+64)])

/* ---------- rigs ---------- */
typedef struct { int16_t cx,cy,cz,hx,hy,hz,px,py,pz; uint8_t col, ch; } Part;
/* ch: 0 static, 1 L arm, 2 R arm, 3 L leg, 4 R leg */
static const Part SURV[6] = {
 {0,18,0,5,6,3,0,18,0,2,0},   {0,28,0,4,4,4,0,24,0,1,0},
 {-7,18,0,2,6,2,-7,24,0,1,1}, {7,18,0,2,6,2,7,24,0,1,2},
 {-3,6,0,2,6,2,-3,12,0,3,3},  {3,6,0,2,6,2,3,12,0,3,4}};
static const Part KILL[9] = {
 {0,20,0,7,7,4,0,20,0,11,0},   {0,33,0,5,5,5,0,27,0,10,0},
 {0,33,5,3,4,1,0,27,0,7,0},    {0,20,3,3,5,1,0,20,0,2,0},
 {-10,20,0,3,7,3,-10,27,0,10,1},
 {10,20,0,3,7,3,10,27,0,10,2}, {-4,7,0,3,7,3,-4,14,0,4,3},
 {4,7,0,3,7,3,4,14,0,4,4},     {10,14,7,1,2,8,10,27,0,6,2}};

/* ---------- render queue ---------- */
typedef struct { int depth; uint8_t col; int16_t x[8], y[8], z[8]; } Box;
static Box q[MAXQ]; static uint8_t order[MAXQ]; static int nq;
static int camx, camy, camz, csn, ccs;
static const uint8_t FACE[6][4] = {{0,1,3,2},{5,4,6,7},{4,0,2,6},{1,5,7,3},{2,3,7,6},{1,0,4,5}};
static const uint8_t FSH[6] = {2,1,1,2,3,0};

static int cl(int v){ return v<-2000?-2000:(v>2000?2000:v); }

static void emit_part(const Part *p, int pitch, int tilt, int wx, int wz, uint8_t yaw, uint8_t col){
 if(nq>=MAXQ) return;
 Box *b=&q[nq];
 int ps=S(pitch), pcs=C(pitch), ts=S(tilt), tc=C(tilt), ys=S(yaw), yc=C(yaw);
 int zsum=0, nf=0, mnx=9999, mxx=-9999, mny=9999, mxy=-9999;
 for(int i=0;i<8;i++){
  int x=((i&1)?p->hx:-p->hx)+p->cx-p->px;
  int y=((i&2)?p->hy:-p->hy)+p->cy-p->py;
  int z=((i&4)?p->hz:-p->hz)+p->cz-p->pz;
  if(pitch){ int t=(y*pcs-z*ps)>>6; z=(y*ps+z*pcs)>>6; y=t; }
  x+=p->px; y+=p->py; z+=p->pz;
  if(tilt){ int t=(y*tc-z*ts)>>6; z=(y*ts+z*tc)>>6; y=t; }
  int xw=((x*yc+z*ys)>>6)+wx;
  int zw=((z*yc-x*ys)>>6)+wz;
  int dx=xw-camx, dz=zw-camz, dy=y-camy;
  int vx=(dx*ccs-dz*csn)>>6;
  int vz=(dx*csn+dz*ccs)>>6;
  b->z[i]=vz; zsum+=vz;
  if(vz>=NEAR){
   int sx=cl(160+vx*F/vz), sy=cl(CY0-dy*F/vz);
   b->x[i]=sx; b->y[i]=sy; nf++;
   if(sx<mnx)mnx=sx; if(sx>mxx)mxx=sx; if(sy<mny)mny=sy; if(sy>mxy)mxy=sy;
  }
 }
 if(!nf) return;
 if(nf==8 && (mxx<0||mnx>319||mxy<0||mny>239)) return;
 b->depth=zsum>>3; b->col=col; order[nq]=nq; nq++;
}

static void emit_static(int x,int z,int hx,int hy,int hz,int yoff,uint8_t col){
 Part p={0,(int16_t)(yoff+hy),0,(int16_t)hx,(int16_t)hy,(int16_t)hz,0,(int16_t)(yoff+hy),0,col,0};
 emit_part(&p,0,0,x,z,0,col);
}

static void emit_rig(const Part *r,int n,const int *pc,int tilt,int wx,int wz,uint8_t yaw,int flash,int rage){
 for(int i=0;i<n;i++){
  uint8_t c=r[i].col;
  if(flash) c=5; else if(c==7 && rage) c=5;
  emit_part(&r[i], r[i].ch?pc[r[i].ch]:0, tilt, wx, wz, yaw, c);
 }
}

static void draw_queue(void){
 for(int i=1;i<nq;i++){ uint8_t k=order[i]; int d=q[k].depth; int j=i-1;
  while(j>=0 && q[order[j]].depth<d){ order[j+1]=order[j]; j--; } order[j+1]=k; }
 for(int n=0;n<nq;n++){
  Box *b=&q[order[n]];
  for(int f=0;f<6;f++){
   const uint8_t *v=FACE[f];
   if(b->z[v[0]]<NEAR||b->z[v[1]]<NEAR||b->z[v[2]]<NEAR||b->z[v[3]]<NEAR) continue;
   int32_t cr=(int32_t)(b->x[v[1]]-b->x[v[0]])*(b->y[v[2]]-b->y[v[1]])
             -(int32_t)(b->y[v[1]]-b->y[v[0]])*(b->x[v[2]]-b->x[v[1]]);
   if(cr>=0) continue;
   gfx_SetColor(COL(b->col,FSH[f]));
   gfx_FillTriangle(b->x[v[0]],b->y[v[0]],b->x[v[1]],b->y[v[1]],b->x[v[2]],b->y[v[2]]);
   gfx_FillTriangle(b->x[v[0]],b->y[v[0]],b->x[v[2]],b->y[v[2]],b->x[v[3]],b->y[v[3]]);
  }
 }
}

/* ---------- level ---------- */
static const int gx[3]={-150,140,-40}, gz[3]={-120,-20,170};
#define GATEZ 285
static int ox[NPIL], oz[NPIL], ohy[NPIL];
static uint32_t rs;
static int rnd(int n){ rs=rs*1103515245u+12345u; return (int)((rs>>16)%(uint32_t)n); }

static void build_level(void){
 rs=12345;
 for(int i=0;i<NPIL;i++){
  int ok;
  do{
   ok=1; ox[i]=rnd(500)-250; oz[i]=rnd(460)-230;
   for(int g=0;g<3;g++) if(abs(ox[i]-gx[g])<45&&abs(oz[i]-gz[g])<45) ok=0;
   if(abs(ox[i])<45&&oz[i]>-290&&oz[i]<-200) ok=0;
   if(abs(ox[i])<50&&oz[i]>230) ok=0;
  }while(!ok);
  ohy[i]=rnd(14)+16;
 }
}
static int blocked(int x,int z){
 for(int i=0;i<NPIL;i++) if(abs(x-ox[i])<12&&abs(z-oz[i])<12) return 1;
 for(int g=0;g<3;g++) if(abs(x-gx[g])<13&&abs(z-gz[g])<11) return 1;
 return 0;
}
static void move_ent(int *p16x,int *p16z,int dx,int dz){
 int nx=*p16x+dx, nz=*p16z+dz;
 if(!blocked(nx>>4,*p16z>>4)) *p16x=nx;
 if(!blocked(*p16x>>4,nz>>4)) *p16z=nz;
 if(*p16x>290*16)*p16x=290*16; if(*p16x<-290*16)*p16x=-290*16;
 if(*p16z>295*16)*p16z=295*16; if(*p16z<-290*16)*p16z=-290*16;
}

/* ---------- game state ---------- */
static int game, tick;
static int px,pz,kx,kz,gprog[3];
static uint8_t pyaw,kyaw,pph,kph;
static int hp,stam,tired,hurt,inv,bleed,deadt,pstate;
static int katk,katype,khit,kacd,kbeheadcd,kgashcd,krage,kragecd,kmove;
static int cola,cola_cd,slate,slate_cd,burger,burger_cd,shield,paction,paction_t,helpless,kstun;
static uint8_t ability_keys;

static int gens_done(void){ int n=0; for(int i=0;i<3;i++) if(gprog[i]>=GENREQ) n++; return n; }

static void reset(void){
 build_level();
 px=0; pz=-250*16; pyaw=0; pph=0; hp=100; stam=STAMMAX; tired=0; hurt=0; inv=0; bleed=0; deadt=0; pstate=0;
 cola=cola_cd=slate=slate_cd=burger=burger_cd=shield=paction=paction_t=helpless=kstun=0;
 kx=100*16; kz=150*16; kyaw=128; kph=0; katk=0; katype=khit=0; kacd=60; kbeheadcd=300; kgashcd=600; krage=0; kragecd=180; kmove=0;
 for(int i=0;i<3;i++) gprog[i]=0;
 game=1; tick=0;
}

static void update(void){
 kb_Scan();
 uint8_t current_ability_keys=(kb_IsDown(kb_Key7)?1:0)|(kb_IsDown(kb_Key8)?2:0)|(kb_IsDown(kb_Key9)?4:0);
 uint8_t pressed_ability_keys=current_ability_keys&~ability_keys;
 ability_keys=current_ability_keys;
 if(game!=1){
  if(game==3 && deadt<30) deadt++;
  if(kb_IsDown(kb_KeyEnter)&&(game==2||deadt>=30)) reset();
  return;
 }
 tick++;
 if(cola>0) cola--; if(cola_cd>0) cola_cd--;
 if(slate>0 && --slate==0){ shield=0; slate_cd=1320; }
 if(slate_cd>0) slate_cd--;
 if(burger>0) burger--; if(burger_cd>0) burger_cd--;
 if(kbeheadcd>0) kbeheadcd--; if(kgashcd>0) kgashcd--;
 if(helpless>0) helpless--;
 /* ---- survivor ---- */
 int use_cola=(pressed_ability_keys&1)!=0, use_slate=(pressed_ability_keys&2)!=0;
 int use_burger=(pressed_ability_keys&4)!=0;
 if(!paction_t){
  if(use_cola && !cola_cd){ cola=210; cola_cd=1500; paction=1; paction_t=75; }
  else if(use_slate && !slate_cd){
   if(slate){
    slate=0; shield=0; slate_cd=1320; kstun=45; katk=0; katype=0; khit=0;
    move_ent(&kx,&kz,-(S(kyaw)*24>>6),-(C(kyaw)*24>>6));
   } else { slate=180; shield=40; slate_cd=75; }
   paction=2; paction_t=45;
  } else if(use_burger && !burger_cd){
   burger=210; burger_cd=1200; paction=3; paction_t=60;
   katk=0; katype=0; khit=0;
  }
 }
 if(paction_t>0) paction_t--;
 int fwd=kb_IsDown(kb_KeyUp), back=kb_IsDown(kb_KeyDown), running=0, rep=-1;
 if(kb_IsDown(kb_KeyLeft)) pyaw-=5;
 if(kb_IsDown(kb_KeyRight)) pyaw+=5;
 for(int g=0;g<3;g++){
  int dx=(px>>4)-gx[g], dz=(pz>>4)-gz[g];
  if(gprog[g]<GENREQ && dx*dx+dz*dz<26*26) rep=g;
 }
 pstate=0;
 if(paction_t){ pstate=5+paction; pph+=16; }
 else if(helpless){ pstate=4; }
 else if(rep>=0 && kb_IsDown(kb_KeyEnter) && !fwd && !back){ gprog[rep]++; pstate=3; pph+=40; }
 else if(fwd||back){
  int sp=20;
  if(cola) sp=sp*5/4;
  if(slate) sp=sp*3/4;
  if(burger) sp=sp*4/5;
  if(fwd && kb_IsDown(kb_Key2nd) && stam>0 && !tired && !paction_t){ sp=sp*9/5; running=1; }
  if(back) sp=10;
  if(hurt>0) sp=sp*3/4;
  int d=fwd?1:-1;
  move_ent(&px,&pz,(S(pyaw)*sp*d)>>6,(C(pyaw)*sp*d)>>6);
  pstate=running?2:1; pph+=running?24:14;
 } else pph+=3;
 if(running){ if(--stam<=0){ stam=0; tired=1; } }
 else { if((tick&1)&&stam<STAMMAX) stam++; if(tired&&stam>=40) tired=0; }
 if(hurt>0){ hurt--; if(pstate==0||pstate==3) pstate=4; }
 if(inv>0) inv--;
 if(bleed>0){ bleed--; if(bleed%18==0) hp--; }
 if(burger && hp<100 && tick%16==0) hp++;
 /* ---- killer AI ---- */
 int tx=(px-kx)>>4, tz=(pz-kz)>>4, d2=tx*tx+tz*tz;
 int dr=tx*C(kyaw)-tz*S(kyaw), df=tx*S(kyaw)+tz*C(kyaw);
 if(kacd>0) kacd--; if(kragecd>0) kragecd--;
 if(krage>0) krage--;
 if(kstun>0) kstun--;
 if(burger||kstun){ kmove=0; }
 else if(!katk){
  if(dr>192) kyaw+=5; else if(dr<-192||df<0) kyaw-=5;
  if(!krage && !kragecd && d2>80*80 && d2<260*260){ krage=420; kragecd=900; }
  if(d2>20*20){ int sp=krage?40:24; kmove=1; move_ent(&kx,&kz,(S(kyaw)*sp)>>6,(C(kyaw)*sp)>>6); kph+=krage?26:15; }
  else { kmove=0; kph+=3; }
  if(d2<42*42 && df>0){
   if(!kgashcd && d2<34*34){ katype=3; katk=1; khit=0; kgashcd=krage?720:960; }
   else if(!kbeheadcd && d2<34*34){ katype=2; katk=1; khit=0; kbeheadcd=krage?360:540; }
   else if(!kacd && d2<34*34){ katype=1; katk=1; khit=0; }
  }
 } else {
  int hitframe=katype==1?7:(katype==2?14:0);
  int duration=katype==1?18:(katype==2?27:48);
  if(krage && katk==4 && katype!=3)
   move_ent(&kx,&kz,(S(kyaw)*12)>>6,(C(kyaw)*12)>>6);
  if(katype==1 && katk==hitframe && d2<38*38 && df>0 && !inv && !khit){
   int damage=krage?25:20;
   if(slate && shield){ int blocked=(damage+1)/2; if(blocked>shield) blocked=shield; shield-=blocked; damage-=blocked; }
   hp-=damage; bleed=150; hurt=20; inv=40; khit=1;
  } else if(katype==2 && katk==hitframe && d2<42*42 && df>0 && !inv && !khit){
   int damage=krage?35:25;
   if(slate && shield){ int blocked=(damage+1)/2; if(blocked>shield) blocked=shield; shield-=blocked; damage-=blocked; }
   hp-=damage; helpless=45; hurt=20; inv=40; khit=1;
  } else if(katype==3 && (katk==10||katk==16||katk==22||katk==28||katk==34||katk==40)
            && d2<42*42 && df>0){
   static const uint8_t gash_damage[6]={10,10,10,10,5,5};
   int damage=gash_damage[khit<6?khit:5];
   if(krage) damage=damage*7/5;
   if(slate && shield){ int blocked=(damage+1)/2; if(blocked>shield) blocked=shield; shield-=blocked; damage-=blocked; }
   hp-=damage; hurt=20; inv=10; khit++;
  }
  if(++katk>duration){ katk=0; katype=0; kacd=krage?24:57; }
 }
 if(hp<=0){ hp=0; game=3; deadt=0; pstate=5; return; }
 if(gens_done()==3){ int dx=(px>>4), dz=(pz>>4)-GATEZ; if(abs(dx)<24&&abs(dz)<20) game=2; }
}

/* ---------- animation ---------- */
static void surv_pose(int *pc,int *tilt){
 int s=S(pph);
 pc[1]=pc[2]=pc[3]=pc[4]=0; *tilt=0;
 switch(pstate){
  case 0: pc[1]=s>>3; pc[2]=-(s>>3); break;
  case 1: pc[3]=(s*24)>>6; pc[4]=-pc[3]; pc[1]=-pc[3]; pc[2]=pc[3]; break;
  case 2: pc[3]=(s*44)>>6; pc[4]=-pc[3]; pc[1]=(s*50)>>6; pc[2]=-pc[1]; *tilt=10; break;
  case 3: pc[1]=-50+(s>>3); pc[2]=-50-(s>>3); *tilt=6; break;
  case 4: pc[1]=pc[2]=-75; *tilt=-12; break;
  case 6: pc[1]=-48+(s>>4); pc[2]=16; *tilt=2; break;
  case 7: pc[1]=-82; pc[2]=-25; *tilt=4; break;
  case 8: pc[1]=-60+(s>>4); pc[2]=24; *tilt=-3; break;
  default: *tilt=deadt*2+(deadt>0?4:0); if(*tilt>64)*tilt=64; pc[1]=pc[2]=-20; break;
 }
}
static void kill_pose(int *pc,int *tilt){
 int s=S(kph);
 pc[1]=pc[2]=pc[3]=pc[4]=0; *tilt=0;
 if(katk){
  int f=katk-1, a;
  if(katype==1){
   if(f<6) a=60*f/6; else if(f<11) a=60-(150*(f-6)/5); else a=-90+(45*(f-11)/7);
   pc[2]=a; pc[1]=-a/2; *tilt=(f>=6&&f<11)?12:0;
  } else if(katype==2){
   if(f<14) a=100*f/14; else if(f<20) a=100-(180*(f-14)/6); else a=-80;
   pc[2]=a; pc[1]=-a/2; *tilt=(f>=14&&f<20)?18:0;
  } else {
   int phase=f%12;
   a=phase<6?100*phase/6:100-(160*(phase-6)/6);
   pc[1]=a; pc[2]=-a; *tilt=(f>34)?-20:8;
   if(f>40){ pc[3]=65; pc[4]=-65; *tilt=22; }
  }
 } else if(krage){
  pc[3]=(s*44)>>6; pc[4]=-pc[3]; pc[1]=pc[3]; pc[2]=-30-(pc[3]>>2); *tilt=14;
 } else if(kmove){
  pc[3]=(s*26)>>6; pc[4]=-pc[3]; pc[1]=-pc[3]; pc[2]=-30+(pc[3]>>2); *tilt=4;
 } else { pc[1]=s>>4; pc[2]=-30-(s>>4); }
}

/* ---------- render ---------- */
static int project_world(int wx,int wy,int wz,int *sx,int *sy,int *depth){
 int dx=wx-camx, dz=wz-camz, dy=wy-camy;
 int vx=(dx*ccs-dz*csn)>>6, vz=(dx*csn+dz*ccs)>>6;
 if(vz<NEAR) return 0;
 *sx=160+vx*F/vz; *sy=CY0-dy*F/vz; *depth=vz;
 return *sx>=-24&&*sx<344&&*sy>=-24&&*sy<264;
}

static int faces_camera(int wx,int wz,uint8_t yaw){
 return ((camx-wx)*S(yaw)+(camz-wz)*C(yaw))>0;
}

static void draw_world_sprite(const gfx_sprite_t *sprite,int x,int y,int z,
                              uint8_t yaw,int forward){
 int wx=x+((S(yaw)*forward)>>6), wz=z+((C(yaw)*forward)>>6);
 int sx,sy,depth;
 if(project_world(wx,y,wz,&sx,&sy,&depth))
  gfx_TransparentSprite(sprite,sx-sprite->width/2,sy-sprite->height/2);
}

static void character_textures(void){
 int pxw=px>>4, pzw=pz>>4, kxw=kx>>4, kzw=kz>>4;
 if(faces_camera(kxw,kzw,kyaw)){
  draw_world_sprite(slasher_hood,kxw,37,kzw,kyaw,4);
  draw_world_sprite(slasher_mask,kxw,33,kzw,kyaw,6);
  draw_world_sprite(machete,kxw+((S(kyaw)*8)>>6),14,
                    kzw+((C(kyaw)*8)>>6),kyaw,2);
 }
 if(faces_camera(pxw,pzw,pyaw))
  draw_world_sprite(noob_face,pxw,28,pzw,pyaw,5);
}

static void hud(void){
 gfx_SetTextScale(1,1); gfx_SetTextFGColor(COL(9,3));
 gfx_SetColor(COL(10,1)); gfx_FillRectangle(3,3,104,9); gfx_FillRectangle(3,14,104,6);
 gfx_SetColor(COL(5,3)); gfx_FillRectangle(5,5,hp,5);
 gfx_SetColor(tired?COL(5,1):COL(12,3)); gfx_FillRectangle(5,15,stam*100/STAMMAX,3);
 gfx_PrintStringXY("GENS",4,24); gfx_PrintInt(gens_done(),1); gfx_PrintString("/3");
 if(krage) { gfx_SetTextFGColor(COL(5,3)); gfx_PrintStringXY("RAGING PACE",4,34); gfx_SetTextFGColor(COL(9,3)); }
 if(bleed) gfx_PrintStringXY("BLEEDING",4,44);
 gfx_SetTextFGColor(cola_cd?COL(6,2):COL(12,3)); gfx_PrintStringXY("7 COLA",4,56);
 gfx_SetTextFGColor(slate_cd?COL(6,2):COL(3,3)); gfx_PrintStringXY("8 SLATE",48,56);
 gfx_SetTextFGColor(burger_cd?COL(6,2):COL(9,3)); gfx_PrintStringXY("9 GHOST",100,56);
 gfx_SetTextFGColor(COL(9,3));
 if(slate) {  gfx_SetColor(COL(6,3)); gfx_FillRectangle(5,66,shield,2); }
 gfx_SetColor(COL(10,1)); gfx_FillRectangle(172,2,91,42);
 gfx_TransparentSprite(slash_icon,174,3);
 gfx_TransparentSprite(behead_icon,196,3);
 gfx_TransparentSprite(gashing_icon,218,3);
 gfx_TransparentSprite(rage_icon,240,3);
 gfx_SetTextFGColor(COL(9,3)); gfx_PrintStringXY("M1 Q E R",177,27);
 if(kacd){ gfx_SetColor(COL(6,2)); gfx_FillRectangle(174,25,22,2); }
 if(kbeheadcd){ gfx_SetColor(COL(6,2)); gfx_FillRectangle(196,25,22,2); }
 if(kgashcd){ gfx_SetColor(COL(6,2)); gfx_FillRectangle(218,25,22,2); }
 if(kragecd){ gfx_SetColor(COL(6,2)); gfx_FillRectangle(240,25,22,2); }
 for(int g=0;g<3;g++) if(gprog[g]<GENREQ){
  int dx=(px>>4)-gx[g], dz=(pz>>4)-gz[g];
  if(dx*dx+dz*dz<26*26 && gprog[g]>0){ gfx_SetColor(COL(10,1)); gfx_FillRectangle(110,226,100,8);
   gfx_SetColor(COL(12,3)); gfx_FillRectangle(112,228,gprog[g]*96/GENREQ,4); }
  if(dx*dx+dz*dz<26*26) gfx_PrintStringXY("HOLD ENTER",125,214);
 }
 if(gens_done()==3) gfx_PrintStringXY("GATE OPEN - RUN N",90,4);
 /* minimap */
 int x0=266,y0=4;
 gfx_SetColor(COL(10,1)); gfx_FillRectangle(x0,y0,50,50);
 #define MX(w) (x0+1+(((w)+300)*48/600))
 #define MY(w) (y0+49-(((w)+300)*48/600))
 for(int g=0;g<3;g++){ gfx_SetColor(gprog[g]>=GENREQ?COL(3,3):COL(12,3)); gfx_FillRectangle(MX(gx[g])-1,MY(gz[g])-1,3,3); }
 gfx_SetColor(gens_done()==3?COL(3,3):COL(6,2)); gfx_FillRectangle(MX(0)-2,MY(GATEZ),5,2);
 gfx_SetColor(COL(5,3)); gfx_FillRectangle(MX(kx>>4)-1,MY(kz>>4)-1,3,3);
 gfx_SetColor(COL(9,3)); gfx_FillRectangle(MX(px>>4)-1,MY(pz>>4)-1,3,3);
}

static void render(void){
 camx=(px>>4)-((S(pyaw)*CAMD)>>6); camz=(pz>>4)-((C(pyaw)*CAMD)>>6); camy=CAMH;
 csn=S(pyaw); ccs=C(pyaw);
 gfx_SetColor(COL(10,1)); gfx_FillRectangle(0,0,320,CY0);
 gfx_SetColor(COL(11,1)); gfx_FillRectangle(0,CY0,320,240-CY0);
 gfx_SetColor(COL(11,3));
 int bx=(camx/40)*40, bz=(camz/40)*40;
 for(int i=-5;i<=6;i++) for(int j=-5;j<=6;j++){
  int dx=bx+i*40-camx, dz=bz+j*40-camz;
  int vz=(dx*csn+dz*ccs)>>6; if(vz<NEAR) continue;
  int vx=(dx*ccs-dz*csn)>>6;
  int sx=160+vx*F/vz, sy=CY0+camy*F/vz;
  if(sx>=0&&sx<319&&sy>=0&&sy<239) gfx_FillRectangle(sx,sy,vz<150?2:1,vz<150?2:1);
 }
 nq=0;
 for(int i=0;i<NPIL;i++) emit_static(ox[i],oz[i],7,ohy[i],7,0,8);
 for(int g=0;g<3;g++){
  emit_static(gx[g],gz[g],9,7,7,0,12);
  emit_static(gx[g],gz[g],2,2,2,14,gprog[g]>=GENREQ?3:5);
 }
 { uint8_t gc=gens_done()==3?3:5;
   emit_static(-16,GATEZ,3,18,3,0,gc); emit_static(16,GATEZ,3,18,3,0,gc); emit_static(0,GATEZ,19,2,2,32,6); }
 int pc[5], tl;
 surv_pose(pc,&tl);
 emit_rig(SURV,6,pc,tl,px>>4,pz>>4,pyaw,(hurt>0)&&(tick&2),0);
 kill_pose(pc,&tl);
 emit_rig(KILL,9,pc,tl,kx>>4,kz>>4,kyaw,0,krage);
 draw_queue();
 character_textures();
 hud();
 if(game==2){ gfx_SetTextScale(3,3); gfx_SetTextFGColor(COL(3,3)); gfx_PrintStringXY("ESCAPED!",72,100);
  gfx_SetTextScale(1,1); gfx_PrintStringXY("ENTER: play again",100,140); }
 if(game==3){ gfx_SetTextScale(3,3); gfx_SetTextFGColor(COL(5,3)); gfx_PrintStringXY("ELIMINATED",40,100);
  gfx_SetTextScale(1,1); gfx_PrintStringXY("ENTER: retry",115,140); }
}

static void intro(void){
 gfx_FillScreen(COL(0,0));
 gfx_TransparentSprite(intro_banner,50,18);
 gfx_TransparentSprite(noob_face,42,138);
 gfx_TransparentSprite(slasher_mask,258,138);
 gfx_SetTextScale(1,1); gfx_SetTextFGColor(COL(6,2));
 gfx_PrintStringXY("NOOB: survivor",78,144);
 gfx_PrintStringXY("Repair 3 generators, then escape.",52,174);
 gfx_PrintStringXY("Arrows move  2nd sprint  Enter repair",52,191);
 gfx_PrintStringXY("7 cola  8 slateskin  9 ghostburger",52,207);
 gfx_PrintStringXY("Clear quits",128,223);
 gfx_SwapDraw(); delay(2000);
}

int main(void){
 for(int i=0;i<256;i++) sintab[i]=(int8_t)lroundf(64.0f*sinf(6.2831853f*(float)i/256.0f));
 gfx_Begin();
 gfx_SetPalette(global_palette,sizeof_global_palette,0);
 gfx_SetTransparentColor(0);
 gfx_SetDrawBuffer();
 intro();
 reset();
 timer_Set(1,0); timer_Enable(1,TIMER_32K,TIMER_NOINT,TIMER_UP);
 while(!kb_IsDown(kb_KeyClear)){
  update(); render(); gfx_SwapDraw();
  while(timer_Get(1)<1092) {}
  timer_Set(1,0);
 }
 gfx_End();
 return 0;
}
