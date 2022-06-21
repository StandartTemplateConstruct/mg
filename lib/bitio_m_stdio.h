/**************************************************************************
 *
 * bitio_m_stdio.h -- Macros for bitio to a file
 * Copyright (C) 1994  Neil Sharman
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
 * $Id: bitio_m_stdio.h,v 1.2 1994/09/20 04:19:55 tes Exp $
 *
 **************************************************************************
 *
 *  This file contains macros for doing bitwise input and output on a FILE*.
 *  With these routines you cannot mix reads and writes on the FILE,
 *  or multiple writes, at the same time and guarantee them to work, also you 
 *  cannot seek to a point and do a write. The decode routine can detect when
 *  you run off the end of the file and will produce an approate error message.
 *
 *
 **************************************************************************/

#ifndef H_BITIO_M_STDIO
#define H_BITIO_M_STDIO

typedef struct stdio_bitio_state
  {
    FILE *File;
    unsigned char Buff;
    unsigned char Btg;
  }
stdio_bitio_state;



#ifndef DECODE_ERROR
#define DECODE_ERROR (fprintf(stderr,"Unexpected EOF in \"%s\" on line %d\n",\
				__FILE__, __LINE__), exit(1))
#endif



#define ENCODE_START(f)							\
  {									\
    FILE *__outfile = f;						\
    register unsigned char __buff = 0;					\
    register unsigned char __btg = sizeof(__buff)*8;

#define ENCODE_CONTINUE(b)						\
  {									\
    FILE *__outfile = (b).File;						\
    register unsigned char __buff = (b).Buff;				\
    register unsigned char __btg = (b).Btg;

#define ENCODE_BIT(b)							\
  do {									\
    __btg--;								\
    if (b) __buff |= (1 << __btg);					\
    if (!__btg)								\
      {									\
	putc(__buff, __outfile);					\
	__buff = 0;							\
	__btg = sizeof(__buff)*8;					\
      }									\
  } while(0)

#define ENCODE_PAUSE(b)							\
    (b).File = __outfile;						\
    (b).Buff = __buff;							\
    (b).Btg =  __btg;							\
  }

#define ENCODE_DONE							\
    if (__btg != sizeof(__buff)*8)					\
      putc(__buff,__outfile);						\
  }


#define DECODE_START(f)							\
  {									\
    FILE *__infile = f;							\
    register unsigned char __buff = 0;					\
    register unsigned char __btg = 0;

#define DECODE_CONTINUE(b)						\
  {									\
    FILE *__infile = (b).File;						\
    register unsigned char __buff = (b).Buff;				\
    register unsigned char __btg = (b).Btg;

#define DECODE_ADD_FF(b)						\
  do {									\
    if (!__btg)								\
      {									\
	register int val = getc(__infile);				\
	__buff = val == EOF ? 0xff : val;				\
	__btg = sizeof(__buff)*8;					\
      }									\
    (b) += (b) + ((__buff >> --__btg) & 1);				\
  } while(0)

#define DECODE_ADD_00(b)						\
  do {									\
    if (!__btg)								\
      {									\
	register int val = getc(__infile);				\
	__buff = val == EOF ? 0xff : val;				\
	__btg = sizeof(__buff)*8;					\
      }									\
    (b) += (b) + ((__buff >> --__btg) & 1);				\
  } while(0)

#define DECODE_BIT (__btg ? (__buff >> --__btg) & 1 :			\
   (__buff = getc(__infile), (feof(__infile) ?				\
    (DECODE_ERROR, 0) :							\
    (__btg = sizeof(__buff)*8, (__buff >> --__btg) & 1))))

#define DECODE_DONE	;						\
  }

#define DECODE_PAUSE(b)							\
    (b).File = __infile;						\
    (b).Buff = __buff;							\
    (b).Btg =  __btg;							\
  }


#define DECODE_SEEK(pos)						\
  do {									\
    register long __new_pos = (pos);					\
    fseek(__infile, __new_pos >> 3, 0);					\
    __buff = getc(__infile);						\
    __btg = 8 - (__new_pos&7);						\
    }									\
  while(0)

#endif
