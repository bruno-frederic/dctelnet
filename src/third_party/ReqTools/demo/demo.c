/*********************************
*                                *
*  reqtools.library (V38)        *
*                                *
*  Release 2.5                   *
*                                *
*  (c) 1991-1994 Nico François   *
*      1995-1996 Nico François   *
*                                *
*  demo.c                        *
*                                *
*  This source is public domain  *
*  in all respects.              *
*                                *
*********************************/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <libraries/dos.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libraries/reqtools.h>
#include <proto/reqtools.h>

extern struct ExecBase *SysBase;
struct ReqToolsBase *ReqToolsBase;

#define REG register

/* Callback functions
	Define DOHOOKS when you compile to activate these filter functions */

BOOL __asm __saveds file_filterfunc (
	REG __a0 struct Hook *, REG __a2 struct rtFileRequester *,
	REG __a1 struct FileInfoBlock *
	);
BOOL __asm __saveds font_filterfunc (
	REG __a0 struct Hook *, REG __a2 struct rtFontRequester *,
	REG __a1 struct TextAttr *
	);
BOOL __asm __saveds vol_filterfunc (
	REG __a0 struct Hook *, REG __a2 struct rtFileRequester *,
	REG __a1 struct rtVolumeEntry *
	);

ULONG undertag[] = { RT_Underscore, '_', TAG_END };

void myputs (char *str)
{
	Write (Output(), str, strlen(str));
}

void __stdargs __main (char *line)
{
	struct rtFileRequester *filereq;
	struct rtFontRequester *fontreq;
	struct rtScreenModeRequester *scrmodereq;
	struct rtFileList *flist, *tempflist;
	struct Hook filterhook, font_filterhook, vol_filterhook;
	char buffer[128], filename[34];
	ULONG longnum, ret, color;

	if (!(ReqToolsBase = (struct ReqToolsBase *)
  										OpenLibrary (REQTOOLSNAME, REQTOOLSVERSION))) {
		myputs ("You need reqtools.library V38 or higher!\n"
				  "Please install it in your Libs: drirectory.\n");
		exit (0);
		}

	rtEZRequest ("ReqTools 2.0 Demo\n"
	             "~~~~~~~~~~~~~~~~~\n"
					 "'reqtools.library' offers several\n"
					 "different types of requesters:",
					 "Let's see them", NULL, NULL);

	rtEZRequest ("NUMBER 1:\nThe larch :-)", "Be serious!", NULL, NULL);

	rtEZRequest ("NUMBER 1:\nString requester\nfunction: rtGetString()",
					 "Show me", NULL, NULL);

	strcpy (buffer, "A bit of text");
	if (!rtGetString (buffer, 127, "Enter anything:", NULL, TAG_END))
		rtEZRequest ("You entered nothing :-(", "I'm sorry", NULL, NULL);
	else
		rtEZRequest ("You entered this string:\n'%s'.",
						 "So I did", NULL, NULL, buffer);

	ret = rtGetString (buffer, 127, "Enter anything:", NULL,
							 RTGS_GadFmt, " _Ok |New _2.0 feature!|_Cancel",
							 RTGS_TextFmt,
								 "These are two new features of ReqTools 2.0:\n"
			 					 "Text above the entry gadget and more than\n"
					 			 "one response gadget.",
					 TAG_MORE, undertag);
	if (ret == 2)
		rtEZRequest ("Yep, this is a new\nReqTools 2.0 feature!",
						 "Oh boy!", NULL, NULL);

	ret = rtGetString (buffer, 127, "Enter anything:", NULL,
							 RTGS_GadFmt, " _Ok |_Abort|_Cancel",
							 RTGS_TextFmt,
								 "New is also the ability to switch off the\n"
					 			 "backfill pattern.  You can also center the\n"
					 			 "text above the entry gadget.\n"
					 			 "These new features are also available in\n"
					 			 "the rtGetLong() requester.",
							 RTGS_BackFill, FALSE,
							 RTGS_Flags, GSREQF_CENTERTEXT|GSREQF_HIGHLIGHTTEXT,
							 TAG_MORE, undertag);
	if (ret == 2)
		rtEZRequest ("What!! You pressed abort!?!\nYou must be joking :-)",
						 "Ok, Continue", NULL, NULL);


	rtEZRequest ("NUMBER 2:\nNumber requester\nfunction: rtGetLong()",
					 "Show me", NULL, NULL);

	if (!rtGetLong (&longnum, "Enter a number:", NULL,
						 RTGL_ShowDefault, FALSE,
						 RTGL_Min, 0, RTGL_Max, 666,TAG_END))
		rtEZRequest ("You entered nothing :-(",
						 "I'm sorry", NULL, NULL);
	else
		rtEZRequest ("The number you entered was:\n%ld%s",
						 "So it was", NULL, NULL, longnum,
							(longnum == 666) ? " (you devil! :)" : "");

	rtEZRequest ("NUMBER 3:\nMessage requester, the requester\n"
					 "you've been using all the time!\nfunction: rtEZRequest()",
					 "Show me more", NULL, NULL);

	rtEZRequest ("Simplest usage: some body text and\na single centered gadget.",
					 "Got it", NULL, NULL);

	while (!rtEZRequest ("You can also use two gadgets to\n"
								"ask the user something.\n"
								"Do you understand?", "Of course|Not really",
								NULL, NULL))
		rtEZRequest ("You are not one of the brightest are you?\n"
						 "We'll try again...",
						 "Ok", NULL, NULL);

	rtEZRequest ("Great, we'll continue then.", "Fine", NULL, NULL);

	switch (rtEZRequest ("You can also put up a requester with\n"
								"three choices.\n"
								"How do you like the demo so far ?",
								"Great|So so|Rubbish", NULL, NULL)) {
		case FALSE:
			rtEZRequest ("Too bad, I really hoped you\nwould like it better.",
							 "So what", NULL, NULL);
			break;
		case TRUE:
			rtEZRequest ("I'm glad you like it so much.", "Fine", NULL, NULL);
			break;
		case 2:
			rtEZRequest ("Maybe if you run the demo again\n"
							 "you'll REALLY like it.",
							 "Perhaps", NULL, NULL);
			break;
		}

	ret = rtEZRequestTags ("The number of responses is not limited to three\n"
								  "as you can see.  The gadgets are labeled with\n"
								  "the return code from rtEZRequest().\n"
								  "Pressing Return will choose 4, note that\n"
								  "4's button text is printed in boldface.",
								  "1|2|3|4|5|0", NULL, NULL,
								  RTEZ_DefaultResponse, 4, TAG_END);
	rtEZRequest ("You picked '%ld'.", "How true", NULL, NULL, ret);

	rtEZRequestTags ("New for Release 2.0 of ReqTools (V38) is\n"
						  "the possibility to define characters in the\n"
						  "buttons as keyboard shortcuts.\n"
						  "As you can see these characters are underlined.\n"
						  "Pressing shift while still holding down the key\n"
						  "will cancel the shortcut.\n"
						  "Note that in other requesters a string gadget may\n"
						  "be active.  To use the keyboard shortcuts there\n"
						  "you have to keep the Right Amiga key pressed down.",
						  "_Great|_Fantastic|_Swell|Oh _Boy",
						  NULL, NULL,
						  RT_Underscore, '_', TAG_END);

	rtEZRequest (
		"You may also use C-style formatting codes in the body text.\n"
		"Like this:\n\n"
		"'The number %%ld is written %%s.' will give:\n\n"
		"The number %ld is written %s.\n\n"
		"if you also pass '5' and '\"five\"' to rtEZRequest().",
		"_Proceed", NULL, (struct TagItem *)&undertag, 5, "five");

	if (rtEZRequestTags ("It is also possible to pass extra IDCMP flags\n"
							   "that will satisfy rtEZRequest(). This requester\n"
							   "has had DISKINSERTED passed to it.\n"
							   "(Try inserting a disk).",
							   "_Continue", NULL, NULL,
							   RT_IDCMPFlags, DISKINSERTED,
								RT_Underscore, '_', TAG_END)
							   & DISKINSERTED)
		rtEZRequest ("You inserted a disk.", "I did", NULL, NULL);
	else
		rtEZRequest ("You used the 'Continue' gadget\n"
						 "to satisfy the requester.", "I did", NULL, NULL);

	rtEZRequestTags ("Finally, it is possible to specify the position\n"
						  "of the requester.\n"
						  "E.g. at the top left of the screen, like this.\n"
						  "This works for all requesters, not just rtEZRequest()!",
						  "_Amazing", NULL, NULL,
						  RT_ReqPos, REQPOS_TOPLEFTSCR,
						  RT_Underscore, '_', TAG_END);

	rtEZRequestTags ("Alternatively, you can center the\n"
						  "requester on the screen.\n"
						  "Check out 'reqtools.doc' for all the possibilities.",
						  "I'll do that", NULL, NULL,
						  RT_ReqPos, REQPOS_CENTERSCR, TAG_END);

	rtEZRequestTags ("NUMBER 4:\nFile requester\n"
						  "function: rtFileRequest()",
						  "_Demonstrate", NULL, NULL, RT_Underscore, '_', TAG_END);

	if (filereq = rtAllocRequestA (RT_FILEREQ, NULL)) {

		filterhook.h_Entry = (ULONG (*)())file_filterfunc;

		filename[0] = 0;
		if (rtFileRequest (filereq, filename, "Pick a file",
#ifdef DOHOOKS
								 RTFI_FilterFunc, &filterhook,
#endif
								 TAG_END))
			rtEZRequest ("You picked the file:\n'%s'\nin directory:\n'%s'",
							 "Right", NULL, NULL, filename, filereq->Dir);
		else
			rtEZRequest ("You didn't pick a file.", "No", NULL, NULL);

		rtEZRequestTags ("The file requester has the ability\n"
							  "to allow you to pick more than one\n"
							  "file (use Shift to extend-select).\n"
							  "Note the extra gadgets you get.",
							  "_Interesting", NULL, NULL, RT_Underscore, '_', TAG_END);

		flist = rtFileRequest (filereq, filename, "Pick some files",
									  RTFI_Flags, FREQF_MULTISELECT, TAG_END);
		if (flist) {
			rtEZRequest ("You selected some files, this is\n"
							 "the first one:\n"
							 "'%s'\n"
							 "All the files are returned as a linked\n"
							 "list (see demo.c and reqtools.h).",
							 "Aha", NULL, NULL, flist->Name);
			/* Traverse all selected files */
			/*
			tempflist = flist;
			while (tempflist) {
				DoSomething (tempflist->Name, tempflist->StrLen);
				tempflist = tempflist->Next;
				}
			*/
			/* Free filelist when no longer needed! */
			rtFreeFileList (flist);
			}

		rtFreeRequest (filereq);
		}
	else
		rtEZRequest ("Out of memory!", "Oh boy!", NULL, NULL);


	rtEZRequestTags ("The file requester can be used\n"
						  "as a directory requester as well.",
						  "Let's _see that", NULL, NULL, RT_Underscore, '_', TAG_END);

	if (filereq = rtAllocRequestA (RT_FILEREQ, NULL)) {
		if (rtFileRequest (filereq, filename, "Pick a directory",
								 RTFI_Flags, FREQF_NOFILES, TAG_END))
			rtEZRequest ("You picked the directory:\n'%s'",
							 "Right", NULL, NULL, filereq->Dir);
		else
			rtEZRequest ("You didn't pick a directory.", "No", NULL, NULL);

		rtFreeRequest (filereq);
		}
	else
		rtEZRequest ("Out of memory!", "Oh boy!", NULL, NULL);

	rtEZRequest ("NUMBER 5:\nFont requester\n"
					 "function: rtFontRequest()",
					 "Show", NULL, NULL);

	if (fontreq = rtAllocRequestA (RT_FONTREQ, NULL)) {

		fontreq->Flags = FREQF_STYLE|FREQF_COLORFONTS;
		font_filterhook.h_Entry = (ULONG (*)())font_filterfunc;

		if (rtFontRequest (fontreq, "Pick a font",
#ifdef DOHOOKS
								 RTFO_FilterFunc, &font_filterhook,
#endif
								 TAG_END))
			rtEZRequest ("You picked the font:\n'%s'\nwith size:\n'%ld'",
							 "Right", NULL, NULL,
							 fontreq->Attr.ta_Name, fontreq->Attr.ta_YSize);
		else
			rtEZRequestTags ("You canceled.\nWas there no font you liked ?",
								  "_Nope", NULL, NULL, RT_Underscore, '_', TAG_END);

		rtFreeRequest (fontreq);
		}
	else
		rtEZRequest ("Out of memory!", "Oh boy!", NULL, NULL);

	rtEZRequestTags ("NUMBER 6:\nPalette requester\n"
						  "function: rtPaletteRequest()",
						  "_Proceed", NULL, NULL, RT_Underscore, '_', TAG_END);

	color = rtPaletteRequest ("Change palette", NULL, TAG_END);
	if (color == -1)
		rtEZRequest ("You canceled.\nNo nice colors to be picked ?",
						 "Nah", NULL, NULL);
	else
		rtEZRequest ("You picked color number %ld.", "Sure did",
						 NULL, NULL, color);

	rtEZRequestTags ("NUMBER 7: (ReqTools 2.0)\n"
						  "Volume requester\n"
						  "function: rtFileRequest() with\n"
						  "          RTFI_VolumeRequest tag.",
						  "_Show me", NULL, NULL, RT_Underscore, '_', TAG_END);

	if (filereq = rtAllocRequestA (RT_FILEREQ, NULL)) {

		vol_filterhook.h_Entry = (ULONG (*)())vol_filterfunc;

		if (rtFileRequest (filereq, NULL, "Pick a volume",
#ifdef DOHOOKS
								 RTFI_FilterFunc, &vol_filterhook,
#endif
								 RTFI_VolumeRequest, 0, TAG_END))
			rtEZRequest ("You picked the volume:\n'%s'",
							 "Right", NULL, NULL, filereq->Dir);
		else
			rtEZRequest ("You didn't pick a volume.", "I did not", NULL, NULL);

		rtFreeRequest (filereq);
		}
	else
		rtEZRequest ("Out of memory!", "Oh boy!", NULL, NULL);

	rtEZRequestTags ("NUMBER 8: (ReqTools 2.0)\n"
						  "Screen mode requester\n"
						  "function: rtScreenModeRequest()\n"
						  "Only available on Kickstart 2.0!",
						  "_Proceed", NULL, NULL, RT_Underscore, '_', TAG_END);

	if (SysBase->LibNode.lib_Version < 37)
		rtEZRequestTags ("Your Amiga doesn't seem to have\n"
							  "Kickstart 2.0 in ROM so I am not\n"
							  "able to show you the Screen mode\n"
							  "requester.\n"
							  "So upgrade to 2.0 *now* :-)",
							  "_Allright", NULL, NULL, RT_Underscore, '_', TAG_END);
	else {
		if (scrmodereq = rtAllocRequestA (RT_SCREENMODEREQ, NULL)) {

			if (rtScreenModeRequest (scrmodereq, "Pick a screen mode:",
											 RTSC_Flags, SCREQF_DEPTHGAD|SCREQF_SIZEGADS|
												 SCREQF_AUTOSCROLLGAD|SCREQF_OVERSCANGAD,
									 		 TAG_END))
				rtEZRequest ("You picked this mode:\n"
								 "ModeID  : 0x%lx\n"
								 "Size    : %ld x %ld\n"
								 "Depth   : %ld\n"
								 "Overscan: %ld\n"
								 "AutoScroll %s",
								 "Right", NULL, NULL,
								 scrmodereq->DisplayID,
								 scrmodereq->DisplayWidth,
								 scrmodereq->DisplayHeight,
								 scrmodereq->DisplayDepth,
								 scrmodereq->OverscanType,
								 scrmodereq->AutoScroll ? "On" : "Off");
			else
				rtEZRequest ("You didn't pick a screen mode.", "Nope", NULL, NULL);

			rtFreeRequest (scrmodereq);
			}
		else
			rtEZRequest ("Out of memory!", "Oh boy!", NULL, NULL);
		}

	rtEZRequestTags ("That's it!\n"
						  "Hope you enjoyed the demo", "_Sure did", NULL, NULL,
						  RT_Underscore, '_', TAG_END);

	/* free all resources */
	CloseLibrary ((struct Library *)ReqToolsBase);

	exit (0);
}

/********
* HOOKS *
********/

BOOL __asm __saveds file_filterfunc (
	REG __a0 struct Hook *hook,
	REG __a2 struct rtFileRequester *filereq,
	REG __a1 struct FileInfoBlock *fib
	)
{
	myputs (fib->fib_FileName);
	myputs ("\n");
	return (TRUE);
}

BOOL __asm __saveds font_filterfunc (
	REG __a0 struct Hook *hook,
	REG __a2 struct rtFontRequester *fontreq,
	REG __a1 struct TextAttr *textattr
	)
{
	myputs (textattr->ta_Name);
	myputs ("\n");
	return (TRUE);
}

BOOL __asm __saveds vol_filterfunc (
	REG __a0 struct Hook *hook,
	REG __a2 struct rtFileRequester *filereq,
	REG __a1 struct rtVolumeEntry *volentry
	)
{
	myputs (volentry->Type == DLT_DEVICE ? "(Volume) " : "(Assign) ");
	myputs (volentry->Name);
	myputs ("\n");
	return (TRUE);
}
