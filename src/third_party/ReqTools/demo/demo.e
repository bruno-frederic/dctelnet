/************************************************
*                                		*
*  reqtools.library (V38)        		*
*                                		*
*  Release 2.5                   		*
*                                		*
*  (c) 1991-1994 Nico François   		*
*      1995-1996 Magnus Holmgren                *
*                                		*
*  demo.e                        		*
*				 		*
* Translated by: Marco Talamelli 		*
*				 		*
* EMail: Marco_Talamelli@amp.flashnet.it	*
*                                		*
* This source is public domain in all respects. *
*                                		*
************************************************/

MODULE 	'reqtools',
	'graphics/text',
	'libraries/reqtools',
	'utility/tagitem',
	'intuition/intuition',
	'utility/hooks'

CONST FILEREQ=0,REQINFO=1

PROC main()

DEF 	scrmodereq : PTR TO rtscreenmoderequester,filereq:PTR TO rtfilerequester,
	fontreq:PTR TO rtfontrequester,color,autoscroll,
	buf[120]:STRING,flist:PTR TO rtfilelist,filename[34]:STRING,
	ret,longnum,response

  IF reqtoolsbase:=OpenLibrary('reqtools.library',37)

	RtEZRequestA('ReqTools 2.0 Demo\n'+
	             '~~~~~~~~~~~~~~~~~\n'+
					 '''reqtools.library'' offers several\n'+
					 'different types of requesters:',
					 'Let\as see them', NIL, NIL,NIL)

	RtEZRequestA('NUMBER 1:\nThe larch :-)', 'Be serious!',NIL, NIL, NIL)

	RtEZRequestA('NUMBER 1:\nString requester\nfunction: RtGetStringA()',
					 'Show me', NIL,NIL, NIL)

	StrCopy(buf,'A bit of text',ALL)

	IF RtGetStringA(buf, 127, 'Enter anything:', NIL, 0)=NIL
		RtEZRequestA('You entered nothing :-(', 'I\am sorry', NIL,NIL, NIL)
	ELSE
		RtEZRequestA('You entered this string:\n \a\s\a.','So I did', NIL, [buf], NIL)
	ENDIF

	ret := RtGetStringA(buf, 127, 'Enter anything:', NIL,
			[RTGS_GADFMT,' _Ok |New _2.0 feature!|_Cancel',
			RTGS_TEXTFMT,
'These are two new features of ReqTools 2.0:\nText above the entry gadget and more than\none response gadget.',
				RT_UNDERSCORE, "_",NIL])

	IF (ret = 2) THEN RtEZRequestA('Yep, this is a new\nReqTools 2.0 feature!',
						 'Oh boy!', NIL,NIL, NIL)

	ret := RtGetStringA(buf, 127, 'Enter anything:', NIL,
				 [RTGS_GADFMT, ' _Ok |_Abort|_Cancel',
				 RTGS_TEXTFMT,
'New is also the ability to switch off the\nbackfill pattern.  You can also center the\ntext above the entry gadget.\nThese new features are also available in\nthe RtGetLongA() requester.',
				 RTGS_BACKFILL, FALSE,
				 RTGS_FLAGS, GSREQF_CENTERTEXT OR GSREQF_HIGHLIGHTTEXT,
				 RT_UNDERSCORE, "_",NIL])

	IF (ret = 2) THEN RtEZRequestA('What!! You pressed abort!?!\nYou must be joking :-)',
						 'Ok, Continue', NIL, NIL, NIL)


	RtEZRequestA('NUMBER 2:\nNumber requester\nfunction: RtGetLongA()',
					 'Show me', NIL, NIL, NIL)

	IF RtGetLongA({longnum}, 'Enter a number:', NIL,
						 [RTGL_SHOWDEFAULT, FALSE,
						 RTGL_MIN, 0, RTGL_MAX, 666,NIL])=FALSE

		RtEZRequestA('You entered nothing :-(','I\am sorry', NIL,NIL, NIL)
	ELSE
		RtEZRequestA('The number you entered was:\n\d\s',
	 'So it was', NIL,
		IF (longnum = 666) THEN [longnum, ' (you devil! :)'] ELSE  [longnum,'!'], NIL)
	ENDIF

	RtEZRequestA('NUMBER 3:\nMessage requester, the requester\n'+
			'you\ave been using all the time!\nfunction: RtEZRequestA()',
					 'Show me more', NIL,NIL, NIL)

	RtEZRequestA('Simplest usage: some body text and\na single centered gadget.',
					 'Got it', NIL,NIL, NIL)

	WHILE RtEZRequestA('You can also use two gadgets to\n'+
		'ask the user something.\n'+
		'Do you understand?', 'Of course|Not really',
		NIL, NIL,NIL)=FALSE DO RtEZRequestA('You are not one of the'+
		' brightest are you?\nWe\all try again...','Ok', NIL, NIL,NIL)

	RtEZRequestA('Great, we\all continue then.', 'Fine', NIL, NIL,NIL)

	response := RtEZRequestA('You can also put up a requester with\n'+
					'three choices.\n'+
					'How do you like the demo so far ?',
					'Great|So so|Rubbish', NIL, NIL,NIL)
	SELECT response
		CASE 0
			RtEZRequestA('Too bad, I really hoped you\nwould like it better.',
							 'So what', NIL, NIL,NIL)
		CASE 1
			RtEZRequestA('I\am glad you like it so much.', 'Fine', NIL, NIL,NIL)
		CASE 2
			RtEZRequestA('Maybe if you run the demo again\n'+
					 'you\all REALLY like it.',
					 'Perhaps', NIL, NIL,NIL)
	ENDSELECT

	ret := RtEZRequestA('The number of responses is not limited to three\n'+
				  'as you can see.  The gadgets are labeled with\n'+
				  'the return code from RtEZRequestA().\n'+
				  'Pressing Return will choose 4, note that\n'+
				  '4\as button text is printed in boldface.',
				  '1|2|3|4|5|0', NIL, NIL,
				  [RTEZ_DEFAULTRESPONSE, 4,NIL])

	RtEZRequestA('You picked \d.', 'How true', NIL, [ret], NIL)

	RtEZRequestA('New for Release 2.0 of ReqTools (V38) is\n'+
			  'the possibility to define characters in the\n'+
			  'buttons as keyboard shortcuts.\n'+
			  'As you can see these characters are underlined.\n'+
			  'Pressing shift while still holding down the key\n'+
			  'will cancel the shortcut.\n'+
			  'Note that in other requesters a string gadget may\n'+
			  'be active.  To use the keyboard shortcuts there\n'+
			  'you have to keep the Right Amiga key pressed down.',
			  '_Great|_Fantastic|_Swell|Oh _Boy',
			  NIL, NIL, [RT_UNDERSCORE, "_",NIL])

	RtEZRequestA('You may also use E-style formatting codes in the body text.\n'+
		'Like this:\n\n\aThe number \\d is written \\s\a. will give:\n\n'+
		'The number \d is written \s.\n\n'+
		'if you also pass 5 and ''"five"'' to RtEZRequestA().',
		'_Proceed', NIL, [5, 'five'],[RT_UNDERSCORE, "_",NIL])


	IF (RtEZRequestA('It is also possible to pass extra IDCMP flags\n'+
				  'that will satisfy RtEZRequestA(). This requester\n'+
				  'has had IDCMP_DISKINSERTED passed to it.\n'+
				  '(Try inserting a disk).',
				  '_Continue', NIL, NIL,[ RT_IDCMPFLAGS, IDCMP_DISKINSERTED,
				RT_UNDERSCORE, "_",NIL]) AND IDCMP_DISKINSERTED)
		RtEZRequestA('You inserted a disk.', 'I did', NIL, NIL,NIL)

	ELSE
		RtEZRequestA('You used the \aContinue\a gadget\n'+
					 'to satisfy the requester.', 'I did', NIL, NIL,NIL)
	ENDIF

	RtEZRequestA('Finally, it is possible to specify the position\n'+
				 'of the requester.\n'+
				 'E.g. at the top left of the screen, like this.\n'+
				 'This works for all requesters, not just rtEZRequest()!',
				 'Amazing', NIL,NIL,
				[RT_REQPOS, REQPOS_TOPLEFTSCR,RT_UNDERSCORE, "_",NIL])

	RtEZRequestA('Alternatively, you can center the\n'+
				  'requester on the screen.\n'+
				  'Check out \areqtools.doc\a for all the possibilities.',
				  'I\all do that', NIL, NIL,[RT_REQPOS, REQPOS_CENTERSCR,NIL])

	RtEZRequestA('NUMBER 4:\nFile requester\n'+
				  'function: RtFileRequestA()',
				  '_Demonstrate', NIL, NIL, [RT_UNDERSCORE, "_",NIL])

	IF (filereq := RtAllocRequestA(RT_FILEREQ, NIL))

		filename[0] := 0
		IF RtFileRequestA(filereq, filename, 'Pick a file',0)

			RtEZRequestA('You picked the file:\n\s\nin directory:\n\a\s\a',
					'Right', NIL, [filename, filereq.dir], NIL)
		ELSE
			RtEZRequestA('You didn\at pick a file.', 'No', NIL, NIL,NIL)

		ENDIF

		RtEZRequestA('The file requester has the ability\n'+
				  'to allow you to pick more than one\n'+
				  'file (use Shift to extend-select).\n'+
				  'Note the extra gadgets you get.',
				  '_Interesting', NIL, NIL, [RT_UNDERSCORE, "_",NIL])

		flist := RtFileRequestA(filereq, filename, 'Pick some files',
						[RTFI_FLAGS, FREQF_MULTISELECT,NIL])
		IF (flist) THEN;
			RtEZRequestA('You selected some files, this is\n'+
					 'the first one:\n\a\s\a\n'+
					 'All the files are returned as a linked\n'+
					 'list (see demo.e and reqtools.m).',
					 'Aha', NIL, [flist.name], NIL)


		RtFreeRequest(filereq)
	ELSE
		RtEZRequestA('Out of memory!', 'Oh boy!', NIL, NIL, NIL)

	ENDIF

	RtEZRequestA('The file requester can be used\n'+
			  'as a directory requester as well.',
			  'Let\as _see that', NIL, NIL, [RT_UNDERSCORE, "_"])

	IF (filereq := RtAllocRequestA(RT_FILEREQ, NIL))
		IF (RtFileRequestA(filereq, filename, 'Pick a directory',
					[RTFI_FLAGS, FREQF_NOFILES]))

			RtEZRequestA('You picked the directory:\n\a\s\a',
					 'Right', NIL, [filereq.dir], NIL)
		ELSE
			RtEZRequestA('You didn\at pick a directory.', 'No', NIL, NIL,NIL)

		RtFreeRequest(filereq)
		ENDIF
	ELSE
		RtEZRequestA('Out of memory!', 'Oh boy!', NIL, NIL,NIL)
	ENDIF

	RtEZRequestA('NUMBER 5:\nFont requester\n'+
			 'function: RtFontRequestA()',
			 'Show', NIL, NIL,NIL)

	IF (fontreq := RtAllocRequestA(RT_FONTREQ, NIL))

		fontreq.flags := FREQF_STYLE OR FREQF_COLORFONTS

		IF (RtFontRequestA(fontreq, 'Pick a font',0))

			RtEZRequestA('You picked the font:\n\a\s\a\nwith size:\n\a\d\a',
					'Right', NIL,
					[fontreq.attr.name, fontreq.attr.ysize], NIL)
		ELSE
			RtEZRequestA('You canceled.\nWas there no font you liked ?',
					 '_Nope', NIL, NIL, [RT_UNDERSCORE,"_",NIL])
		ENDIF

		RtFreeRequest(fontreq)
	ELSE
		RtEZRequestA('Out of memory!', 'Oh boy!', NIL, NIL,NIL)
	ENDIF

	RtEZRequestA('NUMBER 6:\nPalette requester\n'+
				 'function: RtPaletteRequestA()',
				 '_Proceed', NIL, NIL, [RT_UNDERSCORE, "_",NIL])

	color := RtPaletteRequestA('Change palette', NIL, NIL)

	IF (color = -1)
		RtEZRequestA('You canceled.\nNo nice colors to be picked ?',
						 'Nah', NIL, NIL,NIL)
	ELSE
		RtEZRequestA('You picked color number \d.', 'Sure did',
						 NIL, [color], NIL)
	ENDIF

	RtEZRequestA('NUMBER 7: (ReqTools 2.0)\n'+
				  'Volume requester\n'+
				  'function: RtFileRequestA() with\n'+
				  '          RTFI_VOLUMEREQUEST tag.',
				  '_Show me', NIL, NIL, [RT_UNDERSCORE, "_",NIL])

	IF (filereq := RtAllocRequestA(RT_FILEREQ, NIL))

		IF (RtFileRequestA(filereq, NIL, 'Pick a volume',[RTFI_VOLUMEREQUEST, NIL]))

			RtEZRequestA('You picked the volume:\n\a\s\a',
					'Right', NIL, [filereq.dir], NIL)
		ELSE
			RtEZRequestA('You didn\at pick a volume.', 'I did not', NIL, NIL,NIL)
		ENDIF

		RtFreeRequest(filereq)
	ELSE
		RtEZRequestA('Out of memory!', 'Oh boy!', NIL, NIL,NIL)
	ENDIF
	RtEZRequestA('NUMBER 8: (ReqTools 2.0)\n'+
			  'Screen mode requester\n'+
			  'function: rtScreenModeRequest()\n'+
			  'Only available on Kickstart 2.0!',
			  '_Proceed', NIL, NIL, [RT_UNDERSCORE, "_",NIL])

	IF KickVersion(38)=FALSE
		RtEZRequestA('Your Amiga doesn\at seem to have\n'+
					 'Kickstart 2.0 in ROM so I am not\n'+
					 'able to show you the Screen mode\n'+
					 'requester.\n'+
					 'So upgrade to 2.0 *now* :-)',
					 '_Allright', NIL, NIL, [RT_UNDERSCORE, "_",NIL])
	ELSE
		IF scrmodereq := RtAllocRequestA(RT_SCREENMODEREQ, NIL)

			IF (RtScreenModeRequestA(scrmodereq, 'Pick a screen mode:',
							[RTSC_FLAGS, SCREQF_DEPTHGAD OR
								     SCREQF_SIZEGADS OR
								     SCREQF_AUTOSCROLLGAD OR
								     SCREQF_OVERSCANGAD,
								     NIL]))

				autoscroll:=IF scrmodereq.autoscroll THEN 'On' ELSE 'Off'

				RtEZRequestA('You picked this mode:\n'+
							'ModeID  : $\h\n'+
							'Size    : \d x \d\n'+
							'Depth   : \d\n'+
							'Overscan: \d\n'+
							'AutoScroll \s',
							'Right', NIL,
							[scrmodereq.displayid,
							scrmodereq.displaywidth,
							scrmodereq.displayheight,
							scrmodereq.displaydepth,
							scrmodereq.overscantype,
							autoscroll], NIL)
			ELSE
				RtEZRequestA('You didn\at pick a screen mode.', 'Nope', NIL, NIL,NIL)

			RtFreeRequest(scrmodereq)
			ENDIF
		ELSE
			RtEZRequestA('Out of memory!', 'Oh boy!', NIL, NIL, NIL)
		ENDIF
		RtEZRequestA('That\as it!\nHope you enjoyed the demo',
				'_Sure did', NIL, NIL,[	RT_UNDERSCORE, "_",NIL])
	ENDIF
    CloseLibrary(reqtoolsbase)
  ELSE
    WriteF('Could not open reqtools.library!\n')
  ENDIF
ENDPROC
