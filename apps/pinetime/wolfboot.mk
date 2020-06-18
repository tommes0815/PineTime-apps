ifdef WOLFBOOT_OFFSET

WOLFBOOT:=$(abspath $(CURDIR)/../../wolfboot/)

SIGNTOOL ?= python3 $(WOLFBOOT)/tools/keytools/sign.py
KEYGENTOOL ?= python3 $(WOLFBOOT)/tools/keytools/keygen.py

BINFILE ?= $(BINDIR)/$(APPLICATION).bin
WOLFBOOT_KEYFILE ?= $(WOLFBOOT)/ecc256.der
WOLFBOOT_BIN ?= $(WOLFBOOT)/wolfboot.bin

export IMAGE_HDR_SIZE ?= 256

IMAGE_VERSION?=1
SIGN_BINFILE = $(BINDIR)/$(APPLICATION)_v$(IMAGE_VERSION)_signed.bin


wolfboot-create-key: $(WOLFBOOT_KEYFILE)

$(WOLFBOOT_KEYFILE):
	make -C $(WOLFBOOT) clean
	make -C $(WOLFBOOT) distclean
	cp -f wolfboot.config $(WOLFBOOT)/.config
	make -C $(WOLFBOOT) TARGET=nrf52 SIGN=ECC256 DEBUG=0 ecc256.der 

wolfboot: wolfboot-create-key link
	@$(COLOR_ECHO)
	@$(COLOR_ECHO) '$(COLOR_PURPLE)Re-linking for wolfBoot at $(WOLFBOOT_OFFSET)...$(COLOR_RESET)'
	@$(COLOR_ECHO)
	@$(COLOR_ECHO) 'sources:'
	@$(COLOR_ECHO) $(SRC)
	@$(COLOR_ECHO)
	$(_LINK) $(LINKFLAGPREFIX)--defsym=_rom_start_addr="$$(($(WOLFBOOT_OFFSET) + $(IMAGE_HDR_SIZE)))" \
	$(LINKFLAGPREFIX)--defsym=length="$$(($(WOLFBOOT_PARTITION_SIZE) - $(IMAGE_HDR_SIZE)))" \
	$(LINKFLAGPREFIX)--defsym=image_header="$(IMAGE_HDR_SIZE)" -o $(ELFFILE) && \
	$(OBJCOPY) $(OFLAGS) -Obinary $(ELFFILE) $(BINFILE) && \
	$(SIGNTOOL) $(BINFILE) $(WOLFBOOT_KEYFILE) $(IMAGE_VERSION) $(WOLFBOOT_OFFSET)
	@$(COLOR_ECHO)
	@$(COLOR_ECHO) '$(COLOR_PURPLE)Signed with $(WOLFBOOT_KEYFILE) for version $(IMAGE_VERSION) \
		$(COLOR_RESET)'
	@$(COLOR_ECHO)


$(WOLFBOOT_BIN):
	@$(COLOR_ECHO) 'sources:'
	@$(COLOR_ECHO) $(SRC)
	@$(COLOR_ECHO)
	make -C $(WOLFBOOT) clean
	cp -f wolfboot.spi_drv_nrf52.h $(WOLFBOOT)/hal/spi/spi_drv_nrf52.h
	cp -f wolfboot.config $(WOLFBOOT)/.config
	make -C $(WOLFBOOT) wolfboot.bin

.PHONY: wolfboot-flash-bootloader wolfboot-flash wolfboot-create-key wolfboot-bootloader

wolfboot-bootloader: $(WOLFBOOT_BIN)

wolfboot-flash-bootloader: HEXFILE = $(WOLFBOOT_BIN)
wolfboot-flash-bootloader: $(WOLFBOOT_BIN) $(FLASHDEPS)
	sudo $(FLASHER) $(FFLAGS) -o 0x0

wolfboot-flash: HEXFILE = $(SIGN_BINFILE)
wolfboot-flash: wolfboot $(FLASHDEPS) wolfboot-flash-bootloader
	sudo  $(FLASHER) $(FFLAGS) -o $(WOLFBOOT_OFFSET)

wolfboot-clean:
	make -C $(WOLFBOOT) clean

else
wolfboot:
	$(Q)echo "error: wolfboot not supported on board $(BOARD)!"
	$(Q)false
endif # WOLFBOOT_OFFSET
