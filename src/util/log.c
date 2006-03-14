#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include "log.h"

/*****************************************************************************
 *  Constants
 *****************************************************************************/

#define LOG_NAME_MAXLEN 128


/*****************************************************************************
 *  Data Types
 *****************************************************************************/

struct log_ctx {
    int   intialized;
    int   verbose;
    char  id[LOG_NAME_MAXLEN];
};


/*****************************************************************************
 *  Static Variables
 *****************************************************************************/

static struct log_ctx log_ctx = { 0, 0, 0 };


/*****************************************************************************
 *  Static Prototypes
 *****************************************************************************/

static void log_msg (const char *fmt, va_list ap);


/*****************************************************************************
 *  Extern Functions
 *****************************************************************************/

void 
log_init (int quiet, int verbose)
{
}

void 
log_err (const char *fmt, ...)
{
    va_list ap;

    assert (log_ctx.initialized);

    if (!

    va_start (ap, fmt);

}


