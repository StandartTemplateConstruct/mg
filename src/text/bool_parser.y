/**************************************************************************
 *
 * bool_parser - boolean query parser
 * Copyright (C) 1994  Neil Sharman & Tim Shimmin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 **************************************************************************/
 
/**************************************************************************/
%{
 
#include "sysfuncs.h"

#include "messages.h"
 
#include "memlib.h"
#include "words.h"
#include "stemmer.h"
#include "term_lists.h"
#include "bool_tree.h"
/* [RPAP - Jan 97: Stem Index Change] */
#include "backend.h"     /* for stemmed_dict def */
#include "stem_search.h"

#include "query_term_list.h"  /* [RPAP - Feb 97: Term Frequency] */

/* --- routines --- */
static int query_lex();
static int yyerror(char *);
#define yylex() query_lex(&ch_buf, end_buf)
 
/* --- module variables --- */
static char *ch_buf; /* ptr to the character query line buffer */
static char *end_buf; /* ptr to the last character of the line buffer */
static bool_tree_node *tree_base = NULL;
static TermList **term_list;
static int stemmer_num;
static int stem_method;
/* [RPAP - Jan 97: Stem Index Change] */
stemmed_dict *p__sd;
static int indexed;
/* [RPAP - Feb 97: Term Frequency] */
static QueryTermList **query_term_list;
static int word_num;
static u_long count;
static u_long doc_count;
static u_long invf_ptr;
static u_long invf_len;
%}
 
 
%union {
  char *text;
  bool_tree_node *node;
}
 
%token <text> TERM
%type <node> query term not and or
 
%%
 
query: or  { tree_base = $1;}
;
 
 
term:     TERM  { $$ = CreateBoolTermNode(term_list, $1, 1, word_num, count, doc_count, invf_ptr, invf_len, stemmer_num); }
        | '(' or ')' { $$ = $2; }
        | '*' { $$ = CreateBoolTreeNode(N_all, NULL, NULL); }
        | '_' { $$ = CreateBoolTreeNode(N_none, NULL, NULL); }
;
 
not:      term
        | '!' not { $$ = CreateBoolTreeNode(N_not, $2, NULL); }
;
 
and:      and '&' not { $$ = CreateBoolTreeNode(N_and, $1, $3); }
        | and not { $$ = CreateBoolTreeNode(N_and, $1, $2); }
        | not
;
 
or:       or '|' and { $$ = CreateBoolTreeNode(N_or, $1, $3); }
        | and
;
 
%%
 
/* Bison on one mips machine defined "const" to be nothing but
   then did not undef it */
#ifdef const
#undef const
#endif

/**************************************************************************/ 


/* =========================================================================
 * Function: query_lex
 * Description:
 *      Hand written lexical analyser for the parser.
 * Input:
 *      ptr = ptr to a ptr into character query-line buffer
 *      end = ptr to last char in buffer
 * Output:
 *      yylval.text = the token's text
 * Notes:
 *      does NOT produce WILD tokens at the moment
 * ========================================================================= */
 
/* [RPAP - Jan 97: Stem Index Change]
   state mode:
      0 = Read next token
      1 = Output word
      2 = Output '|' or ')'
 */
static int query_lex(char **ptr, const char *end)
{
  char *buf_ptr = *ptr;
  static int mode = 0;
  static int termnum = 0;
  static TermList *Terms = NULL;

  if (mode == 0)
    {
      /* jump over whitespace */
      buf_ptr = skipspace(buf_ptr, end);

      if (inaword(buf_ptr, end))
	{
	  static char word[MAXSTEMLEN + 1]; /* [RJM 07/98: Memory Leak] */
	  char *sWord = Xmalloc(MAXSTEMLEN + 1);
	  int stem_to_apply, method_using = -1;

	  PARSE_STEM_WORD(word, buf_ptr, end);

	  /* Extract any parameters */
	  stem_to_apply = stem_method;
	  while (buf_ptr <= end)
	    {
	      int stem_param, param_type;
	      char param[MAXPARAMLEN + 1];

	      param_type = 0;
	      PARSE_OPT_TERM_PARAM (param, param_type, buf_ptr, end);
	      if (!param_type)
		break;

	      if (param_type == STEMPARAM)
		{
		  stem_param = atoi (param);
		  if (errno != ERANGE && indexed && stem_param >= 0 && stem_param <= 3)
		    method_using = stem_to_apply = stem_param;
		}
	    }

	  bcopy ((char *) word, (char *) sWord, *word + 1);
	  stemmer (stem_to_apply, stemmer_num, sWord);

	  if (stem_to_apply == 0 || !indexed || p__sd == NULL)
	    {
	      /* [RPAP - Feb 97: Term Frequency] */
	      word_num = FindWord (p__sd, sWord, &count, &doc_count, &invf_ptr, &invf_len);
	      if (word_num == -1)
		count = doc_count = invf_ptr = invf_len = 0;
	      AddQueryTerm (query_term_list, (u_char *) word, count, method_using);

	      yylval.text = word;
	      *ptr = buf_ptr; /* fix up ptr */
	      Xfree (sWord);
	      return TERM;
	    }
	  else
	    {
	      *ptr = buf_ptr; /* fix up ptr */
	      termnum = 0;
	      ResetTermList (&Terms);
	      if (FindWords (p__sd, (u_char *) sWord, stem_to_apply, &Terms) > 0)
		{
		  /* [RPAP - Feb 97: Term Frequency] */ 
		  int i, freq = 0;
		  for (i = 0; i < Terms->num; i++)
		    freq += Terms->TE[i].WE.count;
		  AddQueryTerm (query_term_list, word, freq, method_using);

		  Xfree (sWord);
		  mode = 1;
		  return '(';
		}
	      else
		{
		  /* Word does not exists - include in tree anyway */
		  Xfree (sWord);

		  /* [RPAP - Feb 97: Term Frequency] */
		  word_num = -1;
		  count = doc_count = invf_ptr = invf_len = 0;
		  AddQueryTerm (query_term_list, (u_char *) word, count, method_using);

		  yylval.text = word;
		  return TERM;
		}
	    }
	}
      else /* NON-WORD */
	{
	  if (*buf_ptr == '\0')
	    {
	      /* return null-char if it is one */
	      *ptr = buf_ptr; /* fix up ptr */
	      return 0;
	    }
	  else
	    {
	      /* return 1st char, and delete from buffer */
	      char c = *buf_ptr++;
	      *ptr = buf_ptr; /* fix up ptr */
	      return c;
	    }
	}
    }
  else if (mode == 1)
    {
      yylval.text = Terms->TE[termnum].Word;
      
      /* [RPAP - Feb 97: Term Frequency] */
      word_num = Terms->TE[termnum].WE.word_num;
      count = Terms->TE[termnum].WE.count;
      doc_count = Terms->TE[termnum].WE.doc_count;
      invf_ptr = Terms->TE[termnum].WE.invf_ptr;
      invf_len = Terms->TE[termnum].WE.invf_len;

      termnum++;
      mode = 2;
      return TERM;
    }
  else  /* mode == 2 */
    {
      if (termnum >= Terms->num)
	{
	  mode = 0;
	  return ')';
	}
      else
	{
	  mode = 1;
	  return '|';
	}
    }
}/*query_lex*/

/* =========================================================================
 * Function: yyerror
 * Description: 
 * Input: 
 * Output: 
 * ========================================================================= */ 
static int yyerror(char *s)
{
  Message("%s", s);
  return(1);
}

 
/* =========================================================================
 * Function: ParseBool
 * Description:
 *      Parse a boolean query string into a term-list and a boolean parse tree
 * Input:
 *      query_line = query line string
 *      query_len = query line length
 *      the_stem_method = stem method id used for stemming
 * Output:
 *      the_term_list = the list of terms
 *      res = parser result code
 * ========================================================================= */
 
bool_tree_node *
ParseBool(char *query_line, int query_len,
          TermList **the_term_list, int the_stemmer_num, int the_stem_method, int *res,
	  stemmed_dict * the_sd, int is_indexed,   /* [RPAP - Jan 97: Stem Index Change] */
	  QueryTermList **the_query_term_list)  /* [RPAP - Feb 97: Term Frequency] */
{
  /* global variables to be accessed by bison/yacc created parser */
  term_list = the_term_list;
  stemmer_num = the_stemmer_num;
  stem_method = the_stem_method;
  ch_buf = query_line;
  end_buf = query_line + query_len;
  p__sd = the_sd;   /* [RPAP - Jan 97: Stem Index Change] */
  indexed = is_indexed;  /* [RPAP - Jan 97: Stem Index Change] */
  query_term_list = the_query_term_list; /* [RPAP - Feb 97: Term Frequency] */

  FreeBoolTree(&(tree_base));
 
  ResetTermList(term_list);
  ResetQueryTermList(query_term_list);  /* [RPAP - Feb 97: Term Frequency] */

  *res = yyparse();
 
  return tree_base;
}
 

