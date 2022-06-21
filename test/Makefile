DISTFILES = Makefile q1 q2 q3 r1 r2 r3 mgrc1 mgrc2 mgrc3 testmg \
            alice.sum simple_sum.c alice.len

all: simple_sum

dist:
	ln $(DISTFILES) $(distdir)

simple_sum: simple_sum.c
	cc -o $@ $?
