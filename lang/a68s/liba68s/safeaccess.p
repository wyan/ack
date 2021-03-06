15000 #include "rundecs.h"
15010     (*  COPYRIGHT 1983 C.H.LINDSEY, UNIVERSITY OF MANCHESTER  *)
15020 (**)
15030 (**)
15040 PROCEDURE GARBAGE(ANOBJECT:OBJECTP); EXTERN;
15050 PROCEDURE ERRORR(N: INTEGER); EXTERN;
15060 (**)
15070 (**)
15080 PROCEDURE FORMPDESC(OLDESC: OBJECTP; VAR PDESC1: PDESC);
15090   VAR N: OFFSETRANGE; I, J, K: INTEGER;
15100     BEGIN WITH OLDESC^, PDESC1 DO
15110       BEGIN
15120       PSIZE := SIZE;
15130       ACCOFFS := -ELSCONST;
15140       J := 0;
15150       FOR I := 0 TO ROWS DO WITH DESCVEC[I], PDESCVEC[J] DO
15160         BEGIN
15170         N := UI-LI+1; IF N<0 THEN N := 0;
15180         ACCOFFS := ACCOFFS+LI*DI;
15190         PND := DI*N;
15200         PROWS := J;
15210         IF PSIZE=DI THEN
15220           BEGIN PSIZE := PND; PD := PSIZE END
15230         ELSE
15240           BEGIN J := J+1; PD := DI END;
15250         PL := ELSCONST-LBADJ+ACCOFFS+PND;
15260         PP := PL;
15270         FOR K := PROWS-1 DOWNTO 0 DO WITH PDESCVEC[K] DO
15280           BEGIN PL := PL+LI*DI; PP := PL END;
15290         END;
15300       WITH PDESCVEC[PROWS] DO PP := PL-PND-PD
15310       END
15320     END;
15330 (**)
15340 (**)
15350 FUNCTION NEXTEL(I: INTEGER; VAR PDESC1: PDESC): BOOLEAN;
15360     BEGIN WITH PDESC1 DO WITH PDESCVEC[I] DO
15370       BEGIN
15380       PP := PP+PD;
15390       IF PP<PL THEN
15400         BEGIN
15410         NEXTEL := TRUE
15420         END
15430       ELSE IF I<PROWS THEN
15440         IF NEXTEL(I+1, PDESC1) THEN
15450           BEGIN
15460           PP := PDESCVEC[I+1].PP;
15470           PL := PP+PND;
15480           NEXTEL := TRUE
15490           END
15500         ELSE NEXTEL := FALSE
15510       ELSE
15520         BEGIN
15530         NEXTEL := FALSE;
15540         PP := PL-PND-PD
15550         END
15560       END
15570     END;
15580 (**)
15590 (**)
15600 PROCEDURE PCINCR(STRUCTPTR: UNDRESSP; TEMPLATE: DPOINT; INCREMENT: INTEGER);
15610  VAR  TEMPOS, STRUCTPOS: INTEGER;
15620       PTR: UNDRESSP;
15630  BEGIN
15640       TEMPOS:= 1;
15650       STRUCTPOS:= TEMPLATE^[1];
15660       WHILE STRUCTPOS >= 0
15670       DO BEGIN
15680            PTR := INCPTR(STRUCTPTR, STRUCTPOS);
15690            WITH PTR^ DO
15700              BEGIN
15710              FINCD(FIRSTPTR^,INCREMENT);
15720              IF FPTST(FIRSTPTR^) THEN GARBAGE(FIRSTPTR);
15730              END;
15740            TEMPOS:= TEMPOS+1;
15750            STRUCTPOS:= TEMPLATE^[TEMPOS];
15760       END;
15770  END;
15780 (**)
15790 (**)
15800 PROCEDURE PCINCRMULT(ELSPTR:OBJECTP; INCREMENT: INTEGER);
15810 VAR   TEMPLATE: DPOINT;
15820       COUNT, ELSIZE: INTEGER;
15830       PTR: UNDRESSP;
15840  BEGIN
15850       TEMPLATE:= ELSPTR^.DBLOCK;
15860       IF ORD(TEMPLATE)<=MAXSIZE  (*NOT STRUCT*)
15870      THEN
15880            IF ORD(TEMPLATE)=0 (*DRESSED*)
15890            THEN
15900                 BEGIN
15910                 PTR := INCPTR(ELSPTR, ELSCONST);
15920                 WHILE ORD(PTR)<ORD(ELSPTR)+ELSCONST+ELSPTR^.D0 DO WITH PTR^ DO
15930                      BEGIN
15940                      FINCD(FIRSTPTR^,INCREMENT);
15950                      IF FPTST(FIRSTPTR^) THEN GARBAGE(FIRSTPTR);
15960                      PTR := INCPTR(PTR, SZADDR);
15970                      END
15980                 END
15990           ELSE (*NO ACTION*)
16000      ELSE BEGIN (*STRUCT*)
16010                 ELSIZE:= TEMPLATE^[0];
16020                IF TEMPLATE^[1]>0
16030                 THEN BEGIN
16040                      COUNT := ELSPTR^.D0-ELSIZE;
16050                      PTR := INCPTR(ELSPTR, ELSCONST);
16060                      WHILE COUNT >= 0
16070                      DO BEGIN
16080                           PCINCR(PTR, TEMPLATE, INCREMENT);
16090                           PTR := INCPTR(PTR, ELSIZE);
16100                           COUNT:= COUNT-ELSIZE
16110                      END;
16120                 END;
16130       END;
16140  END;
16150 (**)
16160 (**)
16170 PROCEDURE COPYSLICE(ASLICE: OBJECTP);
16180   VAR NEWSLICE, OLDELS, NEWELS: OBJECTP;
16190       COUNT, SIZEACC, OFFACC: INTEGER;
16200       PDESC1: PDESC;
16210       OLDESCVEC: ARRAY [0..7] OF PDS;
16220       OLDLBADJ: BOUNDSRANGE;
16230       OLDROWS: 0..7;
16240   PROCEDURE CSSUPP(ASLICE: OBJECTP);
16250     VAR LSLICEADJ, COUNT, NCOUNT, NEWDI, ACCOFFS, ACCADJ: INTEGER;
16260       BEGIN
16270       WITH ASLICE^ DO
16280         BEGIN
16290         FPDEC(PVALUE^);
16300         IF FPTST(PVALUE^) THEN GARBAGE(PVALUE);
16310         PVALUE := NEWELS;
16320         FPINC(NEWELS^);
16330         ASLICE := IHEAD;
16340         END;
16350       WHILE ASLICE<>NIL DO WITH ASLICE^ DO
16360         BEGIN
16370         ACCOFFS := -ELSCONST;
16380         FOR COUNT := 0 TO ROWS DO WITH DESCVEC[COUNT] DO
16390           ACCOFFS := ACCOFFS+LI*DI;
16400         LSLICEADJ := ACCOFFS-LBADJ-PDESC1.ACCOFFS+OLDLBADJ;
16410         ACCADJ := 0;
16420         NCOUNT := ROWS;
16430         FOR COUNT := OLDROWS DOWNTO 0 DO WITH OLDESCVEC[COUNT] DO
16440           BEGIN
16450           NEWDI := NEWSLICE^.DESCVEC[COUNT].DI;
16460           ACCADJ := ACCADJ+(LSLICEADJ DIV DI)*NEWDI;
16470           LSLICEADJ := LSLICEADJ MOD DI;
16480           IF NCOUNT>=0 THEN
16490             IF DESCVEC[NCOUNT].DI=DI THEN WITH DESCVEC[NCOUNT] DO
16500               BEGIN
16510               ACCOFFS := ACCOFFS+LI*(NEWDI-DI);
16520               DI := NEWDI;
16530               NCOUNT := NCOUNT-1
16540               END;
16550           END;
16560         LBADJ := ACCOFFS-ACCADJ;
16570         CSSUPP(ASLICE);
16580         ASLICE := FPTR;
16590         END
16600       END;
16610 (**)
16620     BEGIN (*COPYSLICE*)
16630     FORMPDESC(ASLICE, PDESC1);
16640     WITH ASLICE^  DO
16650       BEGIN
16660       OLDELS := PVALUE;
16670       OLDLBADJ := LBADJ;
16680       OLDROWS := ROWS;
16690       SIZEACC:= SIZE;
16700       OFFACC:= -ELSCONST;
16710       FOR COUNT := 0 TO ROWS DO
16720         BEGIN
16730         OLDESCVEC[COUNT] := DESCVEC[COUNT];
16740         WITH DESCVEC[COUNT] DO
16750           BEGIN
16760                DI:= SIZEACC;
16770                SIZEACC := OFFACC+SIZEACC*LI;
16780                OFFACC:= SIZEACC;
16790                SIZEACC:= UI-LI;
16800                IF SIZEACC < 0
16810                THEN SIZEACC:= 0
16820                ELSE SIZEACC:= SIZEACC+1;
16830                SIZEACC:= SIZEACC*DI;
16840           END;
16850         END;
16860       LBADJ := OFFACC;
16870       ENEW(NEWELS, SIZEACC+ELSCONST);
16880       WITH NEWELS^ DO
16890         BEGIN
16900 (*-02() FIRSTWORD := SORTSHIFT*ORD(IELS); ()-02*)
16910 (*+02() PCOUNT:=0; SORT:=IELS; ()+02*)
16920         OSCOPE := 0;
16930         D0 := SIZEACC;
16940         CCOUNT:= 1;
16950         DBLOCK:= OLDELS^.DBLOCK;
16960         IHEAD := NIL;
16970         END;
16980       IF ASLICE=BPTR^.IHEAD THEN
16990         BEGIN
17000         BPTR^.IHEAD:= FPTR;
17010         IF FPTR=NIL THEN
17020           BEGIN FPDEC(BPTR^); IF FPTST(BPTR^) THEN GARBAGE(BPTR) END
17030         END
17040       ELSE BPTR^.FPTR := FPTR;
17050       IF FPTR<>NIL THEN
17060         BEGIN FPTR^.BPTR := BPTR; FPTR := NIL END;
17070       BPTR:= NIL;
17080       END;
17090     COUNT := ELSCONST;
17100     WHILE NEXTEL(0, PDESC1) DO WITH PDESC1 DO WITH PDESCVEC[0] DO
17110           BEGIN
17120           MOVELEFT(INCPTR(OLDELS, PP), INCPTR(NEWELS, COUNT), PSIZE);
17130           COUNT := COUNT+PSIZE;
17140           END;
17150     PCINCRMULT(NEWELS, +INCRF);
17160     NEWSLICE := ASLICE;
17170     CSSUPP(ASLICE);
17180     END;
17190 (**)
17200 (**)
17210 PROCEDURE TESTCC(TARGET: OBJECTP);
17220   LABEL 0000;
17230   VAR DESTREF, LDESC, HEAD, NEWMULT, NEWELS: OBJECTP;
17240       I, CREATIONC, ELSIZE, ACCOFF, LACOFFSET, LACOFF2: INTEGER;
17250     BEGIN
17260     WITH TARGET^.ANCESTOR^ DO
17270       IF PVALUE^.PCOUNT-ORD(PVALUE^.IHEAD<>NIL)>1 THEN
17280         BEGIN
17290         (*  PCOUNT > 1 FOR OTHERS BESIDES IHEAD *)
17300         WITH PVALUE^ DO BEGIN
17310           FDEC;
17320           ENEW(NEWELS, D0+ELSCONST)
17330           END;
17340         MOVELEFT(PVALUE, NEWELS, PVALUE^.D0+ELSCONST);
17350         PCINCRMULT(PVALUE, +INCRF);
17360         PVALUE:= NEWELS;
17370         NEWELS^.PCOUNT := 1; (* SORT ALREADY SET*)
17380         NEWELS^.IHEAD := NIL;
17390         CCOUNT := NEWELS^.CCOUNT
17400         END
17410       ELSE
17420         BEGIN
17430         NEWELS := PVALUE;
17440         CREATIONC := NEWELS^.CCOUNT;
17450         DESTREF := TARGET;
17460         IF CREATIONC=TARGET^.CCOUNT THEN GOTO 0000; (*EXIT*)
17470         WITH DESTREF^ DO
17480           IF SORT=REFSL1 THEN
17490             BEGIN
17500             ELSIZE := TARGET^.ANCESTOR^.SIZE; ACCOFF := ELSIZE+OFFSET;
17510             END
17520           ELSE
17530             BEGIN
17540             ELSIZE := PVALUE^.D0;
17550             ACCOFF := ELSIZE-LBADJ;
17560             FOR I := 0 TO ROWS DO WITH DESCVEC[I] DO
17570               ACCOFF := ACCOFF+LI*DI;
17580               (*ACCOFF = DIST FROM START OF ELEMENTS TO 1ST EL BEYOND THIS SLICE*)
17590             END;
17600         (*SLCOPY*)
17610         HEAD := NEWELS^.IHEAD;
17620         WHILE HEAD <> NIL DO WITH HEAD^ DO
17630           BEGIN
17640           LACOFFSET := -LBADJ-ACCOFF;
17650           FOR I := 0 TO ROWS DO WITH DESCVEC[I] DO
17660             LACOFFSET := LACOFFSET+LI*DI;
17670           (*DIST FROM BEYOND LAST EL OF DESTREF TO 1ST EL OF HEAD*)
17680           WITH DESCVEC[ROWS] DO
17690             IF UI < LI THEN
17700               I:= 0
17710             ELSE I := (UI-LI+1)*DI;
17720           LACOFF2 := I+LACOFFSET+ELSIZE;
17730           (*DIST FROM 1ST EL OF DESTREF TO BEYOND LAST EL OF HEAD*)
17740           IF (LACOFFSET>=0) OR (LACOFF2<=0) THEN
17750             HEAD := FPTR
17760           ELSE BEGIN
17770             COPYSLICE(HEAD);
17780             HEAD := NEWELS^.IHEAD;
17790             END;
17800           END;
17810         0000:IF CREATIONC<>0 THEN DESTREF^.CCOUNT := CREATIONC
17820         END
17830     END;
17840 (**)
17850 (**)
17860 PROCEDURE TESTSS (REFSTRUCT: OBJECTP);
17870 (*ASSERT ITS PCOUNT > 1*)
17880   VAR OBJSIZE: INTEGER;
17890       TEMPLATE: DPOINT;
17900       NEWSTRUCT: OBJECTP;
17910     BEGIN
17920     WITH REFSTRUCT^ DO
17930       BEGIN
17940       FPDEC(PVALUE^);
17950       TEMPLATE := PVALUE^.DBLOCK;
17960       OBJSIZE := TEMPLATE^[0];
17970       ENEW(NEWSTRUCT, OBJSIZE+STRUCTCONST);
17980       MOVELEFT(INCPTR(PVALUE, STRUCTCONST), INCPTR(NEWSTRUCT, STRUCTCONST), OBJSIZE);
17990       PCINCR(INCPTR(PVALUE, STRUCTCONST), TEMPLATE, +INCRF);
18000       WITH NEWSTRUCT^ DO
18010       BEGIN
18020 (*-02() FIRSTWORD := SORTSHIFT*ORD(STRUCT); ()-02*)
18030 (*+02() SORT:=STRUCT; ()+02*)
18040         PCOUNT := 1;
18050         LENGTH := REFSTRUCT^.PVALUE^.LENGTH;
18060         DBLOCK:= TEMPLATE
18070         END;
18080       PVALUE:= NEWSTRUCT
18090       END
18100     END;
18110 (**)
18120 (**)
18130 FUNCTION SAFEACCESS(LOCATION: OBJECTP): UNDRESSP;
18140 (* RETURNS A POINTER TO THE REAL PART OF THE STRUCTURE *)
18150     BEGIN
18160     WITH LOCATION^.ANCESTOR^ DO
18170         IF FPTWO(PVALUE^) THEN 
18180         CASE SORT OF
18190           REF1: SAFEACCESS := INCPTR(LOCATION,REF1SIZE-SZINT);
18200 (*-01()   REF2: SAFEACCESS := INCPTR(LOCATION,REF2SIZE-SZLONG); ()-01*)
18210           CREF: SAFEACCESS := IPTR;
18220           REFR, RECR, RECN, REFN:
18230             BEGIN
18240             IF SORT IN [REFR, RECR] THEN
18250               TESTCC(LOCATION)
18260             ELSE
18270               TESTSS(ANCESTOR);
18280             PVALUE^.OSCOPE := 0;
18290             SAFEACCESS := INCPTR(PVALUE, LOCATION^.OFFSET)
18300             END;
18310           UNDEF: ERRORR(RASSIG);
18320           NILL: ERRORR(RASSIGNIL)
18330           END
18340       ELSE BEGIN
18350         PVALUE^.OSCOPE := 0;
18360         SAFEACCESS := INCPTR(PVALUE,LOCATION^.OFFSET)
18370         END
18380     END;
18390 (**)
18400 (**)
18410 (*-02() BEGIN END ; ()-02*)
18420 (*+01()
18430 BEGIN (*OF MAIN PROGRAM*)
18440 END (*OF EVERYTHING*).
18450 ()+01*)
