#ifndef NETORDER_H
#define NETORDER_H

#include "sysfuncs.h"

/* [RPAP - Feb 97: WIN32 Port] */
#ifdef __WIN32__
#include "win32in.h"
#else
# include <netinet/in.h>
#endif


#ifndef WORDS_BIGENDIAN

/* double */
#define HTOND(d)                                                                                  \
        do {                                                                                      \
             unsigned long tmph, tmpl;                                                            \
	     bcopy ((char *) &d, (char *) &tmph, sizeof(double) >> 1);                            \
	     bcopy ((char *) &d + (sizeof(double) >> 1), (char *) &tmpl, sizeof (double) >> 1);   \
	     tmph = htonl (tmph);                                                                 \
	     tmpl = htonl (tmpl);                                                                 \
	     bcopy ((char *) &tmpl, (char *) &d, sizeof (double) >> 1);                           \
	     bcopy ((char *) &tmph, (char *) &d + (sizeof(double) >> 1), sizeof (double) >> 1);   \
	}while(0)
#define NTOHD(d)                                                                                  \
        do {                                                                                      \
             unsigned long tmph, tmpl;                                                            \
	     bcopy ((char *) &d, (char *) &tmph, sizeof(double) >> 1);                            \
	     bcopy ((char *) &d + (sizeof(double) >> 1), (char *) &tmpl, sizeof (double) >> 1);   \
	     tmph = ntohl (tmph);                                                                 \
	     tmpl = ntohl (tmpl);                                                                 \
	     bcopy ((char *) &tmpl, (char *) &d, sizeof (double) >> 1);                           \
	     bcopy ((char *) &tmph, (char *) &d + (sizeof(double) >> 1), sizeof (double) >> 1);   \
        }while(0)
#define HTOND2(hd, nd)                                                                            \
        do {                                                                                      \
             unsigned long tmph, tmpl;                                                            \
	     bcopy ((char *) &hd, (char *) &tmph, sizeof(double) >> 1);                           \
	     bcopy ((char *) &hd + (sizeof(double) >> 1), (char *) &tmpl, sizeof (double) >> 1);  \
	     tmph = htonl (tmph);                                                                 \
	     tmpl = htonl (tmpl);                                                                 \
	     bcopy ((char *) &tmpl, (char *) &nd, sizeof (double) >> 1);                          \
	     bcopy ((char *) &tmph, (char *) &nd + (sizeof(double) >> 1), sizeof (double) >> 1);  \
	}while(0)
#define NTOHD2(nd, hd)                                                                            \
        do {                                                                                      \
             unsigned long tmph, tmpl;                                                            \
	     bcopy ((char *) &nd, (char *) &tmph, sizeof(double) >> 1);                           \
	     bcopy ((char *) &nd + (sizeof(double) >> 1), (char *) &tmpl, sizeof (double) >> 1);  \
	     tmph = ntohl (tmph);                                                                 \
	     tmpl = ntohl (tmpl);                                                                 \
	     bcopy ((char *) &tmpl, (char *) &hd, sizeof (double) >> 1);                          \
	     bcopy ((char *) &tmph, (char *) &hd + (sizeof(double) >> 1), sizeof (double) >> 1);  \
        }while(0)

/* float */
#define HTONF(f)                                                                   \
        do {                                                                       \
             unsigned long tmp;                                                    \
             bcopy ((char *) &(f), (char *) &tmp, sizeof (float));                 \
             HTONUL (tmp);                                                         \
	     bcopy ((char *) &tmp, (char *) &(f), sizeof (float));                 \
	}while(0)
#define NTOHF(f)                                                                   \
        do {                                                                       \
             unsigned long tmp;                                                    \
             bcopy ((char *) &(f), (char *) &tmp, sizeof (float));                 \
	     NTOHUL (tmp);                                                         \
	     bcopy ((char *) &tmp, (char *) &(f), sizeof (float));                 \
	}while(0)
#define HTONF2(hf, nf)                                                             \
        do {                                                                       \
             unsigned long tmp;                                                    \
             bcopy ((char *) &(hf), (char *) &tmp, sizeof (float));                \
             HTONUL (tmp);                                                         \
	     bcopy ((char *) &tmp, (char *) &(nf), sizeof (float));                \
	}while(0)
#define NTOHF2(nf, hf)                                                             \
        do {                                                                       \
             unsigned long tmp;                                                    \
             bcopy ((char *) &(nf), (char *) &tmp, sizeof (float));                \
	     NTOHUL (tmp);                                                         \
	     bcopy ((char *) &tmp, (char *) &(hf), sizeof (float));                \
	}while(0)


#ifdef __MINGW32__

static inline short reverseShort(short s) 
{    
  unsigned char c1 = s & 255;
  unsigned char c2 = (s >> 8) & 255;
    
  return (c1 << 8) | c2;
}

static inline int reverseInt (int i) 
{
  unsigned char c1 = i & 255;
  unsigned char c2 = (i >> 8) & 255;
  unsigned char c3 = (i >> 16) & 255;
  unsigned char c4 = (i >> 24) & 255;

  return ((int)c1 << 24) | ((int)c2 << 16) | ((int)c3 << 8) | c4;
}

#define htonl(l) reverseInt(l)
#define htons(s) reverseShort(s)
#define ntohl(l) reverseInt(l)
#define ntohs(s) reverseShort(s)

#endif

/* pointers */
#define HTONP(p)          ((p) = (void *) htonl ((unsigned long) p))
#define NTOHP(p)          ((p) = (void *) ntohl ((unsigned long) p))
#define HTONP2(hp, np)    ((np) = (void *) htonl ((unsigned long) hp))
#define NTOHP2(np, hp)    ((hp) = (void *) ntohl ((unsigned long) np))

/* unsigned long */
#define HTONUL(l)         ((l) = htonl((l)))
#define NTOHUL(l)         ((l) = ntohl((l)))
#define HTONUL2(hl, nl)   ((nl) = htonl ((hl)))
#define NTOHUL2(nl, hl)   ((hl) = ntohl ((nl)))

/* signed long */
#define HTONSL(l)         ((l) = (long) htonl ((unsigned long) (l)))
#define NTOHSL(l)         ((l) = (long) ntohl ((unsigned long) (l)))
#define HTONSL2(hl, nl)   ((nl) = (long) htonl ((unsigned long) (hl)))
#define NTOHSL2(nl, hl)   ((hl) = (long) ntohl ((unsigned long) (nl)))

/* unsigned int */
#define HTONUI(i)         ((i) = (unsigned int) htonl ((unsigned long) (i)))
#define NTOHUI(i)         ((i) = (unsigned int) ntohl ((unsigned long) (i)))
#define HTONUI2(hi, ni)   ((ni) = (unsigned int) htonl ((unsigned long) (hi)))
#define NTOHUI2(ni, hi)   ((hi) = (unsigned int) ntohl ((unsigned long) (ni)))

/* signed int */
#define HTONSI(i)         ((i) = (int) htonl ((unsigned long) (i)))
#define NTOHSI(i)         ((i) = (int) ntohl ((unsigned long) (i)))
#define HTONSI2(hi, ni)   ((ni) = (int) htonl ((unsigned long) (hi)))
#define NTOHSI2(ni, hi)   ((hi) = (int) ntohl ((unsigned long) (ni)))

/* unsigned short */
#define HTONUS(s)         ((s) = htons((s)))
#define NTOHUS(s)         ((s) = ntohs((s)))
#define HTONUS2(hs, ns)   ((ns) = htons((hs)))
#define NTOHUS2(ns, hs)   ((hs) = ntohs((ns)))

#else   /* WORDS_BIGENDIAN */

/* double */
#define HTOND(d)          (d)
#define NTOHD(d)          (d)
#define HTOND2(hd, nd)    ((hd) = (nd))
#define NTOHD2(nd, hd)    ((nd) = (hd))

/* float */
#define HTONF(f)          (f)
#define NTOHF(f)          (f)
#define HTONF2(hf, nf)    ((nf) = (hf))
#define NTOHF2(nf, hf)    ((hf) = (nf))

/* pointers */
#define HTONP(p)          (p)
#define NTOHP(p)          (p)
#define HTONP2(hp, np)    ((np) = (hp))
#define NTOHP2(np, hp)    ((hp) = (np))

/* unsigned long */
#define HTONUL(l)         (l)
#define NTOHUL(l)         (l)
#define HTONUL2(hl, nl)   ((nl) = (hl))
#define NTOHUL2(nl, hl)   ((hl) = (nl))

/* signed long */
#define HTONSL(l)         (l)
#define NTOHSL(l)         (l)
#define HTONSL2(hl, nl)   ((nl) = (hl))
#define NTOHSL2(nl, hl)   ((hl) = (nl))

/* unsigned int */
#define HTONUI(i)         (i)
#define NTOHUI(i)         (i)
#define HTONUI2(hi, ni)   ((ni) = (hi))
#define NTOHUI2(ni, hi)   ((hi) = (ni))

/* signed int */
#define HTONSI(i)         (i)
#define NTOHSI(i)         (i)
#define HTONSI2(hi, ni)   ((ni) = (hi))
#define NTOHSI2(ni, hi)   ((hi) = (ni))

/* unsigned short */
#define HTONUS(s)         (s)
#define NTOHUS(s)         (s)
#define HTONUS2(hs, ns)   ((ns) = (hs))
#define NTOHUS2(ns, hs)   ((hs) = (ns))



#endif

#endif /* netorder.h */
