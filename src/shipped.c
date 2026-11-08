/* src/shipped.c -- open the newest copy of a device or library DCTelnet
 * ships in its own drawer or the system has.
 *
 * DCTelnet's drawer holds Devs/ibmcon.device and Libs/ (reqtools, XEM, XPR).
 * An older copy in DEVS: or LIBS: was used instead of the one that came
 * with DCTelnet -- and one already in memory stayed in use until a reboot.
 * Both files are compared by their $VER string (ProgDir_ParseVersion); a
 * copy of that name already in memory and older than the chosen file is
 * expunged first when nothing uses it.
 */
#ifdef __VBCC__
    #pragma dontwarn 306
#endif
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#ifdef __VBCC__
    #pragma popwarn
#endif
#include "shipped.h"
#include "progdir.h"
#include "utils.h"

#define MAX_SCAN (512UL * 1024)     /* bytes of a file searched for $VER */

const char *ShippedFrom = "the system";

// The $VER version of the file at path; FALSE when there is no such file
// or no version in it.
static BOOL FileVersion(const char *path, ULONG *version, ULONG *revision)
{
    BPTR fh = Open((STRPTR)path, MODE_OLDFILE);
    BOOL found = FALSE;
    LONG size;
    UBYTE *buf;

    if (!fh)
        return FALSE;
    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_BEGINNING);
    if (size > (LONG)MAX_SCAN) size = (LONG)MAX_SCAN;
    if (size > 0 && (buf = AllocVec((ULONG)size, MEMF_ANY)) != NULL)
    {
        if (Read(fh, buf, size) == size)
            found = ProgDir_ParseVersion(buf, (size_t)size, version, revision);
        FreeVec(buf);
    }
    Close(fh);
    return found;
}

/* The name to open: DCTelnet's copy or the system's, the newer file wins.
 * An older copy in memory is expunged when unused; one still in use is
 * what exec will return, so the bare name is kept. */
static const char *Choose(struct List *resident, BOOL device, const char *drawer,
                          const char *assign, const char *name, char *own, size_t max)
{
    char sys[128];
    ULONG ov = 0, orv = 0, sv = 0, srv = 0, fv, fr;
    BOOL hasOwn, hasSys, useOwn;
    struct Library *node;

    if (!ProgDir_Path(drawer, name, own, max))
        return name;                                    // the user's own path
    hasOwn = FileVersion(own, &ov, &orv);
    strlcpy(sys, assign, sizeof(sys));
    strlcat(sys, name, sizeof(sys));
    hasSys = FileVersion(sys, &sv, &srv);
    useOwn = hasOwn && (!hasSys || !ProgDir_Newer(sv, srv, ov, orv));
    fv = useOwn ? ov : sv;
    fr = useOwn ? orv : srv;

    Forbid();
    node = (struct Library *)FindName(resident, (STRPTR)name);
    if (node && (hasOwn || hasSys) && ProgDir_Newer(fv, fr, node->lib_Version, node->lib_Revision))
    {
        if (node->lib_OpenCnt == 0)
        {
            if (device) RemDevice((struct Device *)node);   // expunge the old copy
            else        RemLibrary(node);
            node = (struct Library *)FindName(resident, (STRPTR)name);
        }
    }
    Permit();

    if (node)                   // still in memory: exec hands out that one
    {
        // The same version as the chosen file is that file, loaded earlier
        // (a reopen); an older one in use elsewhere is "memory".
        if (hasOwn || hasSys)
            ShippedFrom = ProgDir_Newer(fv, fr, node->lib_Version, node->lib_Revision)
                        ? "memory" : (useOwn ? "DCTelnet's drawer" : "the system");
        else
            ShippedFrom = "memory";
        return name;
    }
    ShippedFrom = useOwn ? "DCTelnet's drawer" : "the system";
    return useOwn ? own : name;
}

struct Library *OpenNewestLibrary(CONST_STRPTR name, ULONG version)
{
    char own[128];
    const char *pick = Choose(&SysBase->LibList, FALSE, "Libs", "LIBS:", name, own, sizeof(own));
    struct Library *lib = OpenLibrary((STRPTR)pick, version);

    if (!lib && pick != (const char *)name)             // not loadable from there
        lib = OpenLibrary(name, version);
    return lib;
}

BYTE OpenNewestDevice(STRPTR name, ULONG unit, struct IORequest *io, ULONG flags)
{
    char own[128];
    const char *pick = Choose(&SysBase->DeviceList, TRUE, "Devs", "DEVS:", name, own, sizeof(own));
    BYTE err = OpenDevice((STRPTR)pick, unit, io, flags);

    if (err && pick != (const char *)name)
        err = OpenDevice(name, unit, io, flags);
    return err;
}
