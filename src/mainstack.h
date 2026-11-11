/* mainstack.h -- the stack DCTelnet's main task needs.
 *
 * Workbench starts a program with the icon's do_StackSize and a Shell with
 * its own default, both 4096 bytes here. The kitty picture path decodes a PNG
 * inside VtWrite, on the main task's stack: kit_load (path[1024]) ->
 * png_decode -> vt_zinflate (Inf, about 1250 bytes) -> inf_dynamic (len[320]),
 * on top of the receive and parser frames. Measured on rig 3 (debug build
 * with a probe that fills the unused stack with 0xA5 and logs the low-water
 * mark after each picture draw): 3924 of 4096 bytes used in one run and
 * 5088 bytes in another, after a single 160x96 picture. The 4108-byte frame
 * of vt_find alone exceeds 4096. An overrun corrupts the memory under the
 * stack (alert 0100000F in FreeMem, F-line reset). 65536 is more than 12
 * times the 5088 measured.
 *
 * vbcc's startup (+aos68k, startup.o) honours a global `__stack` and runs
 * main() on a stack of at least that size, from Workbench and from a Shell.
 * The icons' do_StackSize carries the same number (tests/test_mainstack.c
 * keeps the code and both icons in step).
 */
#ifndef DCT_MAINSTACK_H
#define DCT_MAINSTACK_H

#define DCT_MAIN_STACK 65536L

#endif
