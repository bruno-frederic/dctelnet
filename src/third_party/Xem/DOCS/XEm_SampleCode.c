

       /* external emulator support functions included in LR-Comm */


VOID __saveds xem_sstart(VOID)
{
	StartSerial();
}

/************************************************************************/

LONG __saveds xem_sstop(VOID)
{
	KillSerial();
	return(0);
}

/************************************************************************/

BOOL __saveds __asm
xem_tgets(register __a0 UBYTE *prompt,register __a1 UBYTE *buffer, register __d0 ULONG size)
{
	return(myGetString(prompt, buffer, size));
}

/************************************************************************/

VOID __saveds __asm
xem_tbeep(register __d0 ULONG ntimes, register __d1 ULONG delay)
{
	while(ntimes)
	{
		if(user_said__FLASH)
			DisplayBeep(NULL);

		if(user_said__BEEP)
			ReallyBeep();

		Delay(delay);
		ntimes--;
	}
}

/************************************************************************/

ULONG __saveds __asm
xpr_sread(register __a0 UBYTE *buff, register __d0 ULONG size, register __d1 LONG timeout)
{
/*	this function was `coded' for xpr usage, but it works well with xem, too */

	if(size == 0  ||  buff == NULL)
		return(0);	/* WYWIWYG..(-: */


	WriteSER->IOSer.io_Command = SDCMD_QUERY;
	DoIO((struct IORequest *)WriteSER);


	if(timeout < 1)
	{
		if(WriteSER->IOSer.io_Actual > 0)
		{
			if(size < WriteSER->IOSer.io_Actual)
				ReadSER->IOSer.io_Length = size;
			else
				ReadSER->IOSer.io_Length = WriteSER->IOSer.io_Actual;

			ReadSER->IOSer.io_Command	= CMD_READ;
			ReadSER->IOSer.io_Data		= buff;

			if(!DoIO((struct IORequest *)ReadSER))
				return(ReadSER->IOSer.io_Actual);
		}
	}
	else
	{
		if(WriteSER->IOSer.io_Actual < size)
		{
			register ULONG signalset;
			ULONG sersig, timesig, winsig;

			DoTIME->tr_node.io_Command	= TR_ADDREQUEST;
			DoTIME->tr_time.tv_secs	= timeout >= MILLION ? timeout / MILLION : 0;
			DoTIME->tr_time.tv_micro	= timeout % MILLION;

			ReadSER->IOSer.io_Command	= CMD_READ;
			ReadSER->IOSer.io_Data		= buff;
			ReadSER->IOSer.io_Length	= size;

			sersig	= 1 << SerReadPort->mp_SigBit;
			timesig	= 1 << DoTIME->tr_node.io_Message.mn_ReplyPort->mp_SigBit;
			winsig	= (XPrWindow) ? (1 << XPrWindow->UserPort->mp_SigBit) : 0;
			SetSignal(0, sersig | timesig);

			SendIO((struct IORequest *)ReadSER);
			SendIO((struct IORequest *)DoTIME);

			for(;;)
			{
				signalset = Wait(sersig | timesig | winsig);


				if(signalset & timesig)
				{
					AbortIO((struct IORequest *)ReadSER);
					WaitIO((struct IORequest *)ReadSER);

					WaitIO((struct IORequest *)DoTIME);

					if(!ReadSER->IOSer.io_Actual)
					{
						WriteSER->IOSer.io_Command = SDCMD_QUERY;
						DoIO((struct IORequest *)WriteSER);

						if(WriteSER->IOSer.io_Actual > 0)
						{
							if(size < WriteSER->IOSer.io_Actual)
								ReadSER->IOSer.io_Length = size;
							else
								ReadSER->IOSer.io_Length = WriteSER->IOSer.io_Actual;

							ReadSER->IOSer.io_Command	= CMD_READ;
							ReadSER->IOSer.io_Data		= buff;

							if(!DoIO((struct IORequest *)ReadSER))
								return(ReadSER->IOSer.io_Actual);
							else
								return(0);
						}
						else
							return(0);
					}
					else
						return(ReadSER->IOSer.io_Actual);
				}

				if(signalset & sersig)
				{
					AbortIO((struct IORequest *)DoTIME);
					WaitIO((struct IORequest *)DoTIME);

					WaitIO((struct IORequest *)ReadSER);

					if(!ReadSER->IOSer.io_Error)
						return(ReadSER->IOSer.io_Actual);
					else
						return(0);
				}

				if(signalset & winsig)
				{
					if(xpr_chkabort() == -1)
					{
						AbortIO((struct IORequest *)ReadSER);
						AbortIO((struct IORequest *)DoTIME);

						WaitIO((struct IORequest *)ReadSER);
						WaitIO((struct IORequest *)DoTIME);

						return(-1);
					}
				}
			}
		}
		else
		{
			ReadSER->IOSer.io_Command	= CMD_READ;
			ReadSER->IOSer.io_Data		= buff;
			ReadSER->IOSer.io_Length	= size;

			if(!DoIO((struct IORequest *)ReadSER))
				return(size);
		}
	}
	return(0);
}

/************************************************************************/

LONG __saveds __asm
xpr_swrite(register __a0 UBYTE *buffy, register __d0 ULONG size)
{
/*	this function was `coded' for xpr usage, but it works well with xem, too */

	WriteSER->IOSer.io_Data		= buffy;
	WriteSER->IOSer.io_Length	= size;
	WriteSER->IOSer.io_Command	= CMD_WRITE;

	return((LONG)DoIO((struct IORequest *)WriteSER));
}

/************************************************************************/

LONG __saveds
xpr_squery(VOID)
{
/*	this function was `coded' for xpr usage, but it works well with xem, too */

	LONG chars=-1;

	ReadSER->IOSer.io_Command = SDCMD_QUERY;
	if(!DoIO((struct IORequest *)ReadSER))
		chars = ReadSER->IOSer.io_Actual;
	ReadSER->IOSer.io_Command = CMD_READ;

	return(chars);
}

/************************************************************************/

LONG __saveds
xpr_sflush(VOID)
{
	LONG err;

/*	this function was `coded' for xpr usage, but it works well with xem, too */

/* flush receive buffer  */
	ReadSER->IOSer.io_Command = CMD_CLEAR;
	err = DoIO((struct IORequest *)ReadSER);
	ReadSER->IOSer.io_Command = CMD_READ;

	return(err);
}

/************************************************************************/

STATIC struct Gadget *CreateOptionGadgets(LONG *count, UWORD *width, UWORD *height, LONG numopts, struct xpr_option *opts[], struct Gadget *gadgetarray[], struct Gadget **gadgetlist, UWORD topedge)
{
	STATIC BOOL GetOptionMode(struct xpr_option *);

	IMPORT APTR VisualInfo;
	IMPORT struct TextAttr DefaultAttr;

	struct Gadget *gadget;
	struct NewGadget newgad;
	LONG i;
	UWORD leftedge;
	UWORD command_len, leftstring_len, rightstring_len, header_len;
	UWORD leftgadget_width, rightgadget_width;
	BOOL split, right;

	if(numopts == 0)		/* AETSCH..!! */
		return(NULL);

	*count = 0;

	memset(&newgad, 0, sizeof(struct NewGadget));
	if(gadget = CreateContext(gadgetlist))
	{
		newgad.ng_Height		= 12;
		newgad.ng_TextAttr	= &DefaultAttr;
		newgad.ng_VisualInfo	= VisualInfo;
		newgad.ng_Flags		= NG_HIGHLABEL;

	/* is there a 2 column display? */
		split = (numopts > 11);

		header_len = 0;
		command_len = 0;
		leftstring_len = 0;
		rightstring_len = 0;

		leftgadget_width = 20;	/* BOOL-Gadgets first! */
		rightgadget_width = 20;
		
		right = FALSE;	/* we start at the left side.. */
		for(i=0; i<numopts; i++)
		{
			UWORD len = (strlen(opts[i]->xpro_description) + 1) << 3;

			switch(opts[i]->xpro_type)
			{
				case XPRO_HEADER:
					if(header_len < len)
						header_len = len;

					right = TRUE;		/* we need the right column, too */
				break;

				case XPRO_COMMAND:
					if(command_len < len)
						command_len = len + 50;

					right = TRUE;		/* we need the right column, too */
				break;

				case XPRO_STRING:
				case XPRO_LONG:
				case XPRO_COMMPAR:
					if(split  &&  right)
						rightgadget_width = 68;
					else
						leftgadget_width = 68;

				/* break thru */

				case XPRO_BOOLEAN:
					if(split  &&  right)
					{
						if(rightstring_len < len)
							rightstring_len = len;
					}
					else
					{
						if(leftstring_len < len)
							leftstring_len = len;
					}
				break;

			}

			right = !right;	/* swap it.. */
		}

#define LEFTEDGE 15
#define ROW 14


		right = FALSE;	/* we start at the left side of life  ;-) */
		for(i=0; i<numopts; i++)
		{
			leftedge = LEFTEDGE + leftstring_len;
			newgad.ng_Width = leftgadget_width;

			if(split  &&  right)
			{
				leftedge += leftgadget_width;
				leftedge += (LEFTEDGE + rightstring_len);
				newgad.ng_Width = rightgadget_width;
			}
			else
				topedge += ROW;


			newgad.ng_GadgetText	= opts[i]->xpro_description;
			newgad.ng_GadgetID	= i;
			newgad.ng_LeftEdge	= leftedge;
			newgad.ng_TopEdge		= topedge;

			switch(opts[i]->xpro_type)
			{
				case XPRO_BOOLEAN:
					gadgetarray[i] = gadget = CreateGadget(CHECKBOX_KIND,gadget,&newgad,
						GTCB_Checked,	GetOptionMode(opts[i]),
					TAG_DONE);
				break;

				case XPRO_LONG:
					gadgetarray[i] = gadget = CreateGadget(INTEGER_KIND,gadget,&newgad,
						GTIN_Number,	AtoL(opts[i]->xpro_value),
					TAG_DONE);
				break;

				case XPRO_STRING:
					gadgetarray[i] = gadget = CreateGadget(STRING_KIND,gadget,&newgad,
						GTST_String,	opts[i]->xpro_value,
						GTST_MaxChars,	opts[i]->xpro_length,
					TAG_DONE);
				break;

				case XPRO_COMMPAR:
					newgad.ng_Width = LEFTEDGE + command_len - leftedge;

					gadgetarray[i] = gadget = CreateGadget(STRING_KIND,gadget,&newgad,
						GTST_String,	opts[i]->xpro_value,
						GTST_MaxChars,	opts[i]->xpro_length,
					TAG_DONE);
				break;

				case XPRO_HEADER:
					if(split  &&  right)
						topedge += ROW;

					newgad.ng_GadgetText	= NULL;
					newgad.ng_Width		= header_len;
					newgad.ng_LeftEdge	= LEFTEDGE;
					newgad.ng_TopEdge		= topedge;

					right = TRUE;	/* we need the right column, too */

					gadgetarray[i] = gadget = CreateGadget(TEXT_KIND,gadget,&newgad,
						GTTX_Text,	opts[i]->xpro_description,
					TAG_DONE);
				break;

				case XPRO_COMMAND:
					leftedge = LEFTEDGE;

					if(split  &&  right)
					{
						leftedge += command_len;
						leftedge <<= 1;
					}
					newgad.ng_Width	 = command_len;
					newgad.ng_LeftEdge = leftedge;

					gadgetarray[i] = gadget = CreateGadget(BUTTON_KIND,gadget,&newgad,
					TAG_DONE);
				break;

				default:
					;
				break;
			}

			right = !right;	/* swap it.. */
		}


		*width = leftstring_len + leftgadget_width;
		if(split)
			*width += (LEFTEDGE + rightstring_len + rightgadget_width);

		if(split)
		{
			UWORD cw;

			cw = (command_len << 1) + LEFTEDGE;
			if(*width < cw)
				*width = cw;
		}
		else
		{
			if(*width < command_len)
				*width = command_len;
		}

		if(*width < header_len)
			*width = header_len;

		*width += (LEFTEDGE << 1);	/* left + right distance.. */

		newgad.ng_GadgetText	= "Continue";
		newgad.ng_Width		= 88;
		newgad.ng_LeftEdge	= (*width >> 1) - 44;
		newgad.ng_TopEdge		= gadget->TopEdge + gadget->Height + 9;
		newgad.ng_GadgetID	= numopts;
		newgad.ng_Flags		= NG_HIGHLABEL | PLACETEXT_IN;

		gadget = CreateGadget(BUTTON_KIND,gadget,&newgad,
			TAG_DONE);

		*height= gadget->TopEdge + gadget->Height + 5;
		*count = i;
	}

	return(gadget);
}

/************************************************************************/

ULONG __saveds __asm
xpr_options(register __d0 LONG numopts,register __a0 struct xpr_option **opts)
{
	IMPORT struct Screen *CommScreen;

/*	this function was `coded' for xpr usage, but it works well with xem, too */

	struct Gadget	*gadgetlist;
	struct Gadget	*gadgetarray[33];
	struct Window	*window;
	ULONG flags = 0;
	UWORD left, top, width, height;
	LONG	i, count;

	if(CreateOptionGadgets(&count,&width,&height,numopts,opts,&gadgetarray[0],&gadgetlist,1))
	{
		GetWindowPosition(&left, &top, width, height);

		if(window = OpenWindowTags(NULL,
			WA_Width,			width,
			WA_Height,			height,
			WA_Left,				left,
			WA_Top,				top,
			WA_Activate,		TRUE,
			WA_DragBar,			TRUE,
			WA_DepthGadget,	TRUE,
			WA_RMBTrap,			TRUE,
			WA_CustomScreen,	CommScreen,
			WA_IDCMP,			IDCMP_GADGETUP | CHECKBOXIDCMP | IDCMP_RAWKEY,
			WA_Title,			"XEm-Options ][",
		TAG_DONE))
		{
			struct IntuiMessage	*imsg;
			struct Gadget		*gadget;
			ULONG class, code;
			BOOL quit;

			AddGList(window,gadgetlist,(UWORD)-1,(UWORD)-1,NULL);
			RefreshGList(gadgetlist,window,NULL,(UWORD)-1);
			GT_RefreshWindow(window,NULL);

			for(quit=FALSE; quit==FALSE; )
			{
				WaitPort(window->UserPort);

				while(imsg = (struct IntuiMessage *)GT_GetIMsg(window->UserPort))
				{
					class	= imsg->Class;
					code	= imsg->Code;
					gadget	= (struct Gadget *)imsg->IAddress;

					GT_ReplyIMsg(imsg);


					if(class == IDCMP_RAWKEY)
					{
						if(CheckAbort(code))
							class = IDCMP_CLOSEWINDOW;
					}

					if(class == IDCMP_GADGETUP)
					{
						UWORD id;

						if((id = gadget->GadgetID) >= numopts)
							class = IDCMP_CLOSEWINDOW;

						if(opts[id]->xpro_type == XPRO_COMMAND)
						{
							flags |= (1 << id);
							quit = TRUE;
						}
						else
						{
							if(opts[id]->xpro_type == XPRO_COMMPAR)
							{
								if(strcmp(opts[id]->xpro_value, ((struct StringInfo *)gadget->SpecialInfo)->Buffer))
								{
									flags |= (1 << id);
									strcpy(opts[id]->xpro_value,((struct StringInfo *)gadget->SpecialInfo)->Buffer);
								}
								quit = TRUE;
							}
						}
					}


					if(class == IDCMP_CLOSEWINDOW)
					{
						for(i=0 ; i<numopts ; i++)
						{
							switch(opts[i]->xpro_type)
							{
								case XPRO_BOOLEAN:
									if(((gadgetarray[i]->Flags & SELECTED)  &&  !GetOptionMode(opts[i])) || (!(gadgetarray[i]->Flags & SELECTED)  &&  GetOptionMode(opts[i])))
									{
										flags |= (1 << i);

										if(gadgetarray[i]->Flags & SELECTED)
											strcpy(opts[i]->xpro_value, "yes");
										else
											strcpy(opts[i]->xpro_value, "no");
									}
								break;

								case XPRO_LONG:
								case XPRO_STRING:
									if(strcmp(opts[i]->xpro_value, ((struct StringInfo *)gadgetarray[i]->SpecialInfo)->Buffer))
									{
										flags |= (1 << i);
										strcpy(opts[i]->xpro_value,((struct StringInfo *)gadgetarray[i]->SpecialInfo)->Buffer);
									}
								break;
							}
						}
						quit = TRUE;
					}
				}
			}

			RemoveGList(window, gadgetlist, (UWORD)-1);
			CloseWindow(window);
		}
		FreeGadgets(gadgetlist);
	}
	
	return(flags);
}

/************************************************************************/

STATIC BOOL GetOptionMode(struct xpr_option *Option)
{
	if(!stricmp(Option->xpro_value, "OFF"))
		return(0);

	if(!stricmp(Option->xpro_value, "FALSE"))
		return(0);

	if(!stricmp(Option->xpro_value, "F"))
		return(0);

	if(!stricmp(Option->xpro_value, "NO"))
		return(0);

	if(!stricmp(Option->xpro_value, "N"))
		return(0);


	if(!stricmp(Option->xpro_value, "ON"))
		return(1);

	if(!stricmp(Option->xpro_value, "TRUE"))
		return(1);

	if(!stricmp(Option->xpro_value, "T"))
		return(1);

	if(!stricmp(Option->xpro_value, "YES"))
		return(1);

	if(!stricmp(Option->xpro_value, "Y"))
		return(1);

	return(0);
}

/************************************************************************/

VOID InitXEmReq(struct XEM_IO *xem_io)
{
	IMPORT struct Window		*CommWindow;
	IMPORT struct TextFont	*DefaultFont;
	IMPORT ULONG	ExternalSignals;
	IMPORT UWORD	CommDepth;

	xem_io->xem_window	= CommWindow;
	xem_io->xem_font 		= DefaultFont;
	xem_io->xem_signal	= &ExternalSignals;
	xem_io->xem_screendepth	= CommDepth;

	xem_io->xem_swrite	= (APTR)xpr_swrite;
	xem_io->xem_sread		= (APTR)xpr_sread;
	xem_io->xem_sbreak	= (APTR)xem_sbreak;
	xem_io->xem_sstart	= (APTR)xem_sstart;
	xem_io->xem_sstop		= (APTR)xem_sstop;
	xem_io->xem_sflush	= (APTR)xpr_sflush;

	xem_io->xem_toptions	= (APTR)xpr_options;
	xem_io->xem_tgets		= (APTR)xem_tgets;
	xem_io->xem_tbeep		= (APTR)xem_tbeep;

	xem_io->xem_process_macrokeys = (APTR)ProcessMacroKeys;
}

/************************************************************************/

STATIC VOID __saveds __asm ProcessMacroKeys(register __a0 struct XEmulatorMacroKey *key)
{
	IMPORT struct	MacroKeys *MacroKeyBuffer;

	VOID (*routine)(VOID);

	if(routine = key->xmk_UserData)
		(*routine)();
	else
		SendModemCmd(MacroKeyBuffer->Keys[key->xmk_Qualifier][key->xmk_Code - 0x50]);
}

/************************************************************************/

VOID StopEmulator(struct XEM_IO *xem_io)
{
	XEmulatorCloseConsole(xem_io);

	XEmulatorCleanup(xem_io);

	CloseLibrary(XEmulatorBase);
	XEmulatorBase = NULL;
}

/************************************************************************/

BOOL StartEmulator(STRPTR libraryName)
{
	IMPORT struct List MacroKeysList;
	IMPORT struct Library *XEmulatorBase;
	IMPORT struct XEM_IO *xem_io;
	IMPORT UBYTE *TrashBuffer;
	IMPORT UBYTE *DirBuffer;

	if(XEmulatorBase = OpenLibrary(libraryName, XEM_LIBRARY_VERSION))
	{
		InitXEmReq(xem_io);

		xem_io->xem_console= NULL;

		if(XEmulatorSetup(xem_io))
		{
			UBYTE *ptr;

			strcpy(DirBuffer, "env:xem");
			strcpy(TrashBuffer, libraryName);
			if(ptr = strstr(TrashBuffer, ".lib"))
				*ptr = '\0';
			AddPart(DirBuffer, TrashBuffer, 256);

			if(XEmulatorPreferences(xem_io, DirBuffer, XEM_PREFS_LOAD) == FALSE)
			{
				mySimpleRequest("\"%s\" Failed To Read His Preferences-File\n \"%s\"\nThe Emulator Steps Now Back To His Internal Preferences", libraryName, DirBuffer);
				XEmulatorPreferences(xem_io, NULL, XEM_PREFS_RESET);
			}


			if(XEmulatorOpenConsole(xem_io))
			{
				XEmulatorMacroKeyFilter(xem_io, &MacroKeysList);

				return(TRUE);		/* emulator is ready to take off.. */
			}
			else
				mySimpleRequest("XEmulatorOpenConsole Failed..");
		}
		else
			mySimpleRequest("XEmulatorSetup Failed..");
	}
	else
		mySimpleRequest("Failed To OpenLibrary(%s);", library);


	return(FALSE);		/* something went bananas.. */
}

/************************************************************************/

VOID main(VOID)
{
	IMPORT struct Window		*CommWindow;
	IMPORT struct MsgPort	*SerialPort;
	IMPORT struct XEM_IO		*XEm_Request;
	IMPORT ULONG ExternalSignals;
	IMPORT

	/*
		Open libraries, allocate buffers, etc.
	*/
	if(AllocateResources()
	{
		/*
			Open Screen and Window, etc..
		*/
		if(CreateDisplay())
		{

			/*
				Open serial device
			*/
			if(CreateSerial("serial.device", 0))
			{
				if(StartEmulator("xemascii.library"))
				{
					ULONG window_signals, serial_signals, external_signals;
					ULONG signalset;
					BOOL quit_flag;

					window_signals = 1 << CommWindow->UserPort->mp_SigBit;
					serial_signals = 1 << SerialPort->mp_SigBit;

					StartSerial();

					for(quit_flag = FALSE; quit_flag == FALSE; )
					{
						signalset = Wait(window_signals | serial_signals | ExternalSignals);

						if(signalset & window_signals)
						{
							struct IntuiMessage *imsg;

							while(imsg = GetMsg(CommWindow->UserPort))
							{
								ULONG class;
								UWORD code;

								class	= imsg->Class;
								code	= imsg->Code;

								if(class != RAWKEY)
									ReplyMsg((struct Message *)imsg);


								switch(class)
								{
									case IDCMP_RAWKEY:
									{
										LONG actual;

										actual = XEmulatorUserMon(XEm_Request, ConsoleBuffer, 16, imsg);

										ReplyMsg((struct Message *)imsg);

										if(actual == 0  ||  actual == -1)
											break;

										WriteSerial(ConsoleBuffer, actual);

										if(Duplex == HALF)
											XEmulatorWrite(XEm_Request, ConsoleBuffer, actual);
									}
									break;

									case IDCMP_MENUPICK:
									{
										HandleMenu(code);
									}
									break;

								}
							}
						}

						if(signalset & serial_signals)
						{
							ULONG actual;

							actual = QueryAndReadIncomingSerialData();
							if(actual != 0)
							{
								STATIC struct HostData hostData;

								XEmulatorWrite(XEm_Request, SerialBuffer, actual);

								if(user_said__remove_ansi_sequences)
									hostData->Destination = CaptureBuffer;
								else
									hostData->Destination = NULL;

								hostData->Source = SerialBuffer;


								actual = XEmulatorHostMon(XEm_Request, hostData, actual);

								if(actual)
								{
									if(user_said__remove_ansi_sequences)
										FeedTheCapture(CaptureBuffer, actual);
									else
										FeedTheCapture(SerialBuffer, actual);
								}

								StartSerial();
							}
						}

						if(signalset & ExternalSignals)
						{
							if(XEmulatorSignal(XEm_Request, signalset) == FALSE)
							{
                         /* the most simple way */

								StartUserPreferredDefaultEmulator();

                         /* another possibility */

								StartInternalEmulator();

                /* or, not very smart; but possible, too */

								quit_flag = TRUE;

             /* you see, you can do whatever you want do do */
							}
						}
					}
					StopEmulator();
				}
				DeleteSerial();
			}
			DeleteDisplay();
		}
		FreeResources();
	}
}
