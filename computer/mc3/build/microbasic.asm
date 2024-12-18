 NAM MICRO  MICROBASIC

* ***** VERSION 1.3A *****
*
* BY ROBERT H UITERWYK, TAMPA, FLORIDA
*
* MODIFIED TO RUN ON THE MC3
* BY DANIEL TUFVESSON (DTU) 2013
*
* ADDITIONAL BUGFIXES
* BY LES HILDENBRANDT (LHI) 2013
*

        ORG $20   
INDEX1  FDB $0000
INDEX2  FDB $0000 
INDEX3  FDB $0000 
INDEX4  FDB $0000 
SAVESP  FDB $0000 
NEXTBA  FDB END  
WORKBA  FDB END  
SOURCE  FDB END  
PACKLN  FDB $0000 
HIGHLN  FDB $0000 
BASPNT  FDB $0000 
BASLIN  FDB $0000 
PUSHTX  FDB $0000 
XSTACK  FDB $A07F 
RNDVAL  FDB $0000 
DIMPNT  FDB $0000 
DIMCAL  FDB $0000 
PRCNT   FCB 0  
MAXLIN  FCB 72  
BACKSP  FCB $08		FCB $0F				#### /DTU  
CANCEL  FCB $18  
MEMEND  FDB $6FFF 	FDB $1FFF 			#### /DTU
ARRTAB  FDB $0000 
KEYWD   FDB $0000 
TSIGN   FCB 0  
NCMPR   FCB 0  
TNUMB   FCB 0  
ANUMB   FCB 0  
BNUMB   FCB 0  
AESTK   FDB ASTACK 
FORPNT  FDB FORSTK 
VARPNT  FDB VARTAB 
SBRPNT  FDB SBRSTK 
SBRSTK  RMB 16  
FORSTK  RMB 48
DIMVAR  FDB VARTAB 
     
     
  ORG $00AC   
BUFNXT  FDB $00B0 
ENDBUF  FDB $00B0 
        ORG $00B0   
BUFFER  RMB $50 

        ORG $0100
PROGM   JMP STARTX	JMP START			#### /DTU
VARTAB  RMB 78
        FCB $1E
COMMAN  FCC /RUN/
        FCB $1E
        FDB RUN
        FCC /LIST/
        FCB $1E
        FDB CLIST 
        FCC /NEW/
        FCB $1E
        FDB START
        FCC /PAT/
        FCB $1E
        FDB PATCH
GOLIST  FCC /GOSUB/
        FCB $1E
        FDB GOSUB
        FCC /GOTO/
        FCB $1E
        FDB GOTO
        FCC /GO TO/
        FCB $1E
        FDB GOTO
        FCC /SIZE/
        FCB $1E
        FDB SIZE
        FCC /THEN/
        FCB $1E
        FDB IF2
        FCC /PRINT/
        FCB $1E
        FDB PRINT
        FCC /LET/
        FCB $1E
        FDB LET
        FCC /INPUT/
        FCB $1E
        FDB INPUT
        FCC /IF/
        FCB $1E
        FDB IF
        FCC /END/
        FCB $1E
        FDB READY
        FCC /RETURN/
        FCB $1E
        FDB RETURN
        FCC /DIM/
        FCB $1E
        FDB DIM
        FCC /FOR/
        FCB $1E
        FDB FOR
        FCC /NEXT/
        FCB $1E
        FDB NEXT
        FCC /REM/
        FCB $1E
        FDB REMARK
PAUMSG  FCC /PAUSE/
        FCB $1E
        FDB PAUSE
        FCB $20
COMEND  FCB $1E
IMPLET  FDB LET
        RMB 60
ASTACK  EQU *-1
RDYMSG  FCB $0D
        FCB $0A
        FCB $15
        FCB $0A
        FCB $15
        FCC /READY/
        FCB $1E
PROMPT  FCB $23
        FCB $00
        FCB $1E
        FCB $1E
PGCNTL  FCB $10
        FCB $16
        FCB $1E
        FCB $1E
        FCB $1E
ERRMS1  FCC /ERROR# /
        FCB $1E
ERRMS2  FCC / IN LINE /
        FCB $1E
KEYBD   LDAA #$3F
        BSR OUTCH
KEYBD0  LDX #BUFFER
        LDAB #10
KEYBD1  BSR INCH
        CMPA #$00
        BNE KEYB11
        DECB
        BNE KEYBD1	BNE KEYBD11			#### /LHI
KEYB10  JMP READY
KEYB11  CMPA CANCEL
        BEQ DEL
        CMPA #$0D
        BEQ IEXIT
KEYBD2  CMPA #$0A
        BEQ KEYBD1
        CMPA #$15
        BEQ KEYBD1
        CMPA #$13
        BEQ KEYBD1
KEYB55  CMPA BACKSP
        BNE KEYBD3
        CPX #BUFFER
        BEQ KEYBD1
        DEX
        BRA KEYBD1
KEYBD3  CPX #BUFFER+71
        BEQ KEYBD1
        STAA 0,X
        INX
        BRA KEYBD1
DEL     BSR CRLF
CNTLIN  LDX #PROMPT
        BSR OUTNCR
        BRA KEYBD0
IEXIT   LDAA #$1E
        STAA ,X
        STX ENDBUF
        BSR CRLF
        RTS

OUTCH
*	BSR BREAK					#### /DTU
	JMP OUTEEE
OUTEEE  EQU $C003					#### /DTU

INCH    JMP INEEE

BREAK   JMP BREAK1	THIS BREAK ROUTINE IS NOT USED FOR THE MC3
BREAK1  PSHA		  DANIEL TUFVESSON 2013
        LDAA PIAD	  CHKBRK IS USED INSTEAD
PIAD    EQU $8004
        BMI BREAK2
        JMP READY

BREAK2  PULA
        RTS

INEEE   EQU $C006					#### /DTU
OUTPUT  EQU *
        BSR OUTNCR
        BRA CRLF

OUTPU2  BSR  OUTCH
OUTPU3  INX
OUTNCR  LDAA 0,X
        CMPA #$1E
        BNE OUTPU2
        RTS

CRLF    BSR PUSHX
        LDX #CRLFST
        BSR OUTNCR
        BSR PULLX
        RTS

CRLFST  FCB $00
        FCB $0D
        FCB $0A
        FCB $15
CREND   FCB $1E
        FCB $FF,$FF
        FCB $FF,$FF
        FCB $1E
PUSHX   STX PUSHTX
        LDX XSTACK
        DEX
        DEX
        STX XSTACK
        PSHA
        LDAA PUSHTX
        STAA 0,X
        LDAA PUSHTX+1
        STAA 1,X
        PULA
        LDX PUSHTX
        RTS

PULLX   LDX XSTACK
        LDX 0,X
        INC XSTACK+1
        INC XSTACK+1
        RTS

STORE   PSHA
        PSHB
        BSR PUSHX
        JSR PULLAE
        LDX AESTK
        INX
        INX
        STX AESTK
        DEX
        LDX 0,X
        STAA 0,X
        STAB 1,X
        BSR PULLX
        PULB
        PULA
        RTS

IND     BSR PUSHX
        PSHA
        PSHB
        LDX AESTK
        INX
        INX
        STX AESTK
        DEX
        LDX 0,X
        LDAA 0,X
        LDAB 1,X
        JSR PUSHAE
        PULB
        PULA
        BSR PULLX
        RTS

LIST    LDX NEXTBA
        STX WORKBA
        LDX SOURCE
        BRA LIST1
LIST0   LDX INDEX3
LIST1   CPX WORKBA
        BEQ LEXIT
        BSR OUTLIN
        INX
        BRA LIST1
LEXIT   RTS

OUTLIN  LDAA 0,X
        CLR PRCNT
        INX
        LDAB 0,X
        INX
        CLR TSIGN
        JSR PRN0
        BSR PRINSP
OUTLI1  LDAA 0,X
        INX
        BSR PUSHX
        LDX #COMMAN
        STX KEYWD
        STAA KEYWD+1
        LDX KEYWD
        DEX
OUTLI2  DEX
        LDAA 0,X
        CMPA #$1E
        BNE OUTLI2
        INX
        INX
        INX
        JSR OUTNCR
        JSR PULLX
        JMP OUTPUT

PRINSP  PSHA
        LDAA #$20
        JSR OUTCH
        PULA
        RTS

RANDOM  INX
        INX
        LDAA 0,X
        CMPA #'D
        BNE  TSTVER
        JSR PUSHX
        LDAA RNDVAL
        LDAB RNDVAL+1
        LDX  #0000
RAND1   ADCB 1,X
        ADCA 0,X
        INX
        INX
        CPX #RNDVAL
        BNE  RAND1
        ANDA #$7F
        STAA RNDVAL
        STAB RNDVAL+1
        STX   INDEX1
        LDAA INDEX1
        LDAB INDEX1+1
        JMP   TSTV9

TSTV    JSR   SKIPSP
	JSR   CHKBRK	JSR BREAK			#### /DTU
        JSR   TSTLTR
        BCC   TSTV1
        RTS

TSTV1   CMPA #'R
        BNE TSTV2
        LDAB 1,X
        CMPB #'N
        BEQ  RANDOM
TSTV2   JSR PUSHX
        SUBA #$40
        STAA VARPNT+1
        ASLA
        ADDA VARPNT+1
        STAA VARPNT+1
        LDX VARPNT
        LDAA VARPNT
        LDAB VARPNT+1
        TST  2,X
        BNE  TSTV20
        JMP  TSTV9

TSTV20  LDX  0,X
        STX  DIMPNT
        INX
        INX
        STX DIMCAL
        JSR  PULLX
        JSR INXSKP
        CMPA #'(
        BEQ TSTV22
TSTVER  JMP DBLLTR
TSTV22  INX
        JSR EXPR
        JSR PUSHX
        JSR PULLAE
        TSTA
        BEQ TSTV3
SUBER1  JMP  SUBERR

TSTV3   LDX DIMPNT
        TSTB
        BEQ  SUBER1
        CMPB 0,X
        BHI  SUBER1
        LDAA 1,X
        STAA ANUMB
        BEQ TST666
        LDX DIMCAL
TSTV4   DECB
        BEQ TSTV6
        LDAA ANUMB
TSTV5   INX
        INX
        DECA
        BNE TSTV5
        BRA TSTV4

TSTV6   STX DIMCAL
        JSR PULLX
        JSR SKIPSP
        CMPA #',
        BNE TSTVER
        INX
        JSR EXPR
        JSR PUSHX
        JSR PULLAE
        TSTA
        BNE SUBER1
        LDX DIMPNT
        TSTB
        BEQ SUBER1
        CMPB 1,X
        BHI SUBER1
TST666  LDX DIMCAL
TSTV7   INX
        INX
        DECB
        BNE TSTV7
        DEX
        DEX
        STX DIMCAL
        JSR PULLX
        JSR SKIPSP
TSTV8   CMPA  #')
        BNE TSTVER
        JSR PUSHX
        LDAA DIMCAL
        LDAB DIMCAL+1
TSTV9   JSR  PULLX
        INX
        JSR PUSHAE
        CLC
        RTS

TSTLTR  CMPA #$41
        BMI NONO
        CMPA #$5A
        BLE YESNO
TESTNO  CMPA #$30
        BMI NONO
        CMPA #$39
        BLE YESNO
NONO    SEC
        RTS
YESNO   CLC
        RTS

PULPSH  BSR PULLAE
PUSHAE  STS SAVESP
        LDS AESTK
        PSHB
        PSHA
        STS AESTK
        LDS SAVESP
        RTS

PULLAE  STS SAVESP
        LDS AESTK
        PULA
        PULB
        STS AESTK
        LDS SAVESP
        RTS

FACT    JSR SKIPSP
        JSR TSTV
        BCS FACT0
        JSR IND
        RTS

FACT0   JSR TSTN
        BCS FACT1
        RTS

FACT1   CMPA #'(
        BNE FACT2
        INX
        BSR  EXPR
        JSR  SKIPSP
        CMPA #')
        BNE FACT2
        INX
        RTS

FACT2   LDAB #13
        JMP  ERROR

TERM    BSR  FACT
TERM0   JSR SKIPSP
        CMPA #'*
        BNE TERM1
        INX
        BSR FACT
        BSR MPY
        BRA TERM0

TERM1   CMPA #'/
        BNE TERM2
        INX
        BSR FACT
        JSR DIV
        BRA TERM0

TERM2   RTS

EXPR    JSR SKIPSP
        CMPA #'-
        BNE EXPR0
        INX
        BSR TERM
        JSR NEG
        BRA EXPR1
EXPR0   CMPA #'+
        BNE EXPR00
        INX
EXPR00  BSR TERM
EXPR1   JSR SKIPSP
        CMPA #'+
        BNE EXPR2
        INX
        BSR TERM
        JSR ADD
        BRA EXPR1
EXPR2   CMPA #'-
        BNE EXPR3
        INX
        BSR TERM
        JSR SUB
        BRA EXPR1
EXPR3   RTS

MPY     BSR MDSIGN
        LDAA #15
        STAA 0,X
        CLRB
        CLRA
MPY4    LSR 3,X
        ROR 4,X
        BCC MPY5
        ADDB 2,X
        ADCA 1,X
        BCC MPY5
MPYERR  LDAA #2
        JMP ERROR
MPY5    ASL 2,X
        ROL 1,X
        DEC 0,X
        BNE MPY4
        TSTA
        BMI MPYERR
        TST TSIGN
        BPL MPY6
        JSR NEGAB
MPY6    STAB 4,X
        STAA 3,X
        JSR PULLX
        RTS

MDSIGN  JSR PUSHX
        CLRA
        LDX AESTK
        TST 1,X
        BPL MDS2
        BSR NEG
        LDAA #$80
MDS2    INX
        INX
        STX AESTK
        TST 1,X
        BPL MDS3
        BSR NEG
        ADDA #$80
MDS3    STAA TSIGN
        DEX
        DEX
        RTS

DIV     BSR MDSIGN
        TST 1,X
        BNE DIV33
        TST 2,X
        BNE DIV33
        LDAB #8
        JMP ERROR
DIV33   LDAA #1
DIV4    INCA
        ASL 2,X
        ROL 1,X
        BMI DIV5
        CMPA #17
        BNE DIV4
DIV5    STAA 0,X
        LDAA 3,X
        LDAB 4,X
        CLR 3,X
        CLR 4,X
DIV163  SUBB 2,X
        SBCA 1,X
        BCC DIV165
        ADDB 2,X
        ADCA 1,X
        CLC
        BRA DIV167
DIV165  SEC
DIV167  ROL 4,X
        ROL 3,X
        LSR 1,X
        ROR 2,X
        DEC 0,X
        BNE DIV163
        TST TSIGN
        BPL DIV169
        BSR NEG
DIV169  JSR PULLX
        RTS

NEG     PSHA
        PSHB
        JSR PULLAE
        BSR NEGAB
        JSR PUSHAE
        PULB
        PULA
        RTS

NEGAB   COMA
        COMB
        ADDB #1
        ADCA #0
        RTS

SUB     BSR NEG
ADD     JSR PULLAE
ADD1    STAB BNUMB
        STAA ANUMB
        JSR PULLAE
        ADDB BNUMB
        ADCA ANUMB
        JSR PUSHAE
        CLC
        RTS

FINDNO  LDAA HIGHLN
        LDAB HIGHLN+1
        SUBB PACKLN+1
        SBCA PACKLN
        BCS  HIBALL
FINDN1  LDX  SOURCE
FIND0   JSR PULPSH
        SUBB 1,X
        SBCA 0,X
        BCS FIND3
        BNE FIND1
        TSTB
        BEQ  FIND4
FIND1   INX
FIND2   BSR  INXSKP
        CMPA #$1E
        BNE  FIND2
        INX
        CPX NEXTBA
        BNE FIND0
HIBALL  LDX  NEXTBA
FIND3   SEC
FIND4   STX WORKBA
        JSR PULLAE
        RTS

SKIPSP  LDAA 0,X
        CMPA #$20
        BNE  SKIPEX
INXSKP  INX
        BRA SKIPSP
SKIPEX  RTS

LINENO  JSR INTSTN
        BCC  LINE1
        LDAB #7
        JMP  ERROR
LINE1   JSR PULPSH
        STAA PACKLN
        STAB PACKLN+1
        STX BUFNXT
        RTS

NXTLIN  LDX  BASPNT
NXTL12  LDAA 0,X
        INX
        CMPA #$1E
        BNE  NXTL12	BNE NXTLIN			#### /DTU
        STX BASLIN
        RTS

CCODE   BSR SKIPSP
        STX INDEX4
        STS SAVESP
        LDX #COMMAN-1
LOOP3   LDS  INDEX4
        DES
LOOP4   INX
        PULA
        LDAB 0,X
        CMPB #$1E
        BEQ LOOP7
        CBA
        BEQ  LOOP4
LOOP5   INX
        CPX #COMEND
        BEQ CCEXIT
        LDAB 0,X
        CMPB #$1E
        BNE  LOOP5
LOOP6   INX
        INX
        BRA LOOP3
LOOP7   INX
        STS BUFNXT
        STS BASPNT
LOOP8   LDS SAVESP
        RTS

CCEXIT  LDS SAVESP
        LDX #IMPLET
        RTS

START   LDX SOURCE
        STX NEXTBA
        STX WORKBA
        STX ARRTAB
        DEX
        CLRA
START2  INX
        STAA 0,X
        CPX MEMEND
        BNE  START2
START1  CLRA
        STAA PACKLN
        STAA PACKLN+1
        STAA PRCNT
        LDX PACKLN
        STX HIGHLN
READY   LDS #$A045
        LDX #RDYMSG
        JSR OUTPUT
NEWLIN  LDS #$A045
        LDX #$A07F
        STX XSTACK
        CLR PRCNT
NEWL3   JSR CNTLIN
        LDX #BUFFER
        JSR SKIPSP
        STX BUFNXT
        JSR TESTNO
        BCS LOOP2
        JMP NUMBER
LOOP2   CMPA #$1E
        BEQ NEWLIN
        JSR CCODE
        LDX 0,X
        JMP  0,X

ERROR   LDS #$A045
        JSR CRLF
        LDX #ERRMS1
        JSR OUTNCR
        CLRA
        JSR PUSHAE
        JSR PRN
        LDX #ERRMS2
        JSR OUTNCR
        CLRB
        LDAA BASLIN
        BEQ ERROR2
        LDX BASLIN
        LDAA 0,X
        LDAB 1,X
ERROR2  JSR PRN0
        JSR CRLF
        BRA READY

RUN     LDX  SOURCE
        STX BASLIN
        LDX #SBRSTK
        STX SBRPNT
        LDX #FORSTK
        STX FORPNT
        LDX #$A07F
        STX XSTACK
        LDX NEXTBA
        STX ARRTAB
        CLRA
        DEX
RUN1    INX
        STAA 0,X
        CPX MEMEND
        BNE RUN1
        LDX #VARTAB
        LDAB  #78
RUN2    STAA 0,X
        INX
        DECB
        BNE RUN2
        JMP  BASIC

CLIST   LDX #PGCNTL
        JSR OUTPUT
        LDX  BASPNT
CLIST1  JSR SKIPSP
        CMPA #$1E
        BEQ CLIST4
        JSR INTSTN
        STX BASPNT
        JSR FINDN1
        STX INDEX3
        LDX BASPNT
        PSHA
        JSR SKIPSP
        CMPA #$1E
        PULA
        BNE CLIST2
        JSR PUSHAE
        BRA  CLIST3
CLIST2  INX
        JSR  INTSTN
CLIST3  CLRA
        LDAB #1
        JSR ADD1
        JSR FINDN1
        JSR LIST0
        BRA CLIST5
CLIST4  JSR  LIST
CLIST5  JMP  REMARK
        NOP

PATCH   JSR  NXTLIN
        LDX #BASIC
        STX $A046
        LDS #$A040
        STS SP
SP      EQU $A008
        JMP CONTRL
CONTRL  EQU  $C000

NUMBER  JSR LINENO
NUM1    JSR FINDNO
        BCC DELREP
        LDX WORKBA
        CPX NEXTBA
        BEQ CAPPEN
        BSR INSERT
        BRA NEXIT
DELREP  LDX BUFNXT
        JSR SKIPSP
        CMPA #$1E
        BNE REPLAC
        LDX NEXTBA
        CPX SOURCE
        BEQ NEXIT
        BSR DELETE
        BRA NEXIT

REPLAC  BSR DELETE
        BSR INSERT
NEXIT   JMP NEWLIN
CAPPEN  BSR INSERT
        LDX PACKLN
        STX HIGHLN
        BRA NEXIT
DELETE  STS SAVESP
        LDX WORKBA
        LDS NEXTBA
        LDAB #2
        INX
        INX
        DES
        DES
DEL2    LDAA  0,X
        DES
        INX
        INCB
        CMPA #$1E
        BNE DEL2
        STS NEXTBA
        STS ARRTAB
        LDX WORKBA
        STAB DEL5+1
* IN AT OBJECT TIME
DEL4    CPX  NEXTBA
        BEQ  DELEX
DEL5    LDAA 0,X
        STAA 0,X
        INX
        BRA DEL4

DELEX   LDS SAVESP
        RTS

INSERT  LDX BUFNXT
        JSR  CCODE
INS1    STX  KEYWD
        LDAB ENDBUF+1
        SUBB BUFNXT+1
        ADDB #$04
        STAB OFFSET+1
        ADDB NEXTBA+1
        LDAA #$00
        ADCA NEXTBA
        CMPA MEMEND
        BHI OVERFL
        STAB NEXTBA+1
        STAA NEXTBA
        LDX NEXTBA
        STX  ARRTAB
INS2    CPX  WORKBA
        BEQ BUFWRT
        DEX
        LDAA 0,X
OFFSET  STAA 0,X
        BRA  INS2
BUFWRT  LDX  WORKBA
        STS SAVESP
        LDAA PACKLN
        STAA 0,X
        INX
        LDAA PACKLN+1
        STAA 0,X
        INX
        LDAA KEYWD+1
        STAA 0,X
        INX
        LDS BUFNXT
        DES
BUF3    PULA
        STAA 0,X
        INX
        CMPA #$1E
        BNE BUF3
        LDS SAVESP
        RTS

OVERFL  LDAB #14
        JMP ERROR
BASIC   LDX BASLIN
        CPX NEXTBA
        BNE BASIC1
BASIC0  JMP READY
BASIC1  TST BASLIN
        BEQ BASIC0
        INX
        INX
        LDAA 0,X
        INX
        STX  BASPNT
        LDX #COMMAN
        STX KEYWD
        STAA KEYWD+1
        LDX #ASTACK
        STX AESTK
        LDX KEYWD
        LDX 0,X
BASIC2  JMP 0,X

GOSUB   LDX BASLIN
        STX INDEX1
        JSR NXTLIN
        LDX SBRPNT
        CPX #SBRSTK+16
        BNE  GOSUB1
        LDAB #9
        JMP  ERROR
GOSUB1  LDAA BASLIN
        STAA 0,X
        INX
        LDAA BASLIN+1
        STAA 0,X
        INX
        STX SBRPNT
        LDX INDEX1
        STX BASLIN
GOTO    LDX BASPNT
        JSR EXPR
        JSR FINDN1
        BCC GOTO2
        LDAB #7
        JMP  ERROR
GOTO2   STX BASLIN
        BRA  BASIC

RETURN  LDX  SBRPNT
        CPX #SBRSTK
        BNE RETUR1
        LDAB #10
        JMP  ERROR
RETUR1  DEX
        DEX
        STX SBRPNT
        LDX  0,X
        STX BASLIN
        BRA BASIC

PAUSE   LDX  #PAUMSG
        JSR OUTNCR
        JSR PRINSP
        LDX  BASLIN
        LDAA 0,X
        INX
        LDAB 0,X
        INX
        JSR  PRN0
PAUSE1  JSR  INCH
        CMPA #$0D
        BNE  PAUSE1
        JSR  CRLF
PAUSE2  JMP  REMARK
INPUT   LDAA  BASPNT
        BNE INPUT0
        LDAB #12
        BRA INPERR
INPUT0  JSR KEYBD
        LDX  #BUFFER
        STX BUFNXT
        LDX BASPNT
INPUT1  JSR TSTV
        BCS INPEX
        STX BASPNT
        LDX BUFNXT
INPUT2  BSR  INNUM
        BCC INPUT4
        DEX
        LDAA 0,X
        CMPA #$1E
        BEQ INPUTS
        LDAB #2
INPERR  JMP  ERROR
INPUTS  JSR  KEYBD
        LDX #BUFFER
        BRA INPUT2
INPUT4  JSR  STORE
        INX
        STX BUFNXT
        LDX BASPNT
        JSR SKIPSP
        INX
        CMPA #',
        BEQ INPUT1
INPEX   DEX
        CLR PRCNT
        CMPA #$1E
        BEQ PAUSE2
DBLLTR  LDAB #3
        JMP  ERROR
TSTN    BSR INTSTN
        BCS TSTN0
        JSR PULLAE
        TSTA
        BPL TSTN1
TSTN0   SEC
        RTS
TSTN1   JSR  PUSHAE
        RTS

INNUM   JSR  SKIPSP
        STAA TSIGN
        INX
        CMPA #'-
        BEQ  INNUM0
        DEX
INTSTN  CLR  TSIGN
INNUM0  JSR   SKIPSP
        JSR TESTNO
        BCC INNUM1
        RTS
INNUM1  DEX
        CLRA
        CLRB
INNUM2  INX
        PSHA
        LDAA 0,X
        JSR TESTNO
        BCS INNEX
        SUBA #$30
        STAA TNUMB
        PULA
        ASLB
        ROLA
        BCS INNERR
        STAB BNUMB
        STAA ANUMB
        ASLB
        ROLA
        BCS INNERR
        ASLB
        ROLA
        BCS INNERR
        ADDB BNUMB
        ADCA ANUMB
        BCS INNERR
        ADDB TNUMB
        ADCA #0
        BCC  INNUM2
INNERR  LDAB #2
        JMP  ERROR
INNEX   PULA
        TST TSIGN
        BEQ INNEX2
        JSR NEGAB
INNEX2  JSR PUSHAE
        CLC
        RTS

PRINT   LDX  BASPNT
PRINT0  JSR  SKIPSP
        CMPA #'"
        BNE PRINT4
        INX
PRINT1  LDAA 0,X
        INX
        CMPA  #'"
        BEQ  PRIN88
        CMPA #$1E
        BNE PRINT2
        LDAB  #4
        BRA  PRINTE
PRINT2  JSR  OUTCH
        JSR ENLINE
        BRA PRINT1
PRINT4  CMPA #$1E
        BNE PRINT6
        DEX
        LDAA 0,X
        INX
        CMPA #';
        BEQ PRINT5
        JSR CRLF
        CLR PRCNT
PRINT5  INX
        STX BASLIN
        JMP BASIC
PRINT6  CMPA #'T
        BNE PRINT8
        LDAB 1,X
        CMPB #'A
        BNE PRINT8
        INX
        INX
        LDAA 0,X
        CMPA #'B
        BEQ PRINT7
        LDAB #11
PRINTE  JMP  ERROR
PRINT7  INX
        JSR EXPR
        JSR PULLAE
        SUBB PRCNT
        BLS PRIN88
PRIN77  JSR PRINSP
        BSR ENLINE
        DECB
        BNE PRIN77
        BRA PRIN88
PRINT8  JSR  EXPR
        JSR  PRN
PRIN88  JSR  SKIPSP
        CMPA #',
        BNE PRIN99
        INX
PRLOOP  LDAA PRCNT
        TAB
        ANDB #$F8
        SBA
        BEQ PRI999
        JSR PRINSP
        BSR ENLINE
        BRA PRLOOP
PRIN99  CMPA #';
        BNE PREND
        INX
PRI999  JMP  PRINT0
PREND   CMPA #$1E
        BEQ PRINT4
        LDAB #6
        BRA PRINTE
ENLINE  PSHA
        LDAA PRCNT
        INCA
        CMPA MAXLIN
        BNE ENLEXT
        JSR CRLF
        CLRA
ENLEXT  STAA PRCNT
        PULA
        RTS
PRN     JSR PRINSP
        BSR ENLINE
        LDAA #$FF
        STAA TSIGN
        JSR PULLAE
        TSTA
        BPL PRN0
        JSR NEGAB
        PSHA
        LDAA #'-
        JSR OUTCH
        BSR ENLINE
        PULA
PRN0    JSR  PUSHX
        LDX #KIOK
PRN1    CLR  TNUMB
PRN2    SUBB 1,X
        SBCA 0,X
        BCS PRN5
        INC TNUMB
        BRA PRN2
PRN5    ADDB 1,X
        ADCA 0,X
        PSHA
        LDAA TNUMB
        BNE PRN6
        CPX #KIOK+8
        BEQ PRN6
        TST TSIGN
        BNE PRN7
PRN6    ADDA #$30
        CLR TSIGN
        JSR OUTCH
        BSR ENLINE
PRN7    PULA
        INX
        INX
        CPX #KIOK+10
        BNE PRN1
        JSR PULLX
        RTS

KIOK    FDB 10000
        FDB 1000
        FDB 100
        FDB 10
        FDB 1

LET     LDX BASPNT
        JSR TSTV
        BCC LET1
LET0    LDAB #12
LET00   JMP  ERROR
LET1    JSR  SKIPSP
        INX
        CMPA #'=
        BEQ LET3
LET2    LDAB #6
        BRA LET00
LET3    JSR EXPR
        CMPA #$1E
        BNE LET2
        JSR STORE
        BRA REMARK
SIZE    LDAB ARRTAB+1
        LDAA ARRTAB
        SUBB SOURCE+1
        SBCA SOURCE
        JSR PRN0
        JSR PRINSP
        LDAB MEMEND+1
        LDAA MEMEND
        SUBB ARRTAB+1
        SBCA ARRTAB
        JSR PRN0
        JSR CRLF
REMARK  JSR NXTLIN
        JMP BASIC
DIM     LDX BASPNT
DIM1    JSR SKIPSP
        JSR TSTLTR
        BCC DIM111
        JMP DIMEX
DIM111  SUBA #$40
        STAA DIMVAR+1
        ASLA
        ADDA DIMVAR+1
        STAA DIMVAR+1
        JSR PUSHX
        LDX DIMVAR
        TST 0,X
        BNE DIMERR
        TST 1,X
        BNE DIMERR
        TST 2,X
        BNE DIMERR
        LDAA ARRTAB+1
        STAA 1,X
        LDAA ARRTAB
        STAA 0,X
        STAA 2,X
        JSR PULLX
        JSR INXSKP
        CMPA #'(
        BEQ  DIM2
DIMERR  LDAB #5
DIMER1  JMP ERROR
DIM2    INX
        JSR EXPR
        JSR PULPSH
        TSTB
        BEQ SUBERR
        TSTA
        BEQ  DIM3
SUBERR  LDAB #15
        BRA DIMER1
DIM3    BSR STRSUB
        LDAA 0,X
        CMPA #',
        BNE DIM6
        INX
        JSR EXPR
        JSR PULPSH
        TSTB
        BEQ SUBERR
        TSTA
        BNE SUBERR
        BSR STRSUB
        JSR MPY
DIM6    CLRA
        LDAB #2
        JSR PUSHAE
        JSR MPY
        LDAA 0,X
        CMPA #')
        BNE DIMERR
        INX
        LDAB ARRTAB+1
        LDAA ARRTAB
        JSR ADD1
        CLRA
        LDAB #2
        JSR ADD1
        JSR PULLAE
        CMPA MEMEND
        BLS DIM7
        JMP OVERFL
DIM7    STAA ARRTAB
        STAB ARRTAB+1
        JSR SKIPSP
        CMPA #',
        BNE DIMEX
        INX
        JMP DIM1
DIMEX   CMPA #$1E
        BNE DIMERR
        JMP REMARK
STRSUB  JSR PUSHX
        LDX DIMVAR
        LDX 0,X
STRSU2  TST 0,X
        BEQ STRSU3
        INX
        BRA STRSU2
STRSU3  STAB 0,X
        JSR PULLX
        RTS

FOR     LDX  BASPNT
        JSR TSTV
        BCC FOR1
        JMP LET0
FOR1    STX BASPNT
        JSR PULPSH
        LDX FORPNT
        CPX #FORSTK+48
        BNE FOR11
        LDAB #16
        JMP ERROR
FOR11   STAA 0,X
        INX
        STAB 0,X
        INX
        STX FORPNT
        LDX BASPNT
        JSR SKIPSP
        INX
        CMPA #'=
        BEQ  FOR3
FOR2    JMP  LET2
FOR3    JSR EXPR
        JSR STORE
        INX
        CMPA #'T
        BNE FOR2
        LDAA 0,X
        INX
        CMPA #'O
        BNE FOR2
        JSR EXPR
        JSR PULLAE
        STX BASPNT
        LDX FORPNT
        STAA 0,X
        INX
        STAB 0,X
        INX
        STX FORPNT
        LDX BASPNT
        LDAA 0,X
        CMPA #$1E
FOR8    BNE FOR2
        INX
        STX BASLIN
        LDX FORPNT
        LDAA BASLIN
        STAA 0,X
        INX
        LDAB BASLIN+1
        STAB 0,X
        INX
        STX FORPNT
        JMP BASIC

NEXT    LDX BASPNT
        JSR TSTV
        BCC NEXT1
        JMP LET0
NEXT1   JSR SKIPSP
        CMPA #$1E
        BNE FOR8
        INX
        STX  BASLIN
        LDX #FORSTK
        JSR PULPSH
NEXT2   CPX FORPNT
        BEQ NEXT6
        CMPA 0,X
        BNE NEXT5
        CMPB 1,X
        BNE NEXT5
        JSR IND
        JSR PULPSH
        SUBB 3,X
        SBCA 2,X
        BCS NEXT4
        STX  FORPNT
NEXT3   JMP  BASIC
NEXT4   JSR PULLAE
        ADDB #1
        ADCA #0
        JSR PUSHX
        LDX 0,X
        STAA 0,X
        STAB 1,X
        JSR PULLX
        LDX 4,X
        STX BASLIN
        BRA  NEXT3
NEXT5   INX
        INX
        INX
        INX
        INX
        INX
        BRA NEXT2
NEXT6   LDAB #17 
        JMP ERROR

IF      LDX BASPNT
        JSR EXPR
        BSR RELOP
        STAA NCMPR
        JSR EXPR
        STX BASPNT
        BSR CMPR
        BCC IF2
        JMP  REMARK
IF2     LDX  BASPNT
        JSR  CCODE
        LDX 0,X
        JMP 0,X
RELOP   JSR SKIPSP
        INX
        CMPA #'=
        BNE RELOP0
        LDAA #0
        RTS
RELOP0  LDAB 0,X
        CMPA #'<
        BNE RELOP4
        CMPB #'=
        BNE RELOP1
        INX
        LDAA #2
        RTS
RELOP1  CMPB #'>
        BNE RELOP3
RELOP2  INX
        LDAA #3
        RTS
RELOP3  LDAA #1
        RTS
RELOP4  CMPA #'>
        BEQ REL44
        LDAB #6
        JMP ERROR
REL44   CMPB  #'=
        BNE RELOP5
        INX
        LDAA #5
        RTS
RELOP5  CMPB #'<
        BEQ RELOP2
        LDAA #4
        RTS

CMPR    LDAA  NCMPR
        ASLA
        ASLA
        STAA FUNNY+1
        LDX #CMPR1
        JSR SUB
        JSR PULLAE
        TSTA
FUNNY   JMP  0,X
CMPR1   BEQ MAYEQ
        BRA NOCMPR
        BMI OKCMPR
        BRA NOCMPR
        BMI OKCMPR
        BRA CMPR1
        BNE OKCMPR
        BRA MYNTEQ
        BEQ MYNTEQ
        BMI NOCMPR
        BPL OKCMPR
NOCMPR  SEC
        RTS
OKCMPR  CLC
        RTS
MAYEQ   TSTB
        BEQ OKCMPR
        BRA NOCMPR
MYNTEQ  TSTB
        BNE OKCMPR
        BRA NOCMPR

******************************
* REPLACEMENT FOR BREAK ROUTINE /DTU
CHKBRK	PSHA
	JSR	CONSVEC
	CMPA	#$0		IS THERE ANY CHARACTER?
	BEQ	CHKNBRK
	JSR	CONIVEC		READ CHARACTER
	CMPA	#$1B		IS CHARACTER AN ESCAPE?
	BNE	CHKNBRK
	JMP	READY		BREAK. GOTO PROMPT
CHKNBRK	PULA			NO BREAK. CONTINUE
	RTS
CONSVEC	EQU	$7FE5	CONSOLE STATUS VECTOR
CONIVEC	EQU	$7FEB	CONSOLE INPUT VECTOR

******************************
* PAGE SELECT ROUTINE FOR MC3 /DTU
STARTX	LDAA	#$87
	STAA	$0002
	JMP	START
	
END     EQU *
        END
