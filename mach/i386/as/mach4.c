#define RCSID4 "$Header$"

/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 *
 */

operation
	:
		prefix oper
			{	address_long = 1; operand_long = 1; }
	|	prefix1		/* to allow for only prefixes on a line */
	;
prefix	:	/* empty */
	|	prefix1
	;
prefix1:	prefix PREFIX
			{	if ($2 == 0146) operand_long = ! operand_long;
				if ($2 == 0147) address_long = ! address_long;
				emit1($2);
			}
	;
oper	:	NOOP_1
			{	emit1($1);}
	|	NOOP_2
			{	emit2($1);}
	|	JOP expr
			{	branch($1,$2);}
	|	JOP2 expr
			{	ebranch($1,$2);}
	|	PUSHOP ea_1
			{	pushop($1);}
	|	IOOP absexp
			{	emit1($1);
				fit(ufitb($2));
				emit1((int)$2);
			}
	|	IOOP R32
			{	if ($2!=2) serror("register error");
				emit1($1+010);
			}
	|	BITTEST ea_ea
			{	bittestop($1);}
	|	BOUND R32 ',' mem
			{	emit1($1); ea_2($2<<3); }
	|	ADDOP ea_ea
			{	addop($1);}
	|	ROLOP ea_ea
			{	rolop($1);}
	|	INCOP ea_1
			{	incop($1);}
	|	NOTOP ea_1
			{	regsize($1); emit1(0366|($1&1)); ea_1($1&070);}
	|	CALLOP ea_1
			{	callop($1&0xFFFF);}
	|	CALFOP expr ':' expr
			{	emit1($1>>8);
#ifdef RELOCATION
				newrelo($4.typ, RELO4);
#endif
				emit4((long)($4.val));
#ifdef RELOCATION
				newrelo($2.typ, RELO2);
#endif
				emit2((int)($2.val));
			}
	|	CALFOP mem
			{	emit1(0377); ea_2($1&0xFF);}
	|	ENTER absexp ',' absexp
			{	fit(fitw($2)); fit(fitb($4));
				emit1($1); emit2((int)$2); emit1((int)$4);
			}
	|	LEAOP R32 ',' mem
			{	emit1($1); ea_2($2<<3);}
	|	LEAOP2 R32 ',' mem
			{	emit1(0xF); emit1($1); ea_2($2<<3);}
	|	LSHFT	ea_1 ',' R32 ',' ea_2
			{	extshft($1, $4);}
	|	EXTEND R32 ',' ea_2
			{	emit1(0xF); emit1($1); ea_2($2<<3);}
	|	EXTOP R32 ',' ea_2
			{	emit1(0xF); emit1($1); ea_2($2<<3);}
	|	EXTOP1 ea_1
			{	emit1(0xF); emit1($1&07); ea_1($1&070);}
	|	IMULB	ea_1
			{	regsize(0); emit1(0366); ea_1($1&070);}
	|	IMUL	ea_2
			{	reg_1 = IS_R32; imul(0); }
	|	IMUL	R32 ',' ea_2
			{	reg_1 = $2 | IS_R32; imul($2|0x10); }
	|	IMUL	R32 ',' ea_ea
			{	imul($2|0x10);}
	|	INT absexp
			{	if ($2==3)
					emit1(0314);
				else {
					fit(ufitb($2));
					emit1(0315); emit1((int)$2);
				}
			}
	|	RET
			{	emit1($1);}
	|	RET expr
			{	emit1($1-1);
#ifdef RELOCATION
				newrelo($2.typ, RELO2);
#endif
				emit2((int)($2.val));
			}
	|	SETCC ea_2
			{	emit1(0xF); emit1($1); ea_2(0);}
	|	XCHG ea_ea
			{	xchg($1);}
	|	TEST ea_ea
			{	test($1);}
	|	MOV ea_ea
			{	mov($1);}
	|	/*	What is really needed is just
			MOV R32 ',' RSYSCR
			but this gives a bad yacc conflict
		*/
		MOV ea_1 ',' RSYSCR
			{
				if ($1 != 1 || !(reg_1 & IS_R32))
					serror("syntax error");
				emit1(0xF); emit1(042); emit1(0200|($4<<3)|(reg_1&07));}
	|	MOV ea_1 ',' RSYSDR
			{
				if ($1 != 1 || !(reg_1 & IS_R32))
					serror("syntax error");
				emit1(0xF); emit1(043); emit1(0200|($4<<3)|(reg_1&07));}
	|	MOV ea_1 ',' RSYSTR
			{
				if ($1 != 1 || !(reg_1 & IS_R32))
					serror("syntax error");
				emit1(0xF); emit1(046); emit1(0200|($4<<3)|(reg_1&07));}
	|	MOV RSYSCR ',' R32
			{
				if ($1 != 1) serror("syntax error");
				emit1(0xF); emit1(040); emit1(0200|($4<<3)|$2);}
	|	MOV RSYSDR ',' R32
			{
				if ($1 != 1) serror("syntax error");
				emit1(0xF); emit1(041); emit1(0200|($4<<3)|$2);}
	|	MOV RSYSTR ',' R32
			{
				if ($1 != 1) serror("syntax error");
				emit1(0xF); emit1(044); emit1(0200|($4<<3)|$2);}
/* Intel 80[23]87 coprocessor instructions */
	|	FNOOP
			{	emit1($1); emit1($1>>8);}
	|	FMEM mem
			{	emit1($1); ea_2(($1>>8)&070);}
	|	FMEM_AX R32
			{	if ($2 != 0) {
					serror("illegal register");
				}
				emit1(FESC|7); emit1(7<<5);
			}
	|	FST_I st_i
			{	emit1($1); emit1(($1>>8)|$2); }
	|	FST_I ST
			{	emit1($1); emit1($1>>8); }
	|	FST_ST ST ',' st_i
			{	emit1($1); emit1(($1>>8)|$4); }
	|	FST_ST st_i ',' ST
			{	emit1($1|4); emit1((($1>>8)|$2)); }
	|	FST_ST2 st_i ',' ST
			{	emit1($1|4); emit1((($1>>8)|$2)^010); }
	;

st_i	:	ST '(' absexp ')'
			{	if (!fit3($3)) {
					serror("illegal index in FP stack");
				}
				$$ = $3;
			}
	;

	;
mem	:	'(' expr ')'
			{	rm_2 = 05; exp_2 = $2; reg_2 = 05; mod_2 = 0;
				RELOMOVE(rel_2, relonami);
			}
	|	bases
			{	exp_2.val = 0; exp_2.typ = S_ABS; indexed();}
	|	expr bases
			{	exp_2 = $1; indexed();
				RELOMOVE(rel_2, relonami);
			}
	;
bases	:	'(' R32 ')'
			{	reg_2 = $2; sib_2 = 0; rm_2 = 0;}
	|	'(' R32 ')' '(' R32 scale ')'
			{	rm_2 = 04; sib_2 |= regindex_ind[$2][$5];
				reg_2 = $2;
			}
	|	'(' R32 '*' absexp ')'
			{	if ($4 == 1) {
					reg_2 = $2; sib_2 = 0; rm_2 = 0;
				}
				else {
					rm_2 = 04;
					sib_2 = checkscale($4) | regindex_ind[05][$2];
					reg_2 = 015;
				}
			}
	;
scale	:	/* empty */
			{	sib_2 = 0;}
	|	'*' absexp
			{	sib_2 = checkscale($2);}
	;
ea_2	:	mem
	|	R8
			{	reg_2 = $1 | IS_R8; rm_2 = 0;}
	|	R32
			{	reg_2 = $1 | IS_R32; rm_2 = 0;}
	|	RSEG
			{	reg_2 = $1 | IS_RSEG; rm_2 = 0;}
	|	expr
			{	reg_2 = IS_EXPR; exp_2 = $1; rm_2 = 0;
				RELOMOVE(rel_2, relonami);
			}
	;
ea_1	:	ea_2
			{	op_1 = op_2;
				RELOMOVE(rel_1, rel_2);
			}
	;
ea_ea	:	ea_1 ',' ea_2
	;
