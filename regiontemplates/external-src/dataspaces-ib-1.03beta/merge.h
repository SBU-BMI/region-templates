/*
 * Header file to account for differences between portals data type
 * implementation on ORNL and local machine
 * 
 * Ciprian Docan (2006-2009) TASSL Rutgers University
 *
 * The redistribution of the source code is subject to the terms of version 
 * 2 of the GNU General Public License: http://www.gnu.org/licenses/gpl.html.
 *
 */

#ifndef __MERGE_H_
#define __MERGE_H_

#include "config.h"

#ifdef HAVE_CRAY_PORTALS

#include "portals/portals3.h"
// #       include "catamount/cnos_mpi_os.h"
#include <stdint.h>
#define PTL_EQ_HANDLER_NONE NULL
#define	PTL_NO_ACK_REQ	PTL_NOACK_REQ
// typedef uint32_t ptl_time_t;
// typedef void (*ptl_eq_handler_t)(ptl_event_t *event);
#else

#include "portals3.h"
#include "p3nal_utcp.h"
#include "p3api/debug.h"

#define PTL_EVENT_MD_UNLINK     PTL_EVENT_UNLINK
#define PTL_IFACE_DUP           PTL_OK

typedef uint32_t UInt32;
#endif /* HAVE_CRAY_PORTALS */


#endif
