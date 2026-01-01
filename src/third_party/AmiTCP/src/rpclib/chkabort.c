RCS_ID_C = "$Id: chkabort.c,v 4.2 1994/09/29 23:48:50 jraja Exp $";
/*
 *      chkabort.c -- prevent SAS/C default CTRL-C checking
 *
 *      Copyright © 1994 AmiTCP/IP Group,
 *                       Network Solutions Development Inc.
 *                       All rights reserved. 
 */

/****** net.lib/__chkabort ***************************************************

    NAME
        __chkabort - replace SAS/C abort checking function with do-nothing

    SYNOPSIS

        void __regargs __chkabort(void);

    FUNCTION
        This function replaces the __chkabort() function of the SAS/C
        library. This is desirable, since the AmiTCP functions break
        with the CTRL-C by default, and this should not be interfered
        with the default SAS/C handling.

    NOTES
        This function is SAS/C 6.x specific. Users of any other
        compilers should provide similar functionality themselves.

    SEE ALSO
        SAS/C 6.x Library Reference

******************************************************************************
*/

void __regargs
__chkabort(void)
{
}
