/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Modified by Michael Dales (michael@dcs.gla.ac.uk)
 * Date Tue Jun 13 17:34:18 BST 2000
 *
 * Modified for use with my SWARM minimal libc - had to simplify the back end
 * so that it all useds write. Note that I've currently dissabled support
 * for printing quad values.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define	BUF    64 /* MWD: XXX: was (MAXEXP+MAXFRACT+1)  + decimal point */

/*
 * Macros for converting digits to letters and vice versa
 */
#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((char)((n) + '0'))

/*
 * Flags used during conversion.
 */
#define	ALT		0x001		/* alternate form */
#define	HEXPREFIX	0x002		/* add 0x or 0X prefix */
#define	LADJUST		0x004		/* left adjustment */
#define	LONGDBL		0x008		/* long double; unimplemented */
#define	LONGINT		0x010		/* long integer */
#define	QUADINT		0x020		/* quad integer */
#define	SHORTINT	0x040		/* short integer */
#define	ZEROPAD		0x080		/* zero (as opposed to blank) pad */
#define FPT		0x100		/* Floating point number */

#define PRINT(ptr, len)     {\
  if (stream->flags & __SSTR)\
    {\
      if ((stream->data != NULL))\
        {\
          memcpy(stream->data, ptr, len);\
          stream->data += len;\
        }\
    }\
  else\
    write(stream->fd, ptr, len);}
#define PAD(howmany, with)    { \
	if ((n = (howmany)) > 0) { \
		while (n > PADSIZE) { \
			PRINT(with, PADSIZE); \
			n -= PADSIZE; \
		} \
		PRINT(with, n); \
	} \
}
#define FLUSH()

/*
 * To extend shorts properly, we need both signed and unsigned
 * argument extraction methods.
 */
#define	SARG() \
	(flags&QUADINT ? va_arg(ap, quad_t) : \
	    flags&LONGINT ? va_arg(ap, long) : \
	    flags&SHORTINT ? (long)(short)va_arg(ap, int) : \
	    (long)va_arg(ap, int))
#define	UARG() \
	(flags&QUADINT ? va_arg(ap, u_quad_t) : \
	    flags&LONGINT ? va_arg(ap, u_long) : \
	    flags&SHORTINT ? (u_long)(u_short)va_arg(ap, int) : \
	    (u_long)va_arg(ap, u_int))

int vfprintf(FILE* stream, const char *format, va_list ap)
{
  int ret;
  const char *fmt, *cp;
  char ch;
  int n;
  int width;		/* width from format (%8d), or 0 */
  int prec;		/* precision from format (%.3d), or -1 */
  char sign;		/* sign prefix (' ', '+', '-', or \0) */
  int flags;	        /* flags as above */
  char buf[BUF];	/* space for %c, %[diouxX], %[eEfgG] */
  int size;		/* size of converted field or string */
  u_quad_t _uquad;	/* integer arguments %[diouxX] */
  enum { OCT, DEC, HEX } base;/* base for [diouxX] conversion */
  char *xdigs = NULL;	/* digits for [xX] conversion */
  char ox[2];		/* space for 0x hex-prefix */
  int dprec;		/* a copy of prec if [diouxX], 0 otherwise */
  int realsz;		/* field size expanded by dprec */
  char *bp;	/* handy char pointer (short term usage) */

  /*
   * Choose PADSIZE to trade efficiency vs. size.  If larger printf
   * fields occur frequently, increase PADSIZE and make the initialisers
   * below longer.
   */
#define	PADSIZE	16		/* pad chunk size */
  static const char blanks[PADSIZE] =
  {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
  static const char zeroes[PADSIZE] =
  {'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};
  

  if (stream == NULL)
    {
      errno = EBADF;
      return -1;
    }
  if (!stream->write)
    {
      errno = EBADF;
      stream->error = EINVAL;
      return -1;
    }

  ret = 0;
  fmt = format;

  for (;;)
    {
      cp = fmt;
      while (*fmt != '\0')
	{
	  if (*fmt != '%')
	    fmt++;
	  else	    
	    {
	      break;
	    }
	}

      if (fmt != cp)
	{
	  stream->pos += write(stream->fd, cp , fmt - cp);
	}

      if (*fmt == '\0')
	break;

      if ((fmt != format) && (*(fmt - 1) == '\\'))
	{
	  /* This % was a real character */
	  continue;
	}

      /* Skip over % */
      fmt++;

      /* prep for processing tag */
      width = 0;
      prec = -1;
      sign = '\0';  
      flags = 0;  
      dprec = 0;  

    rflag:
      ch = *fmt++;
    reswitch:
      switch (ch)
	{
	case ' ':
	  {
	    /*
	     * ``If the space and + flags both appear, the space
	     * flag will be ignored.''
	     *	-- ANSI X3J11
	     */
	    if (!sign)
	      sign = ' ';
	  }
	  goto rflag;
	case '#':
	  {
	    flags |= ALT;
	  }
	  goto rflag;
	case '*':
	  /*
	   * ``A negative field width argument is taken as a
	   * - flag followed by a positive field width.''
	   *	-- ANSI X3J11
	   * They don't exclude field widths read from args.
	   */
	  if ((width = va_arg(ap, int)) >= 0)
	    goto rflag;
	  width = -width;
	  /* FALLTHROUGH */
	case '-':
	  flags |= LADJUST;
	  goto rflag;
	case '+':
	  sign = '+';
	  goto rflag;
	case '.':
	  if ((ch = *fmt++) == '*') 
	    {
	      n = va_arg(ap, int);
	      prec = n < 0 ? -1 : n;
	      goto rflag;
	    }
	  n = 0;
	  while (is_digit(ch)) 
	    {
	      n = 10 * n + to_digit(ch);
	      ch = *fmt++;
	    }
	  prec = n < 0 ? -1 : n;
	  goto reswitch;
	case '0':
	  /*
	   * ``Note that 0 is taken as a flag, not as the
	   * beginning of a field width.''
	   *	-- ANSI X3J11
	   */
	  flags |= ZEROPAD;
	  goto rflag;
	case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  n = 0;
	  do {
	    n = 10 * n + to_digit(ch);
	    ch = *fmt++;
	  } while (is_digit(ch));
	  width = n;
	  goto reswitch;
#ifdef FLOATING_POINT
	case 'L':
	  flags |= LONGDBL;
	  goto rflag;
#endif
	case 'h':
	  flags |= SHORTINT;
	  goto rflag;
	case 'l':
	  if (*fmt == 'l') {
	    fmt++;
	    flags |= QUADINT;
	  } else {
	    flags |= LONGINT;
	  }
	  goto rflag;
	case 'q':
	  flags |= QUADINT;
	  goto rflag;
	case 'c':
	  *buf = va_arg(ap, int);
	  cp = buf;
	  size = 1;
	  sign = '\0';
	  break;
	case 'D':
	  flags |= LONGINT;
	  /*FALLTHROUGH*/
	case 'd':
	case 'i':
	  _uquad = SARG();	  
	  if ((quad_t)_uquad < 0) {
	    _uquad = -_uquad;
	    sign = '-';
	  }
	  base = DEC;
	  goto number;
#ifdef FLOATING_POINT
	case 'e':
	case 'E':
	case 'f':
	case 'g':
	case 'G':
	  if (prec == -1) {
	    prec = DEFPREC;
	  } else if ((ch == 'g' || ch == 'G') && prec == 0) {
	    prec = 1;
	  }
	  
	  if (flags & LONGDBL) {
	    _double = (double) va_arg(ap, long double);
	  } else {
	    _double = va_arg(ap, double);
	  }
	  
	  /* do this before tricky precision changes */
	  if (isinf(_double)) {
	    if (_double < 0)
	      sign = '-';
	    cp = "Inf";
	    size = 3;
	    break;
	  }
	  if (isnan(_double)) {
	    cp = "NaN";
	    size = 3;
	    break;
	  }
	  
	  flags |= FPT;
	  cp = cvt(_double, prec, flags, &softsign,
		   &expt, ch, &ndig);
	  if (ch == 'g' || ch == 'G') {
	    if (expt <= -4 || expt > prec)
	      ch = (ch == 'g') ? 'e' : 'E';
	    else
	      ch = 'g';
	  } 
	  if (ch <= 'e') {	/* 'e' or 'E' fmt */
	    --expt;
	    expsize = exponent(expstr, expt, ch);
	    size = expsize + ndig;
	    if (ndig > 1 || flags & ALT)
	      ++size;
	  } else if (ch == 'f') {		/* f fmt */
	    if (expt > 0) {
	      size = expt;
	      if (prec || flags & ALT)
		size += prec + 1;
	    } else	/* "0.X" */
	      size = prec + 2;
	  } else if (expt >= ndig) {	/* fixed g fmt */
	    size = expt;
	    if (flags & ALT)
	      ++size;
	  } else
	    size = ndig + (expt > 0 ?
			   1 : 2 - expt);
	  
	  if (softsign)
	    sign = '-';
	  break;
#endif /* FLOATING_POINT */
	case 'n':
	  if (flags & QUADINT)
	    *va_arg(ap, quad_t *) = ret;
	  else if (flags & LONGINT)
	    *va_arg(ap, long *) = ret;
	  else if (flags & SHORTINT)
	    *va_arg(ap, short *) = ret;
	  else
	    *va_arg(ap, int *) = ret;
	  continue;	/* no output */	  
	case 'O':
	  flags |= LONGINT;
	  /*FALLTHROUGH*/
	case 'o':
	  _uquad = UARG();
	  base = OCT;
	  goto nosign;
	case 'p':
	  /*
	   * ``The argument shall be a pointer to void.  The
	   * value of the pointer is converted to a sequence
	   * of printable characters, in an implementation-
	   * defined manner.''
	   *	-- ANSI X3J11
	   */
	  /* NOSTRICT */
	  _uquad = (u_long)va_arg(ap, void *);
	  base = HEX;
	  xdigs = "0123456789abcdef";
	  flags |= HEXPREFIX;
	  ch = 'x';
	  goto nosign;
	case 's':
	  if ((cp = va_arg(ap, char *)) == NULL)
	    cp = "(null)";
	  if (prec >= 0) {
				/*
				 * can't use strlen; can only look for the
				 * NUL in the first `prec' characters, and
				 * strlen() will go further.
				 */
	    char *p = memchr(cp, 0, (size_t)prec);

	    if (p != NULL) {
	      size = p - cp;
	      if (size > prec)
		size = prec;
	    } else
	      size = prec;
	  } else
	    size = strlen(cp);
	  sign = '\0';
	  break;

	case 'U':
	  flags |= LONGINT;
	  /*FALLTHROUGH*/
	case 'u':
	  _uquad = UARG();
	  base = DEC;
	  goto nosign;
	case 'X':
	  xdigs = "0123456789ABCDEF";
	  goto hex;
	case 'x':
	  xdigs = "0123456789abcdef";
	hex:		
	  _uquad = UARG();
	  base = HEX;
	  /* leading 0x/X only if non-zero */
	  if (flags & ALT && _uquad != 0)
	    flags |= HEXPREFIX;
	  
	  /* unsigned conversions */
	nosign:		
	  sign = '\0';
	  /*
	   * ``... diouXx conversions ... if a precision is
	   * specified, the 0 flag will be ignored.''
	   *	-- ANSI X3J11
	   */
	number:		
	  if ((dprec = prec) >= 0)
	    flags &= ~ZEROPAD;

	  /*
	   * ``The result of converting a zero value with an
	   * explicit precision of zero is no characters.''
	   *	-- ANSI X3J11
	   */
	  bp = buf + BUF;
	  if (_uquad != 0 || prec != 0) {
				/*
				 * Unsigned mod is hard, and unsigned mod
				 * by a constant is easier than that by
				 * a variable; hence this switch.
				 */
	    switch (base) 
	      {
	      case OCT:
		do {
		  *--bp = to_char(_uquad & 7);
		  _uquad >>= 3;
		} while (_uquad);
		/* handle octal leading 0 */
		if (flags & ALT && *bp != '0')
		  *--bp = '0';
		break;
		
	      case DEC:
		/* many numbers are 1 digit */
		while (_uquad >= 10) {
		  u_quad_t temp = (_uquad - ((_uquad / 10) * 10));/*_uquad % 10;*/
		  *--bp = to_char(temp);
		  _uquad /= 10;
		}
		*--bp = to_char(_uquad);
		break;
		
	      case HEX:
		do {
		  *--bp = xdigs[(size_t)(_uquad & 15)];
		  _uquad >>= 4;
		} while (_uquad);
		break;
		
	      default:
		cp = "bug in vfprintf: bad base";
		size = strlen(cp);
		goto skipsize;
	      }
	  }
	  cp = bp;
	  size = buf + BUF - bp;
	skipsize:
	  break;
	default:
	  /* "%?" prints ?, unless ? is NUL */
	  if (ch == '\0')
	    goto done;
	  /* pretend it was %c with argument ch */
	  *buf = ch;
	  cp = buf;
	  size = 1;
	  sign = '\0';
	  break;
	}


      /*
       * All reasonable formats wind up here.  At this point, `cp'
       * points to a string which (if not flags&LADJUST) should be
       * padded out to `width' places.  If flags&ZEROPAD, it should
       * first be prefixed by any sign or other prefix; otherwise,
       * it should be blank padded before the prefix is emitted.
       * After any left-hand padding and prefixing, emit zeroes
       * required by a decimal [diouxX] precision, then print the
       * string proper, then emit zeroes required by any leftover
       * floating precision; finally, if LADJUST, pad with blanks.
       *
       * Compute actual size, so we know how much to pad.
       * size excludes decimal prec; realsz includes it.
       */
      realsz = dprec > size ? dprec : size;
      if (sign)
	realsz++;
      else if (flags & HEXPREFIX)
	realsz+= 2;
      
      /* right-adjusting blank padding */
      if ((flags & (LADJUST|ZEROPAD)) == 0)
	PAD(width - realsz, blanks);
      
      /* prefix */
      if (sign) {
	PRINT(&sign, 1);
      } else if (flags & HEXPREFIX) {
	ox[0] = '0';
	ox[1] = ch;
	PRINT(ox, 2);
      }

      /* right-adjusting zero padding */
      if ((flags & (LADJUST|ZEROPAD)) == ZEROPAD)
	PAD(width - realsz, zeroes);
      
      /* leading zeroes from decimal precision */
      PAD(dprec - size, zeroes);
      
      /* the string or number proper */
#ifdef FLOATING_POINT
      if ((flags & FPT) == 0) {
	PRINT(cp, size);
      } else {	/* glue together f_p fragments */
	if (ch >= 'f') {	/* 'f' or 'g' */
	  if (_double == 0) {
	    /* kludge for __dtoa irregularity */
	    PRINT("0", 1);
	    if (expt < ndig || (flags & ALT) != 0) {
	      PRINT(decimal_point, 1);
	      PAD(ndig - 1, zeroes);
	    }
	  } else if (expt <= 0) {
	    PRINT("0", 1);
	    PRINT(decimal_point, 1);
	    PAD(-expt, zeroes);
	    PRINT(cp, ndig);
	  } else if (expt >= ndig) {
	    PRINT(cp, ndig);
	    PAD(expt - ndig, zeroes);
	    if (flags & ALT)
	      PRINT(".", 1);
	  } else {
	    PRINT(cp, expt);
	    cp += expt;
	    PRINT(".", 1);
	    PRINT(cp, ndig-expt);
	  }
	} else {	/* 'e' or 'E' */
	  if (ndig > 1 || flags & ALT) {
	    ox[0] = *cp++;
	    ox[1] = '.';
	    PRINT(ox, 2);
	    if (_double) {
	      PRINT(cp, ndig-1);
	    } else	/* 0.[0..] */
	      /* __dtoa irregularity */
	      PAD(ndig - 1, zeroes);
	  } else	/* XeYYY */
	    PRINT(cp, 1);
	  PRINT(expstr, expsize);
	}
      }
#else
      PRINT(cp, size);
#endif
      /* left-adjusting padding (always blank) */
      if (flags & LADJUST)
	PAD(width - realsz, blanks);
      
      /* finally, adjust ret */
      ret += width > realsz ? width : realsz;
      
      FLUSH();	/* copy out the I/O vectors */
    }
 done:

  return ret;
}
