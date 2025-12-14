BUILD_DIR = build
CACERT = usr/share/ssl/root/ca-certificates.crt
OUT = pkg/bin/sayt

APE_PREFIX = mise exec http:cosmocc --
CP = $(APE_PREFIX) cp.ape
MKDIR = $(APE_PREFIX) mkdir.ape
RM = $(APE_PREFIX) rm.ape
FIND = mise exec http:cosmos-find -- find
ZIP = mise exec http:cosmos-zip -- zip

CC = mise exec http:cosmocc -- cosmocc
# Use tiny mode with correct flags. -I. must be before -I cosmopolitan to pick up custom config.h
# Added -DTINY to trigger mbedtls optimizations
CFLAGS = -Os -mtiny -DTINY -D_COSMO_SOURCE -I. -I cosmopolitan -include stdbool.h

# Find sources
HTTP_SRCS := $(shell $(FIND) cosmopolitan/net/http -name "*.c")
HTTPS_SRCS := $(shell $(FIND) cosmopolitan/net/https -name "*.c")
MBEDTLS_SRCS := $(shell $(FIND) cosmopolitan/third_party/mbedtls -name "*.c" -not -path "*/test/*")

SRCS := sayt.c $(HTTP_SRCS) $(HTTPS_SRCS) $(MBEDTLS_SRCS)
OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

.PHONY: all

all: $(OUT)

$(OUT): $(BUILD_DIR)/sayt
	@$(MKDIR) -p $(dir $@)
	$(CP) $< $@

$(BUILD_DIR)/sayt: $(OBJS) $(CACERT)
	@$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $(OBJS)
	$(ZIP) -r $@ usr

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -rf $(BUILD_DIR) $(OUT)
