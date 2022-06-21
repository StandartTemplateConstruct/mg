/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
/* #define HAVE_ALLOCA_H 1 */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* #undef _POSIX_1_SOURCE */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
/* #define TIME_WITH_SYS_TIME 1 */

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define to the name of the distribution.  */
#define PACKAGE "mg"

/* Define to the version of the distribution.  */
#define VERSION "1.2"

/* Define to 1 if you have the valloc function.  */
/* #define HAVE_VALLOC 1 */

/* Define to 1 if ANSI function prototypes are usable.  */
#define PROTOTYPES 1

 
/* Define to 1 for better use of the debugging malloc library.  See
   site ftp.antaire.com in antaire/src, file dmalloc/dmalloc.tar.gz.  */
/* #undef WITH_DMALLOC */

 
/* Define to 1 if GNU regex should be used instead of GNU rx.  */
/* #undef WITH_REGEX */

/* Some braindead header files do not have the requ'd function declarations */
#define HAVE_FGETC_DECL 1
#define HAVE_FREAD_DECL 1

/* See if have this field in the prstatus_t structure */
/* It stores the size of the process heap */
/* #define HAVE_PR_BRKSIZE 1 */

/* Define if you have the ftime function.  */
/* #undef HAVE_FTIME */

/* Define if you have the getpagesize function.  */
/* #undef HAVE_GETPAGESIZE */

/* Define if you have the getrusage function.  */
/* #undef HAVE_GETRUSAGE */

/* Define if you have the mallinfo function.  */
/* #undef HAVE_MALLINFO */

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the setbuffer function.  */
/* #undef HAVE_SETBUFFER */

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strftime function.  */
#define HAVE_STRFTIME 1

/* Define if you have the strtol function.  */
#define HAVE_STRTOL 1

/* Define if you have the times function.  */
/* #define HAVE_TIMES 1 */

/* Define if you have the valloc function.  */
/* #define HAVE_VALLOC 1 */

/* Define if you have the <dirent.h> header file.  */
/* #define HAVE_DIRENT_H 1 */

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/procfs.h> header file.  */
/* #define HAVE_SYS_PROCFS_H 1 */

/* Define if you have the <sys/time.h> header file.  */
/* #define HAVE_SYS_TIME_H 1 */

/* Define if you have the <unistd.h> header file.  */
/* #define HAVE_UNISTD_H 1 */

/* Define if you have the m library (-lm).  */
#define HAVE_LIBM 1
