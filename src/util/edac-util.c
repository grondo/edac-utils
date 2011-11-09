/*****************************************************************************
 *  $Id: edac-util.c 51 2007-05-07 21:39:23Z grondo $
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
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h> 
#include <edac.h>

#include "list.h"
#include "split.h"

/*****************************************************************************
 *  Command-Line Options
 *****************************************************************************/

#include <getopt.h>
struct option opt_table[] = {
    { "help",         0, NULL, 'h' },
    { "quiet",        0, NULL, 'q' },
    { "verbose",      0, NULL, 'v' },
    { "report",       2, NULL, 'r' },
    { "status",       0, NULL, 's' },
    {  NULL,          0, NULL,  0  }
};

const char * const opt_string = "hqvsr::";

#define USAGE "\
Usage: %s [OPTIONS]\n\
  -h, --help           Display this help\n\
  -q, --quiet          Display only non-zero error counts and fatal errors\n\
  -v, --verbose        Increase verbosity. Multiple -v's may be used\n\
  -s, --status         Display EDAC status\n\
  -r, --report=REPORT  Display EDAC error report REPORT\n\
  \n\
Valid REPORT types are default, simple, full, ue, ce\n"
  

/*****************************************************************************
 *  Data Types
 *****************************************************************************/

/*  Program context
 */
struct prog_ctx {
    char *progname;
    struct edac_handle *edac;
    int verbose;
    int quiet;
    int print_status;
    List reports;
};

typedef void (*report_f) (struct prog_ctx *);

struct report {
    int   id;
    report_f report;
    char *name;
};

enum report_type {
    EDAC_REPORT_DEFAULT,
    EDAC_REPORT_SIMPLE,
    EDAC_REPORT_FULL,
    EDAC_REPORT_UE,
    EDAC_REPORT_CE,
    EDAC_REPORT_PCI
};
/*****************************************************************************
 *  Globals
 *****************************************************************************/

static struct prog_ctx prog_ctx = { NULL, NULL, 0, 0, 0, NULL }; 


/*  Report prototypes
 */
static void default_report (struct prog_ctx *);
static void simple_report (struct prog_ctx *);
static void full_report (struct prog_ctx *);
static void ue_report (struct prog_ctx *);
static void ce_report (struct prog_ctx *);
static void pci_report (struct prog_ctx *ctx);

static struct report report_table[] = {
    { EDAC_REPORT_DEFAULT, (report_f) default_report, "default" },
    { EDAC_REPORT_SIMPLE,  (report_f) simple_report,  "simple"  },
    { EDAC_REPORT_FULL,    (report_f) full_report,    "full"    },
    { EDAC_REPORT_UE,      (report_f) ue_report,      "ue"      },
    { EDAC_REPORT_CE,      (report_f) ce_report,      "ce"      },
    { EDAC_REPORT_PCI,     (report_f) pci_report,     "pci"     },
    { -1,                  NULL,                       NULL     }
};


/*****************************************************************************
 *  Prototypes
 *****************************************************************************/

static int prog_ctx_init (struct prog_ctx *ctx, const char *progname);

static void prog_ctx_fini (struct prog_ctx *);

static void parse_cmdline (struct prog_ctx *, int ac, char **av);

static List list_append_from_string (List l, char *str);

static List report_list_create (List l);

static void generate_reports (struct prog_ctx *ctx);

static int print_status (struct prog_ctx *ctx);

static void usage (void);

static void log_fatal (int errnum, const char *format, ...);

static void log_err (const char *format, ...);

static void log_verbose (const char *format, ...);

static void log_msg (const char *format, ...);

#if 0
static void log_debug (const char *format, ...);
#endif


/*****************************************************************************
 *  Functions
 *****************************************************************************/

int main (int ac, char *av[])
{
    char *prog;
   
    prog = (prog = strrchr (av[0], '/')) ? prog + 1 : av[0];
    
    prog_ctx_init (&prog_ctx, prog);

    parse_cmdline (&prog_ctx, ac, av);

    if (edac_handle_init (prog_ctx.edac) < 0) {
        log_fatal (1, "Unable to get EDAC data: %s\n", 
                edac_strerror (prog_ctx.edac));
    }

    if (prog_ctx.print_status) {
        return (print_status (&prog_ctx));
    }

    if (edac_mc_count (prog_ctx.edac)) {
        generate_reports (&prog_ctx);
    }
    else {
        log_err ("No memory controller data found.\n");
    }

    prog_ctx_fini (&prog_ctx);

    return (0);
}

static int
prog_ctx_init (struct prog_ctx *ctx, const char *prog)
{
    ctx->progname = strdup (prog);

    if (!(ctx->edac = edac_handle_create ())) {
        log_fatal (1, "Unable to create EDAC handle\n");
    }
    ctx->verbose = 0;
    ctx->quiet =   0;

    return (0);
}

static void
prog_ctx_fini (struct prog_ctx *ctx)
{
    if (ctx->reports)
        list_destroy (ctx->reports);
    if (ctx->edac)
        edac_handle_destroy (ctx->edac);
    if (ctx->progname)
        free (ctx->progname);
    return;
}

static void
parse_cmdline (struct prog_ctx *ctx, int ac, char **av)
{
    int   c;
    List  l;

    opterr = 0; 
    l =      NULL;

    while (1) {

        c = getopt_long (ac, av, opt_string, opt_table, NULL);

        if (c == -1) 
            break;

        switch (c) {
            case 'h' :
                usage ();
                exit (0);
                break;
            case 'q':
                ctx->quiet++;
                break;
            case 'v':
                ctx->verbose++;
                break;
            case 's':
                ctx->print_status = 1;
                break;
            case 'r':
                if (optarg)
                    l = list_append_from_string (l, optarg);
                else
                    l = list_append_from_string (l, "default");
                break;
            case '?':
                if (optopt > 0) {
                    log_fatal (1, "Invalid option \"-%c\"\n", optopt);
                } else {
                    log_fatal (1, "Invalid option \"%s\"\n", av[optind - 1]);
                }
                break;
            default:
                log_fatal (1, "Unimplemented option \"%s\"\n", 
                        av[optind - 1]);
                break;
        }
    }

    if (av[optind]) {
        log_fatal (1, "Unrecognized parameter \"%s\"\n", av[optind]);
    }

    if ((l != NULL) && (ctx->print_status)) {
        log_fatal (1, "Only specify one of --report or --status\n");
    }

    if (l == NULL)
        l = list_append_from_string (l, "default");

    if (!(ctx->reports = report_list_create (l)))
        exit (1);

    list_destroy (l);

    return;
}

static int
print_status (struct prog_ctx *ctx)
{
    edac_mc *           mc;
    struct edac_mc_info mci;
    char                buf [1024];
    char *              p;
    int                 count;
    int                 n;
    int                 len;

    if (!ctx->edac) {
        log_msg ("EDAC info unavailable.\n");
        return (1);
    } 

    if ((count = edac_mc_count (ctx->edac)) == 0) {
        log_msg ("EDAC drivers loaded. No memory controllers found\n");
        return (1);
    }

    if (!ctx->verbose) {
        log_msg ("EDAC drivers are loaded. %u MC%sdetected\n",
                count, (count > 1) ? "s " : " ");
        return (0);
    }

    p =   buf;
    len = sizeof (buf);

    n = snprintf (p, len, "EDAC drivers are loaded. %u MC%sdetected:\n", 
            count, (count > 1) ? "s " : " ");

    if ((n < 0) || (n >= len)) {
        p += len - 1;
        len = 0;
    }
    else {
        p += n;
        len -= n;
    }

    edac_for_each_mc_info (ctx->edac, mc, mci) {
        if (mci.mc_name[0] != '\0' || mci.mc_name[0] != '\n') {

            n = snprintf (p, len, "  %s:%s\n", mci.id, mci.mc_name);

            if ((n < 0) || (n >= len)) {
                p += len - 1;
                len = 0;
            }
            else {
                p += n;
                len -= n;
            }
        }
    }

    *p = '\0';

    fprintf (stdout, "%s: %s\n", ctx->progname, buf);

    return (0);
}

static struct report *
get_report_by_name (const char *str)
{
    struct report *r;

    for (r = report_table; r->name != NULL; r++) {
        if (strncmp (str, r->name, strlen (str)) == 0)
            return (r);
    }
    
    return (NULL);
}

static int 
report_find (struct report *r, char *name)
{
    if (strcmp (r->name, name) == 0)
        return (1);
    return (0);
}

static List
report_list_create (List l)
{
    List reports;
    ListIterator i;
    char *str;
    int got_err = 0;

    if (l == NULL)
        return (NULL);

    /*  No destroy needed for reports list
     */
    reports = list_create (NULL);

    i = list_iterator_create (l);
    while ((str = list_next (i))) {
        struct report *r = get_report_by_name (str);
        if (r == NULL) {
            log_err ("Invalid report: \"%s\"\n", str);
            got_err = 1;
        } 
        else {
            if (!list_find_first (reports, (ListFindF) report_find, r->name))
                list_append (reports, r);
        }
    }
    list_iterator_destroy (i);

    if (got_err) {
        list_destroy (reports);
        return (NULL);
    }

    return (reports);
}

static void generate_reports (struct prog_ctx *ctx)
{
    ListIterator   i;
    struct report *r;
    
    if (!ctx->reports)
        log_fatal (1, "No reports requested!");

    i = list_iterator_create (ctx->reports);
    while ((r = list_next (i))) {
        if (r->report != NULL)
            r->report (ctx);
    }
    list_iterator_destroy (i);
    return;
}

static void default_report (struct prog_ctx *ctx)
{
    edac_mc *mc;
    edac_csrow *csrow;
    struct edac_mc_info mci;
    struct edac_csrow_info csi;
    int count = 0;
    int i;

    edac_for_each_mc_info (ctx->edac, mc, mci) {

        if (mci.ue_noinfo_count || ctx->verbose)
            fprintf (stdout, "%s: %u Uncorrected Errors with no DIMM info\n",
                    mci.id, mci.ue_noinfo_count);

        if (mci.ce_noinfo_count || ctx->verbose)
            fprintf (stdout, "%s: %u Corrected Errors with no DIMM info\n",
                    mci.id, mci.ce_noinfo_count);

        count += mci.ce_noinfo_count + mci.ue_noinfo_count;
 
        edac_for_each_csrow_info (mc, csrow, csi) {

            count += csi.ue_count;

            if (csi.ue_count || ctx->verbose)
                fprintf (stdout, 
                        "%s: %s: %u Uncorrected Errors\n",
                        mci.id, csi.id, csi.ue_count);

            for (i = 0; i < EDAC_MAX_CHANNELS; i++) {
                struct edac_channel *ch = &csi.channel[i];
                
                if (!ch->valid)
                    continue;

                if (!ch->dimm_label_valid)
                    snprintf (ch->dimm_label, EDAC_LABEL_LEN, "ch%d", i);

                count += ch->ce_count;

                if (ch->ce_count || ctx->verbose)
                    fprintf (stdout, "%s: %s: %s: %u Corrected Errors\n", 
                            mci.id, csi.id, ch->dimm_label, ch->ce_count);
            }
        }
    }

    if (!count && !ctx->quiet)
        fprintf (stdout, "edac-util: No errors to report.\n");

    edac_handle_reset (ctx->edac);
}

static void simple_report (struct prog_ctx *ctx)
{
    edac_mc *mc;
    struct edac_mc_info info;
    unsigned int ue;
    unsigned int ce;

    ue = ce = 0;

    edac_for_each_mc_info (ctx->edac, mc, info) {

        if (!ctx->quiet || info.ce_count)
            fprintf (stdout, "%s: Correctable errors:   %u\n", 
                    info.id, info.ce_count);
        if (!ctx->quiet || info.ue_count)
            fprintf (stdout, "%s: Uncorrectable errors: %u\n",
                    info.id, info.ue_count);

        ue += info.ue_count;
        ce += info.ce_count;
    }

    if (!ctx->quiet || ce)
        fprintf (stdout, "Total CE: %u\n", ce);
    if (!ctx->quiet || ue)
        fprintf (stdout, "Total UE: %u\n", ue);

    edac_handle_reset (ctx->edac);

    return;
}

static void full_report (struct prog_ctx *ctx)
{
    edac_mc *mc;
    edac_csrow *csrow;
    struct edac_mc_info mci;
    struct edac_csrow_info csi;
    int i;

    edac_for_each_mc_info (ctx->edac, mc, mci) {
        edac_for_each_csrow_info (mc, csrow, csi) {


            for (i = 0; i < EDAC_MAX_CHANNELS; i++) {
                struct edac_channel *ch = &csi.channel[i];
                
                if (!ch->valid)
                    continue;

                if (!ch->dimm_label_valid)
                    snprintf (ch->dimm_label, EDAC_LABEL_LEN, "ch%d", i);

                if (ch->ce_count || ctx->verbose)
                    fprintf (stdout, "%s: %s: %s: %u Corrected Errors\n", 
                            mci.id, csi.id, ch->dimm_label, ch->ce_count);

                if (!ctx->quiet || csi.channel[1].ce_count)
                    fprintf (stdout, "%s:%s:%s:CE:%u\n", 
                            mci.id, csi.id, ch->dimm_label,ch->ce_count);
            }

        }

        if (!ctx->quiet || mci.ue_noinfo_count)
            fprintf (stdout, "%s:noinfo:all:UE:%u\n", 
                    mci.id, mci.ue_noinfo_count);
        if (!ctx->quiet || mci.ce_noinfo_count)
            fprintf (stdout, "%s:noinfo:all:CE:%u\n", 
                    mci.id, mci.ce_noinfo_count);
    }

    return;

}

static void 
ue_report (struct prog_ctx *ctx)
{
    struct edac_totals tot;

    if (edac_error_totals (ctx->edac, &tot) < 0) {
        log_fatal (1, "Unable to get EDAC error totals: %s\n", 
                   edac_strerror (ctx->edac));
    }

    if (!ctx->quiet || tot.ue_total)
        fprintf (stdout, "UE: %u\n", tot.ue_total);

    return;
}

static void
ce_report (struct prog_ctx *ctx)
{
    struct edac_totals tot;

    if (edac_error_totals (ctx->edac, &tot) < 0) {
        log_fatal (1, "Unable to get EDAC error totals: %s\n", 
                   edac_strerror (ctx->edac));
    }

    if (!ctx->quiet || tot.ce_total)
        fprintf (stdout, "CE: %u\n", tot.ce_total);

    return;
}

static void
pci_report (struct prog_ctx *ctx)
{
    struct edac_totals tot;

    if (edac_error_totals (ctx->edac, &tot) < 0) {
        log_fatal (1, "Unable to get EDAC error totals: %s\n", 
                   edac_strerror (ctx->edac));
    }

    if (!ctx->quiet || tot.pci_parity_total)
        fprintf (stdout, "PCI Parity Errors: %u\n", tot.pci_parity_total);

    return;
}

static List list_append_from_string (List l, char *str)
{
    List tmp = list_split (",", str);
    ListIterator i = NULL;
    char *attr = NULL;

    if (l == NULL)
        return ((l = tmp));

    i = list_iterator_create (tmp);
    while ((attr = list_next (i))) {
        list_append (l, strdup(attr));
    }
    list_destroy (tmp);

    return (l);

}


static void 
usage (void)
{
    fprintf (stderr, USAGE, prog_ctx.progname);
    return;
}

static void vlog_msg (const char *prefix, const char *format, va_list ap)
{
    char  buf[4096];
    char *p;
    int   n;
    int   len;

    p = buf;
    len = sizeof (buf);

    /*  Prefix output with program name.
     */
    if (prog_ctx.progname && (prog_ctx.progname[0] != '\0')) {
        n = snprintf (buf, len, "%s: ", prog_ctx.progname);
        if ((n < 0) || (n >= len)) {
            p += len - 1;
            len = 0;
        } 
        else {
            p += n;
            len -= n;
        }
    }

    /*  Add a log level prefix.
     */
    if ((len > 0) && (prefix)) {
        n = snprintf (p, len, "%s: ", prefix);
        if ((n < 0) || (n >= len)) {
            p += len - 1;
            len = 0;
        }
        else {
            p += n;
            len -= n;
        }
    }

    if ((len > 0) && (format)) {
        n = vsnprintf (p, len, format, ap);
        if ((n < 0) || (n >= len)) {
            p += len - 1;
            len = 0;
        }
        else {
            p += n;
            len -= n;
        }
    }

    /*  Add suffix for truncation if necessary.
     */
    if (len <= 0) {
        char *q;
        const char *suffix = "+";
        q = buf + sizeof (buf) - 1 - strlen (suffix);
        p = (p < q) ? p : q;
        strcpy (p, suffix);
        p += strlen (suffix);
    }

    *p = '\0';
 
    fprintf (stderr, "%s", buf);

    return;
}

static void log_err (const char *format, ...)
{
    va_list ap;

    if (prog_ctx.quiet)
        return;

    va_start (ap, format);
    vlog_msg ("Error", format, ap);
    va_end (ap);
    return;
}

static void log_fatal (int rc, const char *format, ...)
{
    va_list ap;

    va_start (ap, format);
    vlog_msg ("Fatal", format, ap);
    va_end (ap);

    prog_ctx_fini (&prog_ctx);
    exit (rc);
}

static void log_msg (const char *format, ...)
{
    va_list ap;

    if (prog_ctx.quiet)
        return;

    va_start (ap, format);
    vlog_msg (NULL, format, ap);
    va_end (ap);
    return;
}

static void log_verbose (const char *format, ...)
{
    va_list ap;

    if (prog_ctx.quiet || !prog_ctx.verbose)
        return;

    va_start (ap, format);
    vlog_msg (NULL, format, ap);
    va_end (ap);
    return;
}

static void log_debug (const char *format, ...)
{
    va_list ap;

    if ((prog_ctx.quiet) || (prog_ctx.verbose < 2))
        return;

    va_start (ap, format);
    vlog_msg (NULL, format, ap);
    va_end (ap);
    return;
}

/*  XXX: Stop warnings about unused functions
 */
typedef void (*log_f) (const char *fmt, ...);
log_f log_debug_ptr = log_debug;
log_f log_verbose_ptr = log_verbose;

/*
 * vi: ts=4 sw=4 expandtab
 */
