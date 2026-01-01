#
# Makefile for 'demo'
#

.c.o:
	@Echo "*e[33;1m  sc $(CFLAGS) $<*e[0m"
	sc $(CFLAGS) $*.c

CFLAGS = STRMERGE MCCONS NOWVRET NOSTKCHK OPTSIZE OPTIMIZE OPTGLOBAL OPTIMIZE

OBJS = demo.o

demo: $(OBJS)
	slink LIB:c.o $(OBJS) TO demo LIB LIB:sc.lib LIB:reqtools.lib SC SD ND
