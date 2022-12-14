/*	$NetBSD: sprintf.c,v 1.6 1997/07/13 20:15:29 christos Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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

#if 0
#include <sys/cdefs.h>
#endif
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)sprintf.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: sprintf.c,v 1.6 1997/07/13 20:15:29 christos Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <stdio.h>
#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <limits.h>
#if 0
#include "local.h"
#endif

int
#if __STDC__
sprintf(char *str, char const *fmt, ...)
#else
sprintf(str, fmt, va_alist)
	char *str;
	char *fmt;
	va_dcl
#endif
{
	int ret;
	va_list ap;
	FILE f;

	f.flags = __SSTR;
	f.data = str;
	f.write = 1;
	f.read = 1;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	ret = vfprintf(&f, fmt, ap);
	va_end(ap);
	*(f.data) = 0;
	return (ret);
}
