42800 #include "rundecs.h"
42810     (*  COPYRIGHT 1983 C.H.LINDSEY, UNIVERSITY OF MANCHESTER  *)
42820 (**)
42830 (**)
42840 PROCEDURE GARBAGE (ANOBJECT: OBJECTP); EXTERN ;
42850 (**)
42860 (**)
42870 PROCEDURE RANGEXT;
42880 (*PRANGEXT*)
42890 (*+01()  EXTERN; ()+01*)
42900 (*+05()  EXTERN; ()+05*)
42910 (*-01() (*-05()
42920   VAR LASTRG: PRANGE;
42930       IDP: PIDBLK ;
42940       PP: OBJECTPP ;
42950       I, J: INTEGER;
42960   BEGIN
42970   WITH FIRSTRG.RIBOFFSET^ DO
42980    WITH FIRSTW DO
42990     BEGIN
43000     IDP := RGIDBLK ;
43010     IF FIRSTRG.RIBOFFSET^.RIBOFFSET = FIRSTRG.RIBOFFSET THEN (*PARAMS*)
43020 (*-41() PP := INCPTR(RGNEXTFREE, -PROCBL^.PARAMS) ()-41*)
43030 (*+41() PP := INCPTR(RGLASTUSED, +PROCBL^.PARAMS) ()+41*)
43040     ELSE
43050       (*-41() PP := INCPTR ( FIRSTRG.RIBOFFSET , RGCONST ) ; ()-41*)
43060       (*+41()  PP := ASPTR ( ORD( FIRSTRG.RIBOFFSET ) ) ; ()+41*)
43070     FIRSTRG.RIBOFFSET := RIBOFFSET ;
43080     (*-41() WHILE ORD (PP) < ORD (RGNEXTFREE) DO ()-41*)
43090     (*+41() WHILE ORD (PP) > ORD (RGLASTUSED) DO ()+41*)
43100       BEGIN
43110         IDP := INCPTR (IDP , -SZIDBLOCK) ;
43120         WITH IDP^ DO
43130           BEGIN
43140             IF IDSIZE = 0 THEN
43150               BEGIN
43160                 (*+41() PP := INCPTR( PP , - SZADDR ) ; ()+41*)
43170                 WITH PP^^ DO
43180                   BEGIN
43190                     FDEC;
43200                     IF FTST THEN GARBAGE (PP^)
43210                   END ;
43220                 (*-41() PP := INCPTR( PP , SZADDR ) ()-41*)
43230               END
43240             ELSE PP := INCPTR( PP , (*+41() - ()+41*) IDSIZE )
43250           END
43260       END
43270     END
43280   END;
43290 (**)
43300 (**)
43310 FUNCTION RANGXTP(ANOBJECT: OBJECTP): OBJECTP;
43320 (*PRANGEXT+2*)
43330     BEGIN
43340     WITH ANOBJECT^ DO FINC;
43350     RANGEXT;
43360     WITH ANOBJECT^ DO FDEC;
43370     RANGXTP := ANOBJECT;
43380     END;
43390 ()-05*) ()-01*)
43400 (**)
43410 (**)
43420 (*-02()
43430   BEGIN
43440   END ;
43450 ()-02*)
43460 (*+01()
43470 BEGIN (*OF MAIN PROGRAM*)
43480 END (*OF EVERYTHING*).
43490 ()+01*)
