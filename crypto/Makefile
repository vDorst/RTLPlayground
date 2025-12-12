BOOTLOADER_ADDRESS=0x100

IMAGESIZE = 524288
CONFIG_LOCATION = 458752
HTML_LOCATION = 262144

CC = sdcc
CC_FLAGS = -mmcs51
ASM = sdas8051
AFLAGS= -plosgff

SUBDIRSCLEAN=$(addsuffix clean)

BUILDDIR = output/

all: create_build_dir $(SUBDIRS) $(BUILDDIR)rtlplayground.bin

create_build_dir:
	mkdir -p $(BUILDDIR)

SRCS = siphash.c siphash_test_prg.c
OBJS = ${SRCS:%.c=$(BUILDDIR)%.rel}

clean:
	-rm -r $(BUILDDIR)

$(BUILDDIR)crtstart.rel: ../crtstart.asm
	$(ASM) $(AFLAGS) -o $@ $<

$(BUILDDIR)%.rel: %.c
	$(CC) $(CC_FLAGS) -o $@ -c $<

$(BUILDDIR)%.rel: $(BUILDDIR)%.asm
	${ASM} ${AFLAGS} -o $@ $<
#	mv -f $(addprefix $(basename $^), .lst .rel .sym) .

$(BUILDDIR)rtlplayground.ihx: $(BUILDDIR)crtstart.rel $(OBJS)
	$(CC) $(CC_FLAGS) -Wl-bHOME=${BOOTLOADER_ADDRESS} -Wl-r -o $@ $^
	# $(CC) $(CC_FLAGS) -Wl-bHOME=${BOOTLOADER_ADDRESS}  -Wl-bBANK1=0x14000 -Wl-r -o $@ $^

$(BUILDDIR)rtlplayground.img: $(BUILDDIR)rtlplayground.ihx
	objcopy --input-target=ihex -O binary $< $@

$(BUILDDIR)rtlplayground.bin: $(BUILDDIR)rtlplayground.img
	if [ -e $@ ]; then rm $@; fi
	echo "0000000: 00 40" | xxd -r - $@
	cat $< >> $@
# 	truncate --size=16K $@
# 	dd if=$< skip=80 bs=1024 >>$@
# 	tools/$(BUILDDIR)fileadder -a $(CONFIG_LOCATION) -s $(IMAGESIZE) -d config.txt $@
# 	tools/$(BUILDDIR)fileadder -a $(HTML_LOCATION) -s $(IMAGESIZE) -d html -p html_data $@


.PHONY: clean all $(SUBDIRS)
