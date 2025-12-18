# Tools:
APE_PREFIX = mise exec http:cosmocc --
CP = $(APE_PREFIX) cp.ape
MKDIR = $(APE_PREFIX) mkdir.ape
RM = $(APE_PREFIX) rm.ape
FIND = mise exec http:cosmos-find -- find
ZIP = mise exec http:cosmos-zip -- zip
CC = mise exec http:cosmocc -- cosmocc
STRIP_X86_64 = mise exec http:cosmocc -- x86_64-linux-cosmo-strip
STRIP_AARCH64 = mise exec http:cosmocc -- aarch64-linux-cosmo-strip

# Params:
CACERT = usr/share/ssl/root/ca-certificates.crt
CERTS_H = $(BUILD_DIR)/certs.h
BUILD_DIR = build
BUILD_APE = $(BUILD_DIR)/sayt.com
BUILD_ELF = $(BUILD_DIR)/sayt.elf
PKG_BIN_DIR = pkg/bin
PKG_BIN_APE = $(PKG_BIN_DIR)/sayt.com
PKG_BIN_X64 = $(PKG_BIN_DIR)/sayt-amd64.elf
PKG_BIN_ARM64 = $(PKG_BIN_DIR)/sayt-arm64.elf
SAYT_BUILD_VERSION ?= dev

# Flags
CFLAGS = -Os -mtiny -DTINY -D_COSMO_SOURCE -DSAYT_BUILD_VERSION='"$(SAYT_BUILD_VERSION)"' -I. -I cosmopolitan -include stdbool.h

# Find sources (excluding sayt.c)
HTTP_SRCS := $(shell $(FIND) cosmopolitan/net/http -name "*.c")
HTTPS_SRCS := $(shell $(FIND) cosmopolitan/net/https -name "*.c")
MBEDTLS_SRCS := $(shell $(FIND) cosmopolitan/third_party/mbedtls -name "*.c" -not -path "*/test/*")
LIB_SRCS := $(HTTP_SRCS) $(HTTPS_SRCS) $(MBEDTLS_SRCS)
LIB_OBJS := $(addprefix $(BUILD_DIR)/,$(LIB_SRCS:.c=.o))

# sayt.o variants
SAYT_OBJ_APE = $(BUILD_DIR)/sayt-ape.o
SAYT_OBJ_ELF = $(BUILD_DIR)/sayt-elf.o

.PHONY: all clean

all: $(PKG_BIN_APE) $(PKG_BIN_X64) $(PKG_BIN_ARM64)

$(PKG_BIN_APE): $(BUILD_APE)
	@$(MKDIR) -p $(dir $@)
	$(CP) $< $@

$(PKG_BIN_X64): $(BUILD_ELF)
	@$(MKDIR) -p $(dir $@)
	$(STRIP_X86_64) -o $@ $(BUILD_ELF).com.dbg

$(PKG_BIN_ARM64): $(BUILD_ELF)
	@$(MKDIR) -p $(dir $@)
	$(STRIP_AARCH64) -o $@ $(BUILD_ELF).aarch64.elf

$(CERTS_H): $(CACERT)
	@$(MKDIR) -p $(dir $@)
	@echo "// Auto-generated from $(CACERT)" > $@
	@echo "#define EMBEDDED_CA_CERTS_DATA \\" >> $@
	@sed 's/\\/\\\\/g; s/"/\\"/g; s/$$/\\n" \\/; s/^/"/' $(CACERT) >> $@
	@echo '""' >> $@

$(SAYT_OBJ_APE): sayt.c
	@$(MKDIR) -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ -v

$(BUILD_APE): $(SAYT_OBJ_APE) $(LIB_OBJS) $(CACERT)
	@$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(SAYT_OBJ_APE) $(LIB_OBJS) -v
	$(ZIP) -r $@ usr

$(SAYT_OBJ_ELF): sayt.c $(CERTS_H)
	@$(MKDIR) -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -include $(CERTS_H) -c $< -o $@

$(BUILD_ELF): $(SAYT_OBJ_ELF) $(LIB_OBJS)
	@$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(SAYT_OBJ_ELF) $(LIB_OBJS)

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -rf $(BUILD_DIR) $(PKG_BIN_DIR)
