/* $Header$ */
/*
 * (c) copyright 1987 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 */
/* USER-OPTION HANDLING */

#include	<alloc.h>
#include	"idfsize.h"
#include	"class.h"
#include	"macro.h"
#include	"idf.h"

char options[128];			/* one for every char	*/
int inc_pos = 1;			/* place where next -I goes */
int inc_max;
int inc_total;
int do_preprocess = 1;
int do_dependencies = 0;
char **inctable;
char *dep_file = 0;

extern int idfsize;
int txt2int();

do_option(text)
	char *text;
{
	switch(*text++)	{
	case '-':
		options[*text] = 1;
		break;
	case 'u':
		if (! strcmp(text,"ndef")) {
			/* ignore -undef */
			break;
		}
		/* fall through */
	default:
		error("illegal option: %c", text[-1]);
		break;
	case 'C' :	/* comment output		*/
	case 'P' :	/* run preprocessor stand-alone, without #'s	*/
		options[*(text-1)] = 1;
		break;
	case 'A' :	/* for Amake */
	case 'd' :	/* dependency generation */
		do_dependencies = 1;
		if (*text) {
			dep_file = text;
		}
		else {
			do_preprocess = 0;
		}
		break;
	case 'm':
	case 'i':
		options[*(text-1)] = 1;
		break;

	case 'D' :	/* -Dname :	predefine name		*/
	{
		register char *cp = text, *name, *mactext;

		if (class(*cp) != STIDF)	{
			error("identifier missing in -D%s", text);
			break;
		}
		name = cp;
		while (*cp && in_idf(*cp))
			++cp;
		if (!*cp)			/* -Dname */
			mactext = "1";
		else
		if (*cp == '=')	{		/* -Dname=text	*/
			*cp++ = '\0';		/* end of name	*/
			mactext = cp;
		}
		else	{			/* -Dname?? */
			error("malformed option -D%s", text);
			break;
		}
		macro_def(str2idf(name, 0), mactext, -1, strlen(mactext), NOFLAG);
		break;
	}
	case 'I' :	/* -Ipath : insert "path" into include list	*/
		if (*text)	{
			register int i;
			register char *new = text;

			if (++inc_total > inc_max) {
				inctable = (char **)
				  Realloc(inctable,(inc_max+=10)*sizeof(char *));
			}

			for(i = inc_pos++; i <= inc_total; i++) {
				char *tmp = inctable[i];

				inctable[i++] = new;
				new = tmp;
			}
		}
		else	inctable[inc_pos] = 0;
		break;
	case 'M':	/* maximum identifier length */
		idfsize = txt2int(&text);
		if (*text)
			error("malformed -M option");
		if (idfsize > IDFSIZE) {
			warning("maximum identifier length is %d", IDFSIZE);
			idfsize = IDFSIZE;
		}
		if (idfsize < 8) {
			warning("minimum identifier length is 8");
			idfsize = 8;
		}
		break;
	case 'U' :	/* -Uname :	undefine predefined	*/
		if (*text)	{
			register struct idf *idef = findidf(text);

			if (idef && idef->id_macro) {
				free_macro(idef->id_macro);
				idef->id_macro = (struct macro *) 0;
			}
		}
		break;
	}
}

int
txt2int(tp)
	char **tp;
{
	/*	the integer pointed to by *tp is read, while increasing
		*tp; the resulting value is yielded.
	*/
	register int val = 0;
	register int ch;
	
	while (ch = **tp, ch >= '0' && ch <= '9')	{
		val = val * 10 + ch - '0';
		(*tp)++;
	}
	return val;
}
