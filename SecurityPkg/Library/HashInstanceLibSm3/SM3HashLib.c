

//---------------------------------------------------------------------
#include <Library/BaseMemoryLib.h>


//---------------------------------------------------------------------
typedef struct {
  UINT32	state[8];
	UINT32	length;
	UINT32	curlen;
  UINT8   buf[64];
} SM3_STATE;

//#define RESULT_WORD 5
#define T1  0x79CC4519
#define T2  0x7A879D8AUL
#define IVA 0x7380166f
#define IVB 0x4914b2b9
#define IVC 0x172442d7
#define IVD 0xda8a0600
#define IVE 0xa96f30bc
#define IVF 0x163138aa
#define IVG 0xe38dee4d
#define IVH 0xb0fb0e4e

/* Various logical functions */
#define p1(x)   (x^rotl32(x,15)^rotl32(x,23))
#define p0(x)   (x^rotl32(x,9)^rotl32(x,17))
#define ff0(a,b,c)   (a^b^c)
#define ff1(a,b,c)   ((a&b)|(a&c)|(b&c))
#define gg0(e,f,g)   (e^f^g)
#define gg1(e,f,g)   ((e&f)|((~e)&g))
#define rotl32(x,n)   (((x) << n) | ((x) >> (32 - n)))
#define rotr32(x,n)   (((x) >> n) | ((x) << (32 - n)))




//---------------------------------------------------------------------
// Use global variable to avoid consuming too much stack size.
STATIC UINT32 gBuffer_Wj[68];
STATIC UINT32 gBuffer_Wj1[64];




//---------------------------------------------------------------------
/*
BiToWj is used to do Ext from Bi to Wj.
*/
void BiToWj(UINT32 *Bi, UINT32 *Wj)
{
  UINT32 j;
	UINT32 temp1;

// avoid link error: error LNK2001: unresolved external symbol memcpy
//	for(j=0;j<=15;j++){
//	  *Wj=*Bi;
//    Wj++;
//		Bi++;
//	}
	CopyMem(Wj, Bi, 16*sizeof(UINT32));
	Bi += 16;
	Wj += 16;
	
	for(j=16;j<=67;j++){
    temp1=(*(Wj-16))^(*(Wj-9))^rotl32(*(Wj-3),15);
		*Wj=p1(temp1)^(rotl32(*(Wj-13),7))^(*(Wj-6));
		Wj++;
	}
}

/*
BiToWj is used to calculate Wj1 from Wj.
*/
void WjToWj1(UINT32 *Wj, UINT32 *Wj1)
{
  UINT32 j;
	for(j=0;j<=63;j++){
	  *Wj1=*Wj^(*(Wj+4));
	  Wj++;
	  Wj1++;
	}
}

void Vload(UINT32 *A, UINT32 *B, UINT32 *C, UINT32 *D, 
           UINT32 *E, UINT32 *F, UINT32 *G, UINT32 *H, UINT32 *V)
{
    *V=*A^*V;
	*(V+1)=*B^(*(V+1));
	*(V+2)=*C^(*(V+2));
	*(V+3)=*D^(*(V+3));
	*(V+4)=*E^(*(V+4));
	*(V+5)=*F^(*(V+5));
	*(V+6)=*G^(*(V+6));
	*(V+7)=*H^(*(V+7));
}

void reg_init(UINT32 *A, UINT32 *B, UINT32 *C, UINT32 *D, 
              UINT32 *E, UINT32 *F, UINT32 *G, UINT32 *H, UINT32 *V)
{
	*A=*V;
	*B=*(V+1);
	*C=*(V+2);
	*D=*(V+3);
	*E=*(V+4);
	*F=*(V+5);
	*G=*(V+6);
	*H=*(V+7);
}


/*
CF is used to do CF function.
input: Wj, Wj1, V
output: next V
*/
void CF(UINT32 *Wj, UINT32 *Wj1, UINT32 *V)
{
	UINT32 SS1;
	UINT32 SS2;
	UINT32 TT1;
	UINT32 TT2;
	UINT32 A,B,C,D,E,F,G,H;
	UINT32 T=T1;
	UINT32 FF;
	UINT32 GG;
	UINT32 j;

  reg_init(&A,&B,&C,&D,&E,&F,&G,&H,&V[0]);
  
	for(j=0;j<=63;j++){
	    //SS1
		if(j==0)
		  T=T1;
		else if(j==16)
		  T=rotl32(T2,16);
		else
		  T=rotl32(T,1);
		  
    SS1=rotl32((rotl32(A,12)+E+T),7);
		//SS2
		SS2=SS1^rotl32(A,12);
		//TT1
		if(j<=15)
		  FF=ff0(A,B,C);
		else
		  FF=ff1(A,B,C);
		  
		TT1=FF+D+SS2+*Wj1;
		Wj1++;
		//TT2
		if(j<=15)
		  GG=gg0(E,F,G);
		else
		  GG=gg1(E,F,G);
		  
		TT2=GG+H+SS1+*Wj;
		Wj++;
		//D
		D=C;
		//C
		C=rotl32(B,9);
		//B
		B=A;
		//A
		A=TT1;
		//H
		H=G;
		//G
		G=rotl32(F,19);
		//F
		F=E;
		//E
		E=p0(TT2);
	}
	Vload(&A,&B,&C,&D,&E,&F,&G,&H,&V[0]);
}


VOID Sm3Init(VOID *Sm3Ctx)
{
  SM3_STATE *md;
  md = (SM3_STATE*)Sm3Ctx;

  md->curlen   = 0;
  md->length   = 0;
  md->state[0] = IVA;
  md->state[1] = IVB;
  md->state[2] = IVC;
  md->state[3] = IVD;
  md->state[4] = IVE;
  md->state[5] = IVF;
  md->state[6] = IVG;
  md->state[7] = IVH;
}

/* U32 endian converse.
 *	INPUT:
 * c[bytelen]: input buffer
 * bytelen: bytelen of c, must be multiple of 4
 *	OUTPUT:
 * a[bytelen]: output buffer
 *Note: inplace converse is permitted!
 */
void converse(UINT8 *c, UINT32 bytelen, UINT8 *a)
{
	UINT8  tmp = 0;
  UINT32 i   = 0;
    
  for(i=0; i<bytelen/4; i++){
  	tmp        = *(a+4*i);
    *(a+4*i)   = *(c+4*i+3);
    *(c+4*i+3) = tmp;
  
  	tmp        = *(a+4*i+1);
    *(a+4*i+1) = *(c+4*i+2);
    *(a+4*i+2) = tmp;
  }
}


void SM3_compress(SM3_STATE *md)
{
//UINT32 Wj[68];
//UINT32 Wj1[64];
	converse(md->buf, 64, md->buf);
	BiToWj((UINT32*)md->buf, gBuffer_Wj);
	WjToWj1(gBuffer_Wj,gBuffer_Wj1);
	CF(gBuffer_Wj, gBuffer_Wj1, md->state);	
}

VOID Sm3Update(VOID *Sm3Ctx, CONST UINT8 *DataToHash, UINTN DataToHashLen)
{
  SM3_STATE *md;
  md = (SM3_STATE*)Sm3Ctx;

  while (DataToHashLen--){
    /* copy byte */
    md->buf[md->curlen] = *DataToHash++;
    md->curlen++; 
    /* is 64 bytes full? */
    if(md->curlen == 64){
      SM3_compress(md);
      md->length += 512;
      md->curlen = 0;
    }
  }
}

VOID Sm3Final(VOID *Sm3Ctx, UINT8 *Digist)
{
  UINT32     i;
  SM3_STATE  *md;

  md = (SM3_STATE*)Sm3Ctx;
  
  /* increase the length of the message */
  md->length += md->curlen <<3;

  /* append the '1' bit */
  *((UINT8*)md->buf+md->curlen) = 0x80;
  md->curlen++;
  /* if the length is currenlly above 56 bytes we append zeros
   * then compress.  Then we can fall back to padding zeros and length
   * encoding like normal.
   */
  if (md->curlen >56){
    while(md->curlen < 64){
	    *((UINT8*)md->buf+md->curlen) = 0;
		  md->curlen++;
    }
	  SM3_compress(md);
    md->curlen = 0;
  }

  /* pad upto 56 bytes of zeroes */
  while(md->curlen < 56){
		*((UINT8*)md->buf+md->curlen) = 0;
		md->curlen++;
  }

/* since all messages are under 2^32 bits we mark the top bits zero -- done by the front step*/
  for(i = 56; i < 60; i++){
    md->buf[i] = 0;
  }
	
	md->buf[63] = (UINT8)(md->length & 0xff);
	md->buf[62] = (UINT8)((md->length >> 8) & 0xff);
	md->buf[61] = (UINT8)((md->length >> 16) & 0xff);
	md->buf[60] = (UINT8)((md->length >> 24) & 0xff);	

  SM3_compress(md);
	
	converse((UINT8*)md->state, 32, (UINT8*)md->state);

  CopyMem(Digist, md->state, 32);
}


UINTN Sm3GetContextSize()
{
  return sizeof(SM3_STATE);
}

/*
EFI_STATUS 
EFIAPI 
SM3Hash(
  CONST UINT8 *Data, 
  UINTN DataLength, 
  UINT8 *Hash
  )
{
//SM3_STATE md;
  Sm3Init(&gSm3State);
  SM3_process(&gSm3State, Data, DataLength);
  SM3_done(&gSm3State, Hash);
  
  return EFI_SUCCESS;
}
*/



