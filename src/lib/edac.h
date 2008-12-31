/*****************************************************************************
 *  $Id$
 *****************************************************************************
 *  Copyright (C) 2005-2007 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Mark Grondona <mgrondona@llnl.gov>
 *  UCRL-CODE-230739.
 *
 *  This file is part of edac-utils.
 *
 *  This is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 *****************************************************************************/
#ifndef _LIBEDAC_H
#define _LIBEDAC_H

/*****************************************************************************
 *  C++ macros
 *****************************************************************************/

#undef BEGIN_C_DECLS
#undef END_C_DECLS
#ifdef __cplusplus
#  define BEGIN_C_DECLS         extern "C" {
#  define END_C_DECLS           }
#else  /* !__cplusplus */
#  define BEGIN_C_DECLS         /* empty */
#  define END_C_DECLS           /* empty */
#endif /* !__cplusplus */

/*****************************************************************************
 *  Defines
 *****************************************************************************/

#define EDAC_NAME_LEN      64
#define EDAC_LABEL_LEN    256
#define EDAC_MAX_CHANNELS   6

#define edac_for_each_mc_info(__h, __mc, __i) \
    for (edac_handle_reset (__h), __mc = edac_next_mc_info (__h, &__i); \
         __mc != NULL; \
         __mc = edac_next_mc_info (__h, &__i) \
        ) 

#define edac_for_each_csrow_info(__mc, __csrow, __i) \
    for (edac_mc_reset (__mc), __csrow = edac_next_csrow_info (__mc, &__i); \
         __csrow != NULL; \
         __csrow = edac_next_csrow_info (__mc, &__i) \
        ) 

/*****************************************************************************
 *  Data Types
 *****************************************************************************/

/*  EDAC Opaque types
 */

/*  EDAC library handle
 */
typedef struct edac_handle edac_handle;

/*  EDAC memory controller 
 */
typedef struct edac_mc edac_mc;

/*  EDAC csrow within an MC
 */
typedef struct edac_csrow  edac_csrow;

/*  EDAC memory controller info
 */
struct edac_mc_info {
    char           id[EDAC_NAME_LEN];       /* Id of memory controller (mcN) */
    char           mc_name[EDAC_NAME_LEN];  /* Name of MC (e.g. "E7525")     */
    unsigned int   size_mb;                 /* Amount of RAM in MB           */
    unsigned int   ce_count;                /* Corrected error count         */
    unsigned int   ce_noinfo_count;         /* Corrected errors w/ no info   */
    unsigned int   ue_count;                /* Uncorrected error count       */
    unsigned int   ue_noinfo_count;         /* Uncorrected errors w/ no info */
};

struct edac_channel {
    int           valid;                    /* Is this channel valid         */
    unsigned int  ce_count;                 /* Corrected error count         */
    int           dimm_label_valid;         /* Is DIMM label valid?          */
    char          dimm_label[EDAC_LABEL_LEN]; 
                                            /* DIMM name                     */
};

/*  EDAC row information
 */
struct edac_csrow_info {
    char          id[EDAC_NAME_LEN];       /* CSROW Identity (e.g. csrow0)  */
    unsigned int  size_mb;                 /* CSROW size in MB              */
    unsigned int  ce_count;                /* Total corrected errors        */
    unsigned int  ue_count;                /* Total uncorrected errors      */
    struct edac_channel channel[EDAC_MAX_CHANNELS];
                                           /* Channel info for this csrow   */
};


/*  EDAC error totals
 */
struct edac_totals {
    unsigned int   ce_total;                /* Total corrected errors        */
    unsigned int   ue_total;                /* Total uncorrected errors      */
    unsigned int   pci_parity_total;        /* Total PCI Parity errors       */
};

/*****************************************************************************
 *  Functions
 *****************************************************************************/

BEGIN_C_DECLS

/*
 *  Initialize an EDAC library handle instance. 
 *  Does not read any EDAC data from /sys. Must be followed by a call
 *   to edac_handle_init ().
 *  Returns NULL on failure to allocate memory
 */
edac_handle * edac_handle_create (void);

/*
 *  Load system EDAC data from /sys (and possibly elsewhere) into
 *   the EDAC handle. Must be called at least once. More than one
 *   call to this function will reload EDAC values into the handle
 *   and reset MC counters.
 *  
 */   
int edac_handle_init (edac_handle *edac);

/*
 *  Returns the number of EDAC memory controllers found in /sys
 *   0 if none found (e.g. edac_mc loaded, but no chipset specific driver)
 */
unsigned int edac_mc_count (edac_handle *edac);

/*
 *  Destroy an EDAC handle. Frees associated memory.
 */
void edac_handle_destroy (edac_handle *edac); /*
 *  Return a descriptive text string describing the last error for
 *   the EDAC library handle `edac'.
 */
const char * edac_strerror (edac_handle *edac);

/*
 *  Reset internal iterator in EDAC handle used for iterating over
 *    multiple memory controllers.
 */
int edac_handle_reset (edac_handle *edac);

/*
 *  Return error count totals for all memory controller in totals.
 *   Return <0 on error.
 */
int edac_error_totals (edac_handle * edac, struct edac_totals *totals);

/*
 *  Returns next memory controller fron EDAC context, or NULL
 *   if no more MCs.
 */
edac_mc * edac_next_mc (edac_handle * edac);

/*
 *  Get EDAC memory controller info
 */
int edac_mc_get_info (edac_mc *mc, struct edac_mc_info *info);

/*
 *  Combined edac_next_mc () and edac_mc_get_info ().
 */
edac_mc * edac_next_mc_info (edac_handle *edac, struct edac_mc_info *info);

/*
 *  Returns next csrow in EDAC memory controller, or NULL
 *   if no more csrows.
 */
edac_csrow * edac_next_csrow (struct edac_mc *mc);

/*
 *  Get EDAC csrow info 
 */
int edac_csrow_get_info (edac_csrow *, struct edac_csrow_info *info);

/*
 *  Combined edac_next_csrow () and edac_csrow_get_info ().
 */
edac_csrow * edac_next_csrow_info (edac_mc *mc, struct edac_csrow_info *info);

/*
 *  Reset internal iterator in memory controller for looping through
 *   csrow information.
 */
int edac_mc_reset (struct edac_mc *mc);


END_C_DECLS

#endif /* !_LIBEDAC_H */

/* vi: ts=4 sw=4 expandtab 
 */
