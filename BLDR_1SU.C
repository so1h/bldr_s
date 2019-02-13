/***************************************************************************/
/*                                bldr_1su.c                               */
/***************************************************************************/
/*                                                                         */
/*                Programa mínimo que se pone en modo unreal               */
/*                  y que puede ser arrancado con boot.bs                  */ 
/*            Tambien funciona como exe y como com (dosbox, msdos)         */
/*                     (ren bldr_0su.bin bldr_0su.com)                     */
/*                    Incluso puede arrancarse con PXE                     */
/*                      (ren bldr_0su.bin bldr_0su.0)                      */
/*                                                                         */
/***************************************************************************/

/* 3. PXE APIs (PXE Specification pág 39): PreBoot API, TFTP, UDP y UNDI   */
/* 3.1 PXE Installation Check (PXE Specification pág 40)                   */
/* 4.4.5 Client State at Bootstrap Execution Time (Remote.0)               */
/* ES:BX --> PXENV+ (pág 41), SS:[SP + 4] --> !PXE (pág 43)                */

#include <inttypes.h>                         /* uint8_t, uint16_t, uint32 */
#include <string.h>                                     /* strncmp, strchr */
#include <stdio.h>                                             /* vsprintf */
#include <stdarg.h>                           /* va_list, va_start, va_end */

typedef enum { FALSE, TRUE } bool_t ;

typedef uint8_t * pointer_t ;

#define FILE_LICENCE void __nada__ 
#define GPL2_OR_LATER_OR_UBDL void

#include "qemu/pxe_types.h"
#include "qemu/pxe_api.h"        /* PXENV_GET_CACHED_INFO, PXENV_TFTP_OPEN */

pointer_t g_ptr = 0x0FFFFFF ;            /* 16 megas - 1 (dosbox Ok)       */
//pointer_t g_ptr = 0x1000000 ;          /* 16 megas     (dosbox No)       */
//pointer_t g_ptr = 0x1FFFFFF ;          /* 32 megas - 1 (msdos Takeda Ok) */
//pointer_t g_ptr = 0x2000000 ;          /* 32 megas     (msdos Takeda No) */

/* 2.4.9.1 NBP Execution for x86 PC/AT (PXE Specification pág 31)          */

void startBin ( void ) ;                
asm
(
	"  section .text      \n"
	"  global _startBin   \n"
	"_startBin:           \n"            /* no suponemos CS:IP = 0000:7C00 */

	"  mov bp,sp          \n"           /* para localizar !PXE   SS:[SP+4] */
	"  mov eax,[ss:bp+4]  \n"
    "  movzx ecx,ax       \n"                           /* ecx = 0000 desp */
	"  shr eax,16         \n"                           /* eax = 0000 segm */  
	"  shl eax,4          \n"
    "  add eax,ecx        \n"                        /* eax = 16*segm+desp */
    "  push eax           \n"
//  "  jmp $              \n"                                     /* debug */

	"  xor eax,eax        \n"             /* para localizar PXENV+   ES:BX */
	"  mov ax,es          \n"   
	"  shl eax,4          \n"
	"  movzx ebx,bx       \n"
    "  add eax,ebx        \n"                            /* eax = 16*ES+BX */
    "  push eax           \n"
//  "  jmp $              \n" 	                                  /* debug */

#ifndef __UNREAL__
    "  mov eax,0xE0E0E0E0 \n"              /* hace de dirección de retorno */
    "  push eax           \n"
//  "  jmp $              \n"                                     /* debug */
//	"  call 0x5678:0x1234 \n"                 /* ver linea 152 de c0du.asm */
//	"  jmp 0x5678:0x1234  \n"                      /* opcodes: 0x7A y 0xEA */  
#endif

    "  extern __start     \n"         /* funcion a la que ceder el control */
	"  call labnext       \n" 
    "labnext:             \n"              /* offset de labnext en la pila */
    "  xor ebx,ebx        \n"   
    "  mov bx,cs          \n"
    "  shl ebx,4          \n"                               /* ebx = 16*cs */
    "  xor eax,eax        \n"
    "  pop ax             \n"                   /* eax = offset de labnext */
    "  add ebx,eax        \n"
    "  sub ebx,labnext    \n"               /* ebx = base physical address */
	"  add ebx,__start    \n"          /* _start es la funcion que reubica */
    "  ror ebx,4          \n"                              /* ver c0dh.asm */
    "  push bx            \n"                      /* CS = _start >> 4     */
    "  shr ebx,28         \n"                      
    "  push bx            \n"                      /* IP = _start & 0x000F */
//  "  jmp $              \n" 	                                  /* debug */
    "  retf               \n"
) ;

/* leerTeclaBIOS parece que deja permitidas las interrupciones             */

uint16_t leerTeclaBIOS ( void )                  /* bloqueante en bucle BIOS */
{
//  uint16_t w ;
    asm
    (
        "  mov ah,00h    \n"          /* Llamada al BIOS: Leer del teclado */
        "  int 16h       \n"
//      "  mov [bp-4],ax \n"  /* ah = codigo scan introducido al = caracter ASCII */
    ) ;
//  return(w) ;
}

int printCarBIOS ( char car )
{
    asm
    (
        "  mov al,[bp+8] \n"
        "  mov ah,0eh    \n" /* Llamada al BIOS: Escribir caracter por pantalla */
        "  int 10h       \n"
    ) ;
}

int printStrBIOS ( const char * str )
{
    char car ;
    int cont = 0 ;
    while ((car = *str++) != '\0')
    {
        if (car == '\n')
        {
            printCarBIOS('\r') ;
            cont++ ;
        }
        printCarBIOS(car) ;
        cont++ ;
    }
    return(cont) ;
}

int printfBIOS ( char * fmt, ... ) 
{
    va_list ap ;
    char buf [ 512 ] ;               
//  static char buf [ 512 ] ;      /* if no static, problems with vsprintf */          
	int err = 0 ;
	if (strchr(fmt, '%') == NULL) 
        err = printStrBIOS(fmt) ;
	else
	{ 
        va_start(ap, fmt) ;
        err = vsprintf(buf, fmt, ap) ;
        va_end(ap) ;                         /* no hace nada, ver stdarg.h */
        printStrBIOS(buf) ;
    }    	
	return(err) ;
}

int printStrHastaBIOS ( const char * str, uint16_t n, bool_t lleno )
{
    char car ;
    uint16_t i = 0 ;
    while ((i < n) && ((car = *str++) != (char)0))
    {
        if (car == '\n') printCarBIOS('\r') ;
        printCarBIOS(car) ;
        i++ ;
    }
    if (lleno)
        while (i < n)
        {
            printCarBIOS(' ') ;
            i++ ;
        }
    return(i) ;
}

char dig [ 17 ] = "0123456789ABCDEF" ;

#define tamStrMax 11

static char str [tamStrMax] ;

int printHexBIOS ( uint16_t num, uint16_t l )
{
    /*  char str[5] ; */
    uint16_t i, j ;
    int cont = 0 ;
    char * ptr ;
    if (l == 0) l = 1 ;
    if (num == 0)
    {
        for ( i = 0 ; i < l-1 ; i++ ) printCarBIOS('0') ;
        printCarBIOS('0') ;
        return(l) ;
    }
    ptr = &str[4] ;
    *ptr = (char)0 ;
    i = 0 ;
    while (num > 0)
    {
        str[3-i] = dig[num%16] ;
        num = num/16 ;
        i++ ;
    }
    for ( j = i ; j < l ; j++ ) printCarBIOS('0') ;
    ptr = &str[3-i+1] ;
    while (*ptr != (char)0)
    {
        printCarBIOS(*ptr) ;
        ptr++ ;
        cont++ ;
    }
    return(l + cont) ;
}

int printLHexBIOS ( uint32_t num, uint16_t l )
{
    /*  char str[9] ; */
    char * ptr ;
    uint16_t * ptrWord ;
    uint16_t i, j ;
    if (l == 0) l = 1 ;
    if (num == 0)
    {
        for ( i = 0 ; i < l-1 ; i++ ) printCarBIOS('0') ;
        printCarBIOS('0') ;
        return(l) ;
    }
    str[8] = (char)0 ;
    i = 0 ;
    ptrWord = (uint16_t *)&num ;
    while (num > 0)
    {
        str[7-i] = dig[ptrWord[0]%16] ;
        ptrWord[0] = ptrWord[0]/16 + 0x1000*(ptrWord[1]%16) ;
        ptrWord[1] = ptrWord[1]/16 ;
        i++ ;
    }
    for ( j = i ; j < l ; j++ ) printCarBIOS('0') ;
    ptr = &str[7-i+1] ;

    while (i > 0)
    {
        printCarBIOS(*ptr) ;
        ptr++ ;
        i-- ;
    }
    return(0) ;
}

#define codigoPrintDec(tamStr, tamStrM1, tamStrM2) {                         \
/*  char str[tamStr] ; */                                                    \
    uint16_t i, j ;                                                            \
    int cont = 0 ;                                                           \
    char * ptr ;                                                             \
    if (l == 0) l = 1 ;                                                      \
    if (num == 0) {                                                          \
        for ( i = 0 ; i < l-1 ; i++ ) printCarBIOS(' ') ;                    \
        printCarBIOS('0') ;                                                  \
        return(l) ;                                                          \
    }                                                                        \
    ptr = (char *)&str[tamStrM1] ;                                           \
    *ptr = (char)0 ;                                                         \
    i = 0 ;                                                                  \
    while (num > 0) {                                                        \
        str[tamStrM2-i] = dig[(uint16_t)(num%10)] ;                            \
        num = num/10 ;                                                       \
        i++ ;                                                                \
    }                                                                        \
    for ( j = i ; j < l ; j++ ) printCarBIOS(' ') ;                          \
    ptr = (char *)&str[tamStrM2-i+1] ;                                       \
    while (*ptr != (char)0) {                                                \
        printCarBIOS(*ptr) ;                                                 \
        ptr++ ;                                                              \
        cont++ ;                                                             \
    }                                                                        \
    return(l + cont) ;                                                       \
}

int printDecBIOS ( uint16_t num, uint16_t l )
{
    codigoPrintDec(6, 5, 4)
}

PXENV_t * getPXENV ( void ) 
{
	uint16_t reg_ax = 0x1234 ;
	uint16_t reg_bx = 0x5678 ;
	uint16_t reg_es = 0xABCD ;	
	uint16_t flags  = 0xF1AC ;	
//	for ( ; ; ) ;                                                 /* debug */
    asm 
	( 
	    "  mov ax,0x5650  \n"
		"  int 0x1A       \n"
        "  mov [bp- 4],ax \n"  		
        "  mov [bp- 8],bx \n"  		
        "  mov [bp-12],es \n"
        "  pushf          \n"
        "  pop ax         \n"
        "  mov [bx-16],ax \n"  		
//		"  jmp $          \n"                                     /* debug */
    ) ;  
#if 0	
	printStrBIOS("\n reg_ax = ") ; printHexBIOS(reg_ax, 4) ;
	printStrBIOS("\n reg_bx = ") ; printHexBIOS(reg_bx, 4) ;
	printStrBIOS("\n reg_es = ") ; printHexBIOS(reg_es, 4) ;
	printStrBIOS("\n flags  = ") ; printHexBIOS(flags , 4) ;
#endif	
	if ((reg_ax != 0x564E) || (flags & 0x0001)) return(NULL) ;
	
	return((PXENV_t *)((((uint32_t)reg_es) << 4) + (uint32_t)reg_bx)) ;
}

uint8_t checksum ( pointer_t addr, uint8_t length ) 
{
	uint8_t i ;
    uint8_t acum = 0x00 ;
    acum = 0x00 ;
	for ( i = 0 ; i < length ; i++ )
        acum = acum + *addr++ ;
	return(acum) ;
}	

void abortBoot ( void ) 
{
	printStrBIOS("\n\n boot abortado. Pulse una tecla ... ") ;
	leerTeclaBIOS() ;
	for ( ; ; ) ;
}

//typedef void ( * PXEAPI_t ) ( void ) ;
typedef uint16_t ( * PXEAPI_t ) ( uint16_t opcode, SEGOFF16_t fptr ) ;   /* no funciona huge */

PXEAPI_t PXEAPI ; 

/* ver PXE Specification y */
/* qemu sources: qemu-3.1.0\roms\ipxe\src\arch\x86\interface\pxe\pxe_entry.S */

uint16_t callPXEAPI ( uint16_t opcode, unsigned fptr ) 
{
	unsigned aux ;
	aux = (unsigned)PXEAPI ;    	
asm
(
    "  push dword [bp + 12] \n"   /* fptr */   
//  "  push word [bp + 14]  \n"   /* fptr.segment */   
//  "  push word [bp + 12]  \n"   /* fptr.offset */   
//	"  mov ebx,[bp + 12]    \n"                                   /* debug */
    "  push  word [bp +  8] \n"   /* opcode */
//	"  mov ax,[bp +  8]     \n"                                   /* debug */
//  "  jmp $                \n"                                   /* debug */	
    "  call far [bp - 4]    \n"   /* aux */
	"  add sp,6             \n"
	"  movzx eax,ax         \n"
) ;
}

PXENV_GET_CACHED_INFO_t desc_GET_CACHED_INFO ;
	
PXENV_TFTP_OPEN_t descr_OPEN ; 

PXENV_TFTP_CLOSE_t descr_CLOSE ; 

#define numIP(a, b, c, d) (((((d << 8) + c) << 8) + b) << 8) + a 

void printIPBIOS ( unsigned ip ) 
{
    int i ;
	for ( i = 0 ; i < 4 ; i++ ) 
	{
		printDecBIOS(0x000000FF & (ip >> (i*8)), 1) ;
		if (i < (4-1)) printCarBIOS('.') ;
	}		
}

uint32_t changeEndian ( uint32_t num ) 
{
    return((((num >> 24) & 0x000000FF) <<  0) | 
	       (((num >> 16) & 0x000000FF) <<  8) | 
	       (((num >>  8) & 0x000000FF) << 16) | 
	       (((num >>  0) & 0x000000FF) << 24)) ; 
}

void __setup_unreal ( void ) ; /* para rehabilitar el modo unreal tras las llamadas al BIOS (algunas) */

//void __start__ ( void ) {
void __start__ ( PXENV_t * ptrPXENV, PXE_t * ptrPXE ) {

    pointer_t ptrPXENV_INT1A ;
	
    SEGOFF16_t fptr ;
	
	BOOTPLAYER_t * ptrBPLAYER ;

    pointer_t l_ptr ;
	
	int i ;

    l_ptr = (pointer_t)0x0B8000 ;                /* apunta al byte 0 de la */
                                                       /* memoria de video */
#if 0
    ptrPXE ;	                                     /* pone ptrPXE en eax */
    ptrPXENV ;	                                   /* pone ptrPXENV en eax */
//  for ( ; ; ) ;                                                 /* debug */
#endif

    printfBIOS(
	    "\n"
		" Hello from Network Boot Program. Pulse una tecla ... \n"
	) ;

	leerTeclaBIOS() ;
	
	__setup_unreal() ;
	
	ptrPXENV_INT1A = getPXENV() ;
	printfBIOS(
	    "\n"
		" getPXENV() = %08X \n", 
		(unsigned)ptrPXENV_INT1A
	) ; 
		
	if (ptrPXENV_INT1A != ptrPXENV) 
	{
    	if (ptrPXENV_INT1A != NULL) 
    		printfBIOS("\n la INT 1A da un valor de ptrPXENV != ES:BX \n") ;
		else 
    		printfBIOS("\n la INT 1A no responde correctamente \n") ;
	}

	printfBIOS(
	    "\n ptrPXENV = %08X   ptrPXE = %08X ", 
		(unsigned)ptrPXENV, (unsigned)ptrPXE
	) ;
	
	leerTeclaBIOS() ;
	
	printfBIOS(
	    "\n"
		"\n"
        " ptrPXENV->Signature   = \"%6.6s\" \n"
		" ptrPXENV->Version     = %04hX \n"
		" ptrPXENV->Length      = %02X \n"
	    " ptrPXENV->Checksum    = %02X calculado = %02X ",
		ptrPXENV->Signature,
		ptrPXENV->Version,
	    ptrPXENV->Length,
	    ptrPXENV->Checksum,
	    checksum((pointer_t)ptrPXENV, ptrPXENV->Length) 
	) ;

	printStrBIOS("\n ptrPXENV->RMEntry     = ") ;
	printHexBIOS(ptrPXENV->RMEntry.segment, 4) ;
    printStrBIOS(":") ;
	printHexBIOS(ptrPXENV->RMEntry.offset, 4) ;

	printStrBIOS("\n ptrPXENV->PMOffset    = ") ;
	printLHexBIOS(ptrPXENV->PMOffset, 8) ;
	printStrBIOS("   ptrPXENV->PMSelector   = ") ;
	printHexBIOS(ptrPXENV->PMSelector, 4) ;

	printStrBIOS("\n ptrPXENV->StackSeg    = ") ;
	printHexBIOS(ptrPXENV->StackSeg, 4) ;
	printStrBIOS("       ptrPXENV->StackSize    = ") ;
	printHexBIOS(ptrPXENV->StackSize, 4) ;

	printStrBIOS("\n ptrPXENV->BC_CodeSeg  = ") ;
	printHexBIOS(ptrPXENV->BC_CodeSeg, 4) ;
	printStrBIOS("       ptrPXENV->BC_CodeSize  = ") ;
	printHexBIOS(ptrPXENV->BC_CodeSize, 4) ;

	printStrBIOS("\n ptrPXENV->BC_DataSeg  = ") ;
	printHexBIOS(ptrPXENV->BC_DataSeg, 4) ;
	printStrBIOS("       ptrPXENV->BC_DataSize  = ") ;
	printHexBIOS(ptrPXENV->BC_DataSize, 4) ;

	printStrBIOS("\n ptrPXENV->UNDIDataSeg = ") ;
	printHexBIOS(ptrPXENV->UNDIDataSeg, 4) ;
	printStrBIOS("       ptrPXENV->UNDIDataSize = ") ;
	printHexBIOS(ptrPXENV->UNDIDataSize, 4) ;

	printStrBIOS("\n ptrPXENV->UNDICodeSeg = ") ;
	printHexBIOS(ptrPXENV->UNDICodeSeg, 4) ;
	printStrBIOS("       ptrPXENV->UNDICodeSize = ") ;
	printHexBIOS(ptrPXENV->UNDICodeSize, 4) ;

	printStrBIOS("\n ptrPXENV->PXEPtr      = ") ;
	printHexBIOS(ptrPXENV->PXEPtr.segment, 4) ;
    printStrBIOS(":") ;
	printHexBIOS(ptrPXENV->PXEPtr.offset, 4) ;
	printStrBIOS("   = ") ;
	printLHexBIOS((((uint32_t)ptrPXENV->PXEPtr.segment) << 4) + (uint32_t)ptrPXENV->PXEPtr.offset, 8) ;


	printStrBIOS("\n\n ptrPXE->Signature     = \"") ;
	printStrHastaBIOS(ptrPXE->Signature, 4, TRUE) ;
	printCarBIOS('"') ;

	printStrBIOS("\n ptrPXE->StructLength  = ") ;
	printHexBIOS(ptrPXE->StructLength, 2) ;

	printStrBIOS("\n ptrPXE->StructCksum   = ") ;
	printHexBIOS((uint16_t)ptrPXE->StructCksum, 2) ;
	printStrBIOS("  calculado = ") ;
	printHexBIOS(checksum((pointer_t)ptrPXE, ptrPXE->StructLength), 2) ;


	printStrBIOS("\n ptrPXE->StructRev     = ") ;
	printHexBIOS(ptrPXE->StructRev, 2) ;

	printStrBIOS("\n ptrPXE->reserved_1    = ") ;
	printHexBIOS(ptrPXE->reserved_1, 2) ;

	printStrBIOS("\n ptrPXE->UNDIROMID     = ") ;
	printHexBIOS(ptrPXE->UNDIROMID.segment, 4) ;
	printStrBIOS(":") ;
	printHexBIOS(ptrPXE->UNDIROMID.offset, 4) ;

	printStrBIOS("\n ptrPXE->BaseROMID     = ") ;
	printHexBIOS(ptrPXE->BaseROMID.segment, 4) ;
	printStrBIOS(":") ;
	printHexBIOS(ptrPXE->BaseROMID.offset, 4) ;

	printStrBIOS("\n ptrPXE->EntryPointSP  = ") ;
	printHexBIOS(ptrPXE->EntryPointSP.segment, 4) ;
    printStrBIOS(":") ;
	printHexBIOS(ptrPXE->EntryPointSP.offset, 4) ;

	printStrBIOS("\n ptrPXE->EntryPointESP = ") ;
	printHexBIOS(ptrPXE->EntryPointESP.segment, 4) ;
    printStrBIOS(":") ;
	printHexBIOS(ptrPXE->EntryPointESP.offset, 4) ;

	printStrBIOS("\n ptrPXE->StatusCallout = ") ;
	printHexBIOS(ptrPXE->StatusCallout.segment, 4) ;
    printStrBIOS(":") ;
	printHexBIOS(ptrPXE->StatusCallout.offset, 4) ;

	printStrBIOS("             pulse una tecla ... ") ;
	leerTeclaBIOS() ;

    if (strncmp(ptrPXENV->Signature, "PXENV+", 6) != 0) 
	{ 
        printStrBIOS("\n\n Signatura PXENV+ no encontrada ") ; abortBoot() ;
    }
    if (checksum((pointer_t)ptrPXENV, ptrPXENV->Length) != 0x00) 
	{ 
        printStrBIOS("\n\n Checksum PXENV+ erroneo ") ; abortBoot() ;
    }
	
	PXEAPI = (PXEAPI_t)(*(uint32_t *)&ptrPXENV->RMEntry) ;         /* usar PXENV+ */
	
	if (ptrPXENV->Version >= 0x0201)                   
	{                                                      
	    if ((((unsigned)(ptrPXENV->PXEPtr.segment) << 4) + ((unsigned)ptrPXENV->PXEPtr.offset)) != (unsigned)ptrPXE)
	    { 
            printStrBIOS("\n\n Signatura PXENV+ no encontrada ") ; 
        }
        else if (strncmp(ptrPXE->Signature, "!PXE", 4) != 0) 
	    { 
            printStrBIOS("\n\n Signatura !PXE no encontrada ") ; 
        }
        else if (checksum((pointer_t)ptrPXE, ptrPXE->StructLength) != 0x00) 	
	    { 
            printStrBIOS("\n\n Checksum !PXE erroneo ") ; 
        }
		else 
		{
        	PXEAPI = (PXEAPI_t)(*(uint32_t *)&ptrPXE->EntryPointSP) ;           /* usar !PXE */
		}	
	}

	desc_GET_CACHED_INFO.Status = 0x0000 ;
    desc_GET_CACHED_INFO.PacketType = PXENV_PACKET_TYPE_DHCP_ACK ;
    desc_GET_CACHED_INFO.BufferSize = 0x0000 ;
    desc_GET_CACHED_INFO.Buffer.segment = 0x0000 ;
    desc_GET_CACHED_INFO.Buffer.offset = 0x0000 ;
    desc_GET_CACHED_INFO.BufferLimit = 0x0000 ;

	fptr.segment = (uint16_t)((uint32_t)&desc_GET_CACHED_INFO >> 4) ;
	fptr.offset  = (uint16_t)((uint32_t)&desc_GET_CACHED_INFO & 0x0000000F) ;

    printStrBIOS("\n &desc_GET_CACHED_INFO = ") ;
	printLHexBIOS((unsigned)&desc_GET_CACHED_INFO, 8) ;
    printStrBIOS(" = ") ;
	printHexBIOS(fptr.segment, 4) ;
    printStrBIOS(":") ;
	printHexBIOS(fptr.offset, 4) ;

	printStrBIOS("\n\n op GET_CACHED_INFO en curso \n") ; 
		
	if (callPXEAPI(PXENV_GET_CACHED_INFO, *((unsigned *)&fptr)) == PXENV_EXIT_SUCCESS) 
	{
		printStrBIOS("\n exito op GET_CACHED_INFO Status = ") ; 
		printHexBIOS(desc_GET_CACHED_INFO.Status, 4) ;
	}
	else 
	{
		printStrBIOS("\n fallo op GET_CACHED_INFO Status = ") ; 
		printHexBIOS(desc_GET_CACHED_INFO.Status, 4) ;
		for ( ; ; ) ;
	}
	
	printStrBIOS("\n") ;
	
	ptrBPLAYER = (BOOTPLAYER_t *)(
	             (((unsigned)desc_GET_CACHED_INFO.Buffer.segment) << 4) + 
	               (unsigned)desc_GET_CACHED_INFO.Buffer.offset  
	             ) ;
    printStrBIOS("\n ptrBPLAYER = ") ;
    printLHexBIOS(ptrBPLAYER, 8) ;
    printStrBIOS("\n ptrBPLAYER->opcode = ") ;
    printHexBIOS(ptrBPLAYER->opcode, 2) ;

    printStrBIOS("\n ptrBPLAYER->Hardware = ") ;
    printHexBIOS(ptrBPLAYER->Hardware, 2) ;
    printStrBIOS("\n ptrBPLAYER->Hardlen = ") ;
    printHexBIOS(ptrBPLAYER->Hardlen, 2) ;
    printStrBIOS("\n ptrBPLAYER->Gatehops = ") ;
    printHexBIOS(ptrBPLAYER->Gatehops, 2) ;
    printStrBIOS("\n ptrBPLAYER->ident = ") ;
    printLHexBIOS(ptrBPLAYER->ident, 8) ;
    printStrBIOS("\n ptrBPLAYER->seconds = ") ;
    printHexBIOS(ptrBPLAYER->seconds, 4) ;
    printStrBIOS("\n ptrBPLAYER->Flags = ") ;
    printHexBIOS(ptrBPLAYER->Flags, 4) ;

    printStrBIOS("\n ptrBPLAYER->cip (client)  = ") ;
    printIPBIOS(ptrBPLAYER->cip) ;
    printStrBIOS("\n ptrBPLAYER->yip (you)     = ") ;
    printIPBIOS(ptrBPLAYER->yip) ;
    printStrBIOS("\n ptrBPLAYER->sip (server)  = ") ;
    printIPBIOS(ptrBPLAYER->sip) ;
    printStrBIOS("\n ptrBPLAYER->gip (gateway) = ") ;
    printIPBIOS(ptrBPLAYER->gip) ;
    printStrBIOS("\n ptrBPLAYER->CAddr = ") ;
    for ( i = 0 ; i < ptrBPLAYER->Hardlen /* MAC_ADDR_LEN */ ; i++ ) 
	{ 
        printHexBIOS(ptrBPLAYER->CAddr[i], 2) ;
		if (i < (ptrBPLAYER->Hardlen-1)) printCarBIOS(':') ;
	}
    printStrBIOS("\n ptrBPLAYER->Sname = \"") ;
    printStrBIOS(ptrBPLAYER->Sname) ;
    printStrBIOS("\"") ;
    printStrBIOS("\n ptrBPLAYER->bootfile = \"") ;
    printStrBIOS(ptrBPLAYER->bootfile) ;
    printStrBIOS("\"") ;
    printStrBIOS("\n") ;
	
    printStrBIOS("\n Pulse una tecla ... \n") ;
    leerTeclaBIOS() ;
	
    descr_CLOSE.Status = 0x0000 ;
	fptr.segment = (uint16_t)((uint32_t)&descr_CLOSE >> 4) ;
	fptr.offset  = (uint16_t)((uint32_t)&descr_CLOSE & 0x0000000F) ;
	
	printStrBIOS("\n\n op TFTP_CLOSE en curso \n\n") ; 

    printStrBIOS("\n descr_CLOSE.Status = ") ;
    printHexBIOS(descr_CLOSE.Status, 4) ;
    	
	if (callPXEAPI(PXENV_TFTP_CLOSE, *((unsigned *)&fptr)) == PXENV_EXIT_SUCCESS) 
	{
		printStrBIOS("\n exito op TFTP_CLOSE Status = ") ; 
		printHexBIOS(descr_CLOSE.Status, 4) ;
	}
	else 
	{
		printStrBIOS("\n fallo op TFTP_OPEN Status = ") ; 
		printHexBIOS(descr_CLOSE.Status, 4) ;
	}
	
    printStrBIOS("\n Pulse una tecla ... \n") ;
    leerTeclaBIOS() ;
		
//	descr_OPEN.Status = 0x0001 ;
	descr_OPEN.Status = 0x0000 ;
//  descr_OPEN.ServerIPAddress.num = numIP(192,168,1,103) ;
//  descr_OPEN.GatewayIPAddress.num = numIP(192,168,1,1) ;
//  descr_OPEN.ServerIPAddress.num  = changeEndian(ptrBPLAYER->sip.num) ; 
//  descr_OPEN.GatewayIPAddress.num = changeEndian(ptrBPLAYER->gip.num) ; 
    descr_OPEN.ServerIPAddress  = ptrBPLAYER->sip ; 
    descr_OPEN.GatewayIPAddress = ptrBPLAYER->gip ; 
//  strcpy(descr_OPEN.FileName, "/BLDR_1SU.0") ;
//  strcpy(descr_OPEN.FileName, "/bldr_1su.0") ;
//  strcpy(descr_OPEN.FileName, "/MIO.DAT") ;
//  strcpy(descr_OPEN.FileName, "mio.dat") ;
//  strcpy(descr_OPEN.FileName, "/mio.dat") ;
//  strcpy(descr_OPEN.FileName, "nada") ;
    for ( i = 0 ; i < 128 ; i++ ) descr_OPEN.FileName[i] = 0x00 ;
    strcpy(descr_OPEN.FileName, "/bldr_1su.0") ;
    descr_OPEN.TFTPPort = 69 ;
//  descr_OPEN.PacketSize = 1024 ;
//  descr_OPEN.PacketSize = 2048 ;
//  descr_OPEN.PacketSize = 16384 ;
    descr_OPEN.PacketSize = 256 ;
	
	fptr.segment = (uint16_t)((uint32_t)&descr_OPEN >> 4) ;
	fptr.offset  = (uint16_t)((uint32_t)&descr_OPEN & 0x0000000F) ;
	
	printStrBIOS("\n\n op TFTP_OPEN en curso \n\n") ; 

    printStrBIOS("\n &descr_OPEN = ") ;
    printLHexBIOS((unsigned *)&descr_OPEN, 8) ;
    printStrBIOS("\n fptr = ") ;
    printLHexBIOS(*((unsigned *)&fptr), 8) ;
    
    printStrBIOS("\n descr_OPEN.Status = ") ;
    printHexBIOS(descr_OPEN.Status, 4) ;
    printStrBIOS("\n descr_OPEN.ServerIPAddress = ") ;
    printIPBIOS(descr_OPEN.ServerIPAddress) ;
    printStrBIOS("\n descr_OPEN.GatewayIPAddress = ") ;
    printIPBIOS(descr_OPEN.GatewayIPAddress) ;
    printStrBIOS("\n descr_OPEN.FileName = \"") ;
    printStrBIOS(descr_OPEN.FileName) ;
    printStrBIOS("\"") ;
    printStrBIOS("\n descr_OPEN.TFTPPort = ") ;
    printHexBIOS(descr_OPEN.TFTPPort, 4) ;
    printStrBIOS("\n descr_OPEN.PacketSize = ") ;
    printHexBIOS(descr_OPEN.PacketSize, 4) ;
	printStrBIOS("\n") ;
    		
	if (callPXEAPI(PXENV_TFTP_OPEN, *((unsigned *)&fptr)) == PXENV_EXIT_SUCCESS) 
//	if (callPXEAPI(PXENV_TFTP_OPEN, (unsigned )&descr_OPEN) == PXENV_EXIT_SUCCESS) 
	{
		printStrBIOS("\n exito op TFTP_OPEN Status = ") ; 
		printHexBIOS(descr_OPEN.Status, 4) ;
	}
	else 
	{
		printStrBIOS("\n fallo op TFTP_OPEN Status = ") ; 
		printHexBIOS(descr_OPEN.Status, 4) ;
		
	printStrBIOS("\n") ;
    printStrBIOS("\n descr_OPEN.Status = ") ;
    printHexBIOS(descr_OPEN.Status, 4) ;
    printStrBIOS("\n descr_OPEN.ServerIPAddress = ") ;
    printIPBIOS(descr_OPEN.ServerIPAddress) ;
    printStrBIOS("\n descr_OPEN.GatewayIPAddress = ") ;
    printIPBIOS(descr_OPEN.GatewayIPAddress) ;
    printStrBIOS("\n descr_OPEN.FileName = \"") ;
    printStrBIOS(descr_OPEN.FileName) ;
    printStrBIOS("\"") ;
    printStrBIOS("\n descr_OPEN.TFTPPort = ") ;
    printHexBIOS(descr_OPEN.TFTPPort, 4) ;
    printStrBIOS("\n descr_OPEN.PacketSize = ") ;
    printHexBIOS(descr_OPEN.PacketSize, 4) ;
	
		for ( ; ; ) ;
	}
	
	printStrBIOS("\n\n") ;
	
for ( ; ; ) ;

	*((uint32_t *)g_ptr) = 0xbaca1a00 ;

	*g_ptr = 'A' ;
 	*(l_ptr+4) = *g_ptr ;

    for ( ; ; )
		(*l_ptr)++ ;        /* forever incrementa byte 0 de la m. de video */

}
