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

#if HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include <sysfs/libsysfs.h>
#include <edac.h>

/*****************************************************************************
 *  Constants
 *****************************************************************************/

static const char edac_sysfs_path[] =      "/sys/devices/system/edac/mc";
static const char edac_pci_sysfs_path[] =  "/sys/devices/system/edac/pci";

/*****************************************************************************
 *  Data Types
 *****************************************************************************/

enum edac_error {
    EDAC_SUCCESS           = 0,
    EDAC_ERROR             = 1,
    EDAC_OUT_OF_MEMORY     = 2,
    EDAC_BAD_HANDLE        = 3,
    EDAC_OPEN_FAILED       = 4,
    EDAC_MC_OPEN_FAILED    = 5,
    EDAC_CSROW_OPEN_FAILED = 6
};

struct edac_handle {
    int                   initialized;      /* 1 if structure is valid       */
    struct sysfs_device * dev;              /* sysfs device handle           */
    struct sysfs_device *  pci;             /* sysfs edac/pci/ device handle */
    struct dlist *         mc_list;         /* list of memory controllers    */
    int                    ce_total;        /* Total corrected errors        */
    int                    ue_total;        /* Total uncorrected errors      */
    int                    pci_parity_count;/* Total PCI parity errors       */
    int                    totals_valid;    /* 1=totals valid 0=not          */
    int                    error_num;       /* Last library error            */
    char *                 error_str;       /* Last error string             */
};

struct edac_mc {
    struct edac_handle *   edac;            /* Pointer back to EDAC handle   */
    struct edac_mc_info    info;            /* EDAC MC error info            */
    struct sysfs_device *  dev;             /* sysfs device handle           */
    struct dlist *         csrow_list;      /* list of csrows for this mc    */
};

struct edac_csrow {
    struct edac_mc *       mc;             /* Pointer back to MC             */
    struct sysfs_device *  dev;            /* sysfs device handle            */
    struct edac_csrow_info info;           /* EDAC csrow error info          */
};

typedef void (*del_f) (void *);


/*****************************************************************************
 *  Prototypes
 *****************************************************************************/

static int edac_handle_reload (edac_handle *edac);

static inline void edac_dlist_reset (struct dlist *l);

static struct dlist * mc_list_create (edac_handle *edac);

static int edac_totals_refresh (edac_handle *edac);

static inline void * edac_dlist_next (struct dlist *l);

static struct sysfs_device * _sysfs_open_device_tree (const char *path);

static int get_sysfs_string_attr (struct sysfs_device *dev, char *dest, 
        int len, const char *format, ...);

static int get_sysfs_uint_attr (struct sysfs_device *dev, unsigned int *valp, 
        const char *format, ...);


/*****************************************************************************
 *  Extern Functions
 *****************************************************************************/

edac_handle * edac_handle_create (void)
{
    edac_handle *edac = malloc (sizeof (*edac));

    if (!edac)
        return (NULL);

    memset (edac, 0, sizeof (*edac));

    return (edac);
}
int edac_handle_init (struct edac_handle *edac)
{
    if (edac == NULL)
        return (-1);

    if (edac->initialized) {
        return (edac_handle_reload (edac));
    }

    if (!(edac->dev = _sysfs_open_device_tree (edac_sysfs_path))) {
        edac->error_num = EDAC_OPEN_FAILED;
        return (-1);
    }

    edac->pci = sysfs_open_device_path (edac_pci_sysfs_path);
    /* XXX: Ignore errors? */

    if (!(edac->mc_list = mc_list_create (edac))) {
        edac->error_num = EDAC_MC_OPEN_FAILED;
        return (-1);
    }

    edac->initialized = 1;

    return (0);
}

unsigned int 
edac_mc_count (edac_handle *edac)
{
    if (!edac->initialized) {
        edac_handle_init (edac);
    }
    if (edac->mc_list != NULL) {
        return (edac->mc_list->count);
    }
    return (0);
}

void edac_handle_destroy (edac_handle *edac)
{
    if (edac->mc_list)
        dlist_destroy (edac->mc_list);
    if (edac->dev)
        sysfs_close_device_tree (edac->dev); 
    if (edac->pci)
        sysfs_close_device (edac->pci);
    free (edac);
    return;
}

const char * edac_strerror (edac_handle *edac)
{
    switch (edac->error_num) {
        case EDAC_SUCCESS:
            return ("Success");
        case EDAC_ERROR:
            return ("Internal error");
        case EDAC_OUT_OF_MEMORY:
            return ("Out of memory");
        case EDAC_BAD_HANDLE:
            return ("Invalid EDAC library handle");
        case EDAC_OPEN_FAILED:
            return ("Unable to find EDAC data in sysfs");
        case EDAC_MC_OPEN_FAILED:
            return ("Unable to open EDAC memory controller in sysfs");
        case EDAC_CSROW_OPEN_FAILED:
            return ("Unable to open csrow in sysfs");
        default:
            break;
    }

    return ("Unknown error");
}

int edac_handle_reset (edac_handle *edac)
{
    if (edac->mc_list) 
        edac_dlist_reset (edac->mc_list);
    return (0);
}

int edac_error_totals (edac_handle *edac, struct edac_totals *tot)
{
    if ((edac == NULL) || (tot == NULL)) {
        errno = EINVAL;
        return (-1);
    }

    memset (tot, 0, sizeof (*tot));

    if (!edac->totals_valid)  {
        if (edac_totals_refresh (edac) < 0) {
            return (-1);
        }
    }

    tot->ue_total = edac->ue_total;
    tot->ce_total = edac->ce_total;
    tot->pci_parity_total = edac->pci_parity_count;
    
    return (0);
}

edac_mc * edac_next_mc (edac_handle *edac)
{
    if ((edac == NULL) || (edac->mc_list == NULL))
        return NULL;
    return (edac_dlist_next (edac->mc_list));
}

edac_mc * edac_next_mc_info (edac_handle *edac, struct edac_mc_info *info)
{
    edac_mc * mc = edac_next_mc (edac);
    edac_mc_get_info (mc, info);
    return (mc);
}

int edac_mc_get_info (edac_mc *mc, struct edac_mc_info *info)
{
    if (mc == NULL)
        return (-1);

    if (info == NULL)
        return (-1);

    *info = mc->info;

    return (0);

}

edac_csrow * edac_next_csrow (struct edac_mc *mc)
{
    if (mc == NULL)
        return NULL;

    return (edac_dlist_next (mc->csrow_list));
}

edac_csrow * 
edac_next_csrow_info (struct edac_mc *mc, struct edac_csrow_info *info)
{
    edac_csrow * csrow = edac_next_csrow (mc);
    edac_csrow_get_info (csrow, info);
    return (csrow);
}

int edac_csrow_get_info (edac_csrow *csrow, struct edac_csrow_info *info)
{
    if (!csrow || !info)
        return (-1);

    *info = csrow->info;

    return (0);
}

int edac_mc_reset (struct edac_mc *mc)
{
    if (mc == NULL)
        return (-1);

    edac_dlist_reset (mc->csrow_list);

    return (0);
}


/*****************************************************************************
 *  Private Functions
 *****************************************************************************/

/*  Like dlist_next(), but return current data, then advance marker.
 */
static inline void * edac_dlist_next (struct dlist *l)
{
    void *data = l->marker->data;
    dlist_next (l);
    return (data);
}

static inline void edac_dlist_reset (struct dlist *l)
{
    dlist_start (l);
    dlist_next (l);
}

static inline void remove_newline (char *str)
{
    int len = strlen (str);

    if (len && str[len - 1] == '\n')
        str[len - 1] = '\0';
}

static int
edac_channel_refresh (struct edac_csrow *csrow, int id)
{
    struct sysfs_device *dev =  csrow->dev;
    struct edac_channel *chan = &csrow->info.channel[id];
    int                  rc;

    /* On some EDAC implementations ch1_* files may exist
     *  even though nr_channels = 1. Returning an error here
     *  should suffice to mark the channel invalid.
     */

    if (get_sysfs_uint_attr (dev, &chan->ce_count, "ch%d_ce_count", id) < 0) 
        return (-1);
    rc = get_sysfs_string_attr ( dev, chan->dimm_label, 
                                 sizeof (chan->dimm_label),
                                 "ch%d_dimm_label", id );

    if (  (rc >= 0) 
       && (chan->dimm_label[0] != '\0') 
       && (chan->dimm_label[0] != '\n') ) {
        chan->dimm_label_valid = 1;
    }

    remove_newline (chan->dimm_label);

    chan->valid = 1;
    return (0);
}

static int 
edac_csrow_refresh (struct edac_csrow *csrow)
{
    struct sysfs_device *    dev =   csrow->dev;
    struct edac_csrow_info * info = &csrow->info;
    int                      i;

    strncpy (info->id, dev->name, sizeof (info->id) - 1);

    if (get_sysfs_uint_attr (dev, &info->size_mb, "size_mb") < 0)
        return -1;
    if (get_sysfs_uint_attr (dev, &info->ce_count, "ce_count") < 0)
        return -1;
    if (get_sysfs_uint_attr (dev, &info->ue_count, "ue_count") < 0)
        return -1;

    for (i = 0; i < EDAC_MAX_CHANNELS; i++) {
        edac_channel_refresh (csrow, i);
    }
    
    return 0;
}

static int 
edac_mc_refresh (struct edac_mc *mc)
{
    struct sysfs_device *dev =  mc->dev;
    struct edac_mc_info *i = &mc->info;
    char *               p;

    if (get_sysfs_uint_attr (dev, &i->size_mb, "size_mb") < 0) 
        return (-1);
    if (get_sysfs_uint_attr (dev, &i->ce_count, "ce_count") < 0) 
        return (-1);
    if (get_sysfs_uint_attr (dev, &i->ue_count, "ue_count") < 0)
        return (-1);
    if (get_sysfs_uint_attr (dev, &i->ce_noinfo_count, "ce_noinfo_count") < 0)
        return (-1);
    if (get_sysfs_uint_attr (dev, &i->ue_noinfo_count, "ue_noinfo_count") < 0)
        return (-1);

    get_sysfs_string_attr (dev, i->mc_name, sizeof (i->mc_name), "mc_name");

    if (*(p = i->mc_name + strlen (i->mc_name) - 1) == '\n')
        *p = '\0';

    return (0);
}

static void edac_mc_destroy (struct edac_mc *mc)
{
    if (mc == NULL)
        return;
    if (mc->csrow_list)
        dlist_destroy (mc->csrow_list);
    free (mc);
    return;
}

static struct edac_csrow * 
edac_csrow_create (edac_mc *mc, struct sysfs_device *dev)
{
    struct edac_csrow *csrow = NULL;

    if (strncmp ("csrow", dev->name, 5) != 0)
        return NULL;

    if ((csrow = malloc (sizeof (*csrow))) == NULL)
        return NULL;

    memset (csrow, 0, sizeof (*csrow));

    csrow->dev = dev;
    csrow->mc  = mc;

    edac_csrow_refresh (csrow);
    return (csrow);
}

static void edac_csrow_destroy (struct edac_csrow *csrow)
{
    free (csrow);
    return;
}


static struct edac_mc *
edac_mc_create (edac_handle *edac, struct sysfs_device *dev)
{
    struct edac_mc *mc;

    if (dev->name[0] != 'm' || dev->name[1] != 'c')
        return NULL;

    if ((mc = malloc (sizeof (*mc))) == NULL)
        return NULL;

    memset (mc,  0, sizeof (*mc) );

    mc->dev = dev;
    mc->edac = edac;

    strncpy (mc->info.id, dev->name, sizeof (mc->info.id) - 1);

    if (edac_mc_refresh (mc) < 0)
        goto error_out;

    mc->csrow_list = dlist_new_with_delete (sizeof (struct edac_csrow), 
                                             (del_f) edac_csrow_destroy);

    if (dev->children) {
        struct sysfs_device *child = NULL;
        dlist_for_each_data (dev->children, child, struct sysfs_device) {
            struct edac_csrow *csrow;
            if ((csrow = edac_csrow_create (mc, child))) {
                dlist_push (mc->csrow_list, csrow);
            }
        }
    }

    edac_dlist_reset (mc->csrow_list);

    return (mc);

   error_out:
    edac_mc_destroy (mc);
    return NULL;

}

static int 
get_sysfs_uint_attr (struct sysfs_device *dev, unsigned int *valp, 
        const char *format, ...)
{
    char *                    p;
    va_list                   ap;
    char                      buf[1024];
    struct sysfs_attribute *  attr;
    int                       n;

    memset (buf, '\0', sizeof (buf));

    va_start (ap, format);
    n = vsnprintf (buf, sizeof (buf) - 1, format, ap); 
    va_end (ap);

    if ((n < 0) || (n > sizeof (buf)))
        return (-1);
 
    if (!(attr = sysfs_get_device_attr (dev, buf))) {
        return (-1);
    }

    *valp = strtoul (attr->value, &p, 10);
    /*
     * XXX: Check for valid number?
     */

    return (0);

}

static int
get_sysfs_string_attr (struct sysfs_device *dev, char *dest, int len,
                      const char *format, ...)
{
    va_list                   ap;
    char                      buf[1024];
    char *                    nl;
    struct sysfs_attribute *  attr;
    int                       n;

    memset (buf, '\0', sizeof (buf));

    va_start (ap, format);
    n = vsnprintf (buf, sizeof (buf) - 1, format, ap); 
    va_end (ap);

    if ((n < 0) || (n > sizeof (buf)))
        return (-1);

    if (!(attr = sysfs_get_device_attr (dev, buf))) {
        return (-1);
    }

    /*  Terminate any final newline 
     */
    if ((nl = strrchr (attr->value, '\n')))
        len = (nl - attr->value) + 1;


    memset (dest, '\0', len);
    strncpy (dest, attr->value, len - 1);
    return (0);
}

static struct dlist *
mc_list_create (edac_handle *edac)
{
    struct dlist *l;
    struct sysfs_device *dev;
    size_t size = sizeof (struct edac_mc);

    if (!(l = dlist_new_with_delete (size, (del_f) edac_mc_destroy)))
        return NULL;

    if (edac->dev->children) {
        dlist_for_each_data (edac->dev->children, dev, struct sysfs_device) {
            struct edac_mc *mc;
            if ((mc = edac_mc_create (edac, dev)))
                dlist_push (l, mc);
        }
    }

    edac_dlist_reset (l); 

    return (l);
}

static int edac_totals_refresh (edac_handle *edac)
{
    struct dl_node *i;


    if (edac->pci) {
        int rc = get_sysfs_uint_attr (edac->pci, 
                           (unsigned int *) &edac->pci_parity_count, 
                           "pci_parity_count");
        if (rc < 0)
            return (-1);
    }

    if (edac->mc_list->count == 0) {
        edac->error_num = EDAC_MC_OPEN_FAILED;
        return (-1);
    }

    dlist_for_each_nomark (edac->mc_list, i) {
        struct edac_mc *mc = i->data;
        /* edac_mc_refresh (mc); */
        edac->ue_total += mc->info.ue_count;
        edac->ce_total += mc->info.ce_count;
    }

    edac->totals_valid = 1;

    return (0);
}

static int edac_handle_reload (edac_handle *edac)
{
    if (!edac->mc_list) {
        edac->error_num = EDAC_BAD_HANDLE;
        return (-1);
    }

    dlist_destroy (edac->mc_list);

    if (!(edac->mc_list = mc_list_create (edac))) {
        edac->error_num = EDAC_MC_OPEN_FAILED;
        return (-1);
    }

    edac_handle_reset (edac);

    return (0);
}


#if HAVE_SYSFS_OPEN_DEVICE_TREE

static struct sysfs_device *
_sysfs_open_device_tree (const char *path)
{
    return (sysfs_open_device_tree (path));
}

#else

/*  libsysfs-2.0.0 did not implement sysfs_open_device_tree(). Replicate here.
 */

static void
_null (void *x)
{
    return;
}

static void
_dev_create_child_list (struct sysfs_device *dev)
{
    dev->children = dlist_new_with_delete (sizeof (struct sysfs_device), _null);
}

static struct sysfs_device * 
_sysfs_open_device_tree (const char *path)
{
    struct sysfs_device * dev;
    struct sysfs_device * child;
    struct dlist *        dirs;
    char *                subdir;

    if (!(dev = sysfs_open_device_path (path)))
        return (NULL);

    if (!(dirs = sysfs_open_directory_list (path)))
        return (dev);

    dlist_for_each_data (dirs, subdir, char) {
        int  n;
        char fq_subdir [SYSFS_PATH_MAX + 1];

        memset (fq_subdir, 0, sizeof (fq_subdir));

        if (!sysfs_path_is_dir (subdir)) {
            continue;
        }

        n = snprintf (fq_subdir, SYSFS_PATH_MAX, "%s/%s", path, subdir);

        if ((n < 0) || (n > SYSFS_PATH_MAX)) {
            continue;
        }

        if (!(child = _sysfs_open_device_tree (fq_subdir))) {
            continue;
        }

        if (!dev->children) {
            _dev_create_child_list (dev);
        }

        dlist_push (dev->children, child);
    }

    sysfs_close_list (dirs);

    return (dev);
}
#endif /* HAVE_SYSFS_OPEN_DEVICE_TREE */


/* vi: ts=4 sw=4 expandtab 
 */
