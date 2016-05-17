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

QEMU = /usr/libexec/qemu-kvm
# QEMUOPTS = -no-kvm -net nic -net user,tftp="`pwd`",bootfile="$(PXE)" -boot n -cpu pentium -rtc base=localtime -m 64M -gdb tcp::1234
QEMUOPTS = -gdb tcp::1234 -kernel kernel/kernel.bin

all: | kernel/$(PLATFORM_TOOLS) user/$(PLATFORM_TOOLS)
	$(MAKE) -C user/ all VERBOSE=$(VERBOSE)
	$(MAKE) -C kernel/ kernel.bin VERBOSE=$(VERBOSE)

kernel/$(PLATFORM_TOOLS):
	$(MAKE) -C kernel/ $(PLATFORM_TOOLS)

user/$(PLATFORM_TOOLS):
	$(MAKE) -C user/ $(PLATFORM_TOOLS)

run: all
	$(QEMU) $(QEMUOPTS)

debug:
	$(QEMU) $(QEMUOPTS) -S &
	gdb -x kernel/gdb_debug kernel/kernel.bin

clean:
	$(MAKE) clean -C kernel/
	$(MAKE) clean -C user/

