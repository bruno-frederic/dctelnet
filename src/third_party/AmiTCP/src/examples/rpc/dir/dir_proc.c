/* @(#)dir_proc.c	2.1 88/08/02 4.0 RPCSRC */
/*
 * dir_proc.c: remote readdir implementation
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <sys/dir.h>
#include "dir.h"

readdir_res *
readdir_1_svc(nametype *dirname, struct svc_req *rqstp)
{
	DIR *dirp;
	struct dirent *d;
	namelist nl;
	namelist *nlp;
	static readdir_res res; /* must be static! */
	
	/*
	 * Open directory
	 */
	dirp = opendir(*dirname);
	if (dirp == NULL) {
		res.errno = errno;
		return (&res);
	}

	/*
	 * Free previous result
	 */
	xdr_free(xdr_readdir_res, &res);

	/*
	 * Collect directory entries
	 */
	nlp = &res.readdir_res_u.list;
	while (d = readdir(dirp)) {
		nl = *nlp = (namenode *) malloc(sizeof(namenode));
		nl->name = malloc(strlen(d->d_name)+1);
		strcpy(nl->name, d->d_name);
		nlp = &nl->next;
	}
	*nlp = NULL;

	/*
	 * Return the result
	 */
	res.errno = 0;
	closedir(dirp);
	return (&res);
}
