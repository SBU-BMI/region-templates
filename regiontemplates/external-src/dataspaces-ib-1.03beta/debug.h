/*
 * Ciprian Docan (2010) TASSL Rutgers University
 *
 * The redistribution of the source code is subject to the terms of version 
 * 2 of the GNU General Public License: http://www.gnu.org/licenses/gpl.html.
 */


#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <stdio.h>
#include <errno.h>

#ifdef DEBUG
#define ulog(f, a...) fprintf(stderr, "'%s()': " f, __func__, ##a)
#else
#define ulog(f, a...)
#endif

#define uloga(f, a...)  fprintf(stderr, f, ##a)
#define ulog_err(f, a...) uloga(f ": %s [%d].\n", ##a, strerror(errno), errno)

#define ERROR_TRACE()                                           \
        uloga("'%s()': failed with %d.\n", __func__, err);      \
        return err

#endif
