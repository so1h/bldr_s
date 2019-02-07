/***************************************************************************/
/*                                 bldr_s.c                                */
/***************************************************************************/
/* 3. PXE APIs (PXE Specification pag 39): PreBoot API, TFTP, UDP & UNDI   */
/* 3.1 PXE Installation Check (PXE Specification pag 40)                   */
/* 4.4.5 Client State at Bootstrap Execution Time (Remote.0)               */
/* ES:BX --> PXENV+ (pág 41), SS:[SP + 4] --> !PXE (pag 43)                */
/***************************************************************************/

#include <inttypes.h>                         /* uint8_t, uint16_t, uint32 */

#include <stdio.h>                                              /* sprintf */

/* 2.4.9.1 NBP Execution for x86 PC/AT (PXE Specification pag 31)          */

void startBin ( void ) ;                
asm
(
	"  section .text      \n"
	"  global _startBin   \n"
	"_startBin:           \n"      /* true but not used: CS:IP = 0000:7C00 */

	"  mov bp,sp          \n"                  /* to find !PXE   SS:[SP+4] */
	"  mov eax,[ss:bp+4]  \n"
    "  movzx ecx,ax       \n"                         /* ecx = 0000 offset */
	"  shr eax,16         \n"                        /* eax = 0000 segment */  
	"  shl eax,4          \n"
    "  add eax,ecx        \n"                   /* eax = 16*segment+offset */
    "  push eax           \n"

	"  xor eax,eax        \n"                    /* to find PXENV+   ES:BX */
	"  mov ax,es          \n"   
	"  shl eax,4          \n"
	"  movzx ebx,bx       \n"
    "  add eax,ebx        \n"                            /* eax = 16*ES+BX */
    "  push eax           \n"

#ifndef __UNREAL__
    "  mov eax,0xE0E0E0E0 \n"                 /* makes fill return address */
    "  push eax           \n"
#endif

    "  extern __start     \n"                   /* entry point of c0du.asm */
	"  call .labnext      \n" 
    ".labnext:            \n"       /* the .labnext offset is on the stack */
    "  xor ebx,ebx        \n"   
    "  mov bx,cs          \n"
    "  shl ebx,4          \n"                               /* ebx = 16*cs */
    "  xor eax,eax        \n"
    "  pop ax             \n"                     /* eax = .labnext offset */
    "  add ebx,eax        \n"
    "  sub ebx,.labnext   \n"               /* ebx = base physical address */
	"  add ebx,__start    \n"               /* _start makes the relocation */
    "  ror ebx,4          \n"                              /* see c0dh.asm */
    "  push bx            \n"                          /* CS = _start >> 4 */
    "  shr ebx,28         \n"                      
    "  push bx            \n"                      /* IP = _start & 0x000F */
    "  retf               \n"                           /* "jump" to CS:IP */
) ;

void putcharBIOS ( char car )
{
    asm
    (
        "  mov al,[bp+8] \n"
        "  mov ah,0eh    \n" 
        "  int 10h       \n"  
    ) ;
}

void printstrBIOS ( const char * str )
{
    char car ;
    while ((car = *str++) != '\0')
    {
        if (car == '\n') putcharBIOS('\r') ;
        putcharBIOS(car) ;
    }
}

void __setup_unreal ( void ) ;      /* to reenable unreal mode if necesary */

//void __start__ ( void ) {
void __start__ ( char * ptrPXENV, char * ptrPXE ) {

    char buf [ 512 ] ;
	
	sprintf(buf, 
	    "\n"
        " Hello world from Network Boot Program \n"
	    "\n"
	    " ptrPXENV->Signature   = \"%6.6s\" \n"
	    "\n"
	    " ptrPXE  ->Signature   = \"%4.4s\" \n"
	    "\n"
	    " BIOS date = %8.8s %c \n"
		"\n",
   	    ptrPXENV, 
		ptrPXE,
		(char *)0xFFFFFFF5,
		*((char *)0xFFFFFFF5)	
	) ;

    printstrBIOS(buf) ;

    for ( ; ; ) ; 
	
}