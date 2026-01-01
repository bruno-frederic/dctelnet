/* @(#)sort_proc.c	2.1 88/08/11 4.0 RPCSRC */
#include <stdlib.h>
#include <string.h>
#include <rpc/rpc.h>
#include "sort.h"

static int
comparestrings(char **sp1, char **sp2)
{
    return (strcmp(*sp1, *sp2));
}

struct sortstrings *
sort_1_svc(struct sortstrings *ssp, struct svc_req *rqstp)
{
    static struct sortstrings ss_res = { 0 }; 

    if (ss_res.ss.ss_val != (str *)NULL)
        free(ss_res.ss.ss_val);

    qsort(ssp->ss.ss_val, ssp->ss.ss_len, sizeof (char *), comparestrings);
    ss_res.ss.ss_len = ssp->ss.ss_len;
    ss_res.ss.ss_val = (str *)malloc(ssp->ss.ss_len * sizeof(str *));
    memcpy(ss_res.ss.ss_val, ssp->ss.ss_val, 
	   ssp->ss.ss_len * sizeof(str *));
    return(&ss_res);
}
