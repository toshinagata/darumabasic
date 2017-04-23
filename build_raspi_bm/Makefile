#  Makefile for BareMetal PiBasic
#  2017.4.23. Toshi Nagata
#  Copyright 2017 Toshi Nagata. All rights reserved.

pi2:
	$(MAKE) -f Makefile_sub RASPPI=2

pi1:
	$(MAKE) -f Makefile_sub RASPPI=1

all:
	$(MAKE) clean
	$(MAKE) pi1
	$(MAKE) clean
	$(MAKE) pi2
	mkdir boot; cp _boot/* boot; cp kernel.img kernel7.img boot; cp -R ../basic boot; zip -r daruma_raspi_bm.zip boot -x \*.DS_Store; rm -rf boot; mv -f daruma_raspi_bm.zip ../binaries

clean:
	$(MAKE) -f Makefile_sub clean

