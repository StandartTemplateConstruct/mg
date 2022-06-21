/* Special definitions for mg, processed by autoheader.
   Created by [TS:Aug/95] 
   by grepping for AC_DEFINE on configure.in and using the appropriate defs 
   of acconfig.h used by GNU tar.
*/

/* Define to the name of the distribution.  */
#undef PACKAGE

/* Define to the version of the distribution.  */
#undef VERSION

/* Define if `union wait' is the type of the first arg to wait functions.  */
#undef HAVE_UNION_WAIT

/* Define to 1 if you have the valloc function.  */
#undef HAVE_VALLOC

/* Define to 1 if ANSI function prototypes are usable.  */
#undef PROTOTYPES
 
/* Define to 1 for better use of the debugging malloc library.  See
   site ftp.antaire.com in antaire/src, file dmalloc/dmalloc.tar.gz.  */
#undef WITH_DMALLOC
 
/* Define to 1 if GNU regex should be used instead of GNU rx.  */
#undef WITH_REGEX

/* Some braindead header files do not have the requ'd function declarations */
#undef HAVE_FGETC_DECL
#undef HAVE_FREAD_DECL

/* See if have this field in the prstatus_t structure */
/* It stores the size of the process heap */
#undef HAVE_PR_BRKSIZE
