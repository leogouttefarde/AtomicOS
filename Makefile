.PHONY: clean all

# Output directory for each submakefiles
OUTPUT := out
export OUTPUT

#
# Some build tools need to be explicitely defined before building. The toolchain
# creates the following platform tools configuration file before it allows the
# toolchain to build.
#
PLATFORM_TOOLS := $(OUTPUT)/platform-tools.mk
export PLATFORM_TOOLS

# stub PXE : necessaire pour le demarrage via QEmu
PXE = boot.pxe

QEMU = /usr/libexec/qemu-kvm

# QEMUOPTS = -no-kvm -net nic -net user,tftp="`pwd`",bootfile="$(PXE)" -boot n -cpu pentium -rtc base=localtime -m 64M -gdb tcp::1234
QEMUOPTS = -no-kvm -net nic -net user,tftp="`pwd`",bootfile="$(PXE)" -boot n -cpu pentium -rtc base=localtime -m 256M -gdb tcp::1234

all: | kernel/$(PLATFORM_TOOLS) user/$(PLATFORM_TOOLS)
	$(MAKE) -C user/ all VERBOSE=$(VERBOSE)
	$(MAKE) -C kernel/ kernel.bin VERBOSE=$(VERBOSE)

kernel/$(PLATFORM_TOOLS):
	$(MAKE) -C kernel/ $(PLATFORM_TOOLS)

user/$(PLATFORM_TOOLS):
	$(MAKE) -C user/ $(PLATFORM_TOOLS)

run: all
	$(QEMU) -kernel kernel/kernel.bin

# debug kvm (mieux)
debug: all
	cd kernel && $(QEMU) -kernel kernel.bin -gdb tcp::1234 -S &
	cd kernel && gdb -x gdb_debug kernel.bin

# debug pxe (si kvm non dispo)
pxe: all
	cd kernel && $(QEMU) $(QEMUOPTS) -S &
	cd kernel && gdb -x gdb_debug kernel.bin

clean:
	$(MAKE) clean -C kernel/
	$(MAKE) clean -C user/

iso: all
	cp -f kernel/kernel.bin iso/files/boot/kernel.bin
	cd iso && make

