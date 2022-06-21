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
 
/* --- routines --- */
static int query_lex();
static int yyerror(char *);
#define yylex() query_lex(&ch_buf, end_buf)
 
/* --- module variables --- */
static char *ch_buf; /* ptr to the character query line buffer */
static char *end_buf; /* ptr to the last character of the line buffer */
static bool_tree_node *tree_base = NULL;
static TermList **term_list;
static int stem_method;
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
 
 
term:     TERM  { $$ = CreateBoolTermNode(term_list, $1); }
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
 
static int query_lex(char **ptr, const char *end)
{
  char *buf_ptr = *ptr;

  /* jump over whitespace */
  while (isspace(*buf_ptr))
    buf_ptr++;
 
  if (INAWORD(*buf_ptr))
    {
      char *word = Xmalloc(MAXSTEMLEN+1);
      
      if (!word)
        FatalError(1, "Unable to allocate memory for boolean term");
 
      PARSE_STEM_WORD(word, buf_ptr, end);
 
      yylval.text = word;
 
      stemmer(stem_method, (u_char*)yylval.text);
 
      *ptr = buf_ptr; /* fix up ptr */
      return TERM;
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
          TermList **the_term_list, int the_stem_method, int *res)
{
  /* global variables to be accessed by bison/yacc created parser */
  term_list = the_term_list;
  stem_method = the_stem_method;
  ch_buf = query_line;
  end_buf = query_line + query_len;
 
  FreeBoolTree(&(tree_base));
 
  ResetTermList(term_list);
  *res = yyparse();
 
  return tree_base;
}
 

