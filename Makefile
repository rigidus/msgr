# ---- Project layout ---------------------------------------------------------
SRC_DIR     := src
INC_DIR     := include
BIN_DIR     := bin

# Общие исходники (шаблон цикла и утилиты)
COMMON_SRCS := $(SRC_DIR)/event_loop.c $(SRC_DIR)/common.c
COMMON_OBJS := $(COMMON_SRCS:.c=.o)

# Приложения
UI_IN_SRCS      := $(SRC_DIR)/ui_in_msg.c
UI_OUT_SRCS     := $(SRC_DIR)/ui_out_msg.c
CRYPTOR_SRCS    := $(SRC_DIR)/cryptor.c
COMM_SRCS       := $(SRC_DIR)/communicator.c

UI_IN_OBJS      := $(UI_IN_SRCS:.c=.o)
UI_OUT_OBJS     := $(UI_OUT_SRCS:.c=.o)
CRYPTOR_OBJS    := $(CRYPTOR_SRCS:.c=.o)
COMM_OBJS       := $(COMM_SRCS:.c=.o)

UI_IN_BIN       := $(BIN_DIR)/ui_in_msg
UI_OUT_BIN      := $(BIN_DIR)/ui_out_msg
CRYPTOR_BIN     := $(BIN_DIR)/cryptor
COMM_BIN        := $(BIN_DIR)/communicator

# ---- Toolchain & flags ------------------------------------------------------
CC       ?= cc
CSTD     ?= -std=c11
WARN     := -Wall -Wextra -Wpedantic
OPT      := -O2
DEFS     := -D_DEFAULT_SOURCE
INCS     := -I$(INC_DIR)

CFLAGS   := $(CSTD) $(WARN) $(OPT) $(DEFS) $(INCS)

# Детект OS (на случай специфики)
UNAME_S := $(shell uname -s)

# ---- OpenSSL only for cryptor ----------------------------------------------
# Пытаемся взять флаги из pkg-config, если есть
OPENSSL_CFLAGS := $(shell pkg-config --cflags openssl 2>/dev/null)
OPENSSL_LIBS   := $(shell pkg-config --libs   openssl 2>/dev/null)

# Если pkg-config не нашёл, пробуем частые пути Homebrew/MacPorts
ifeq ($(strip $(OPENSSL_LIBS)),)
  ifeq ($(UNAME_S),Darwin)
    # Популярные пути Homebrew (x86_64 и arm64) и MacPorts
    ifneq ("$(wildcard /opt/homebrew/opt/openssl@3)","")
      OPENSSL_PREFIX := /opt/homebrew/opt/openssl@3
    else ifneq ("$(wildcard /usr/local/opt/openssl@3)","")
      OPENSSL_PREFIX := /usr/local/opt/openssl@3
    else ifneq ("$(wildcard /opt/local/libexec/openssl3)","")
      OPENSSL_PREFIX := /opt/local
      OPENSSL_CFLAGS += -I$(OPENSSL_PREFIX)/include
      OPENSSL_LIBS   += -L$(OPENSSL_PREFIX)/lib -lcrypto
    endif
    ifneq ($(OPENSSL_PREFIX),)
      OPENSSL_CFLAGS += -I$(OPENSSL_PREFIX)/include
      OPENSSL_LIBS   += -L$(OPENSSL_PREFIX)/lib -lcrypto
    endif
  else
    # Linux: попробуем просто линковаться к -lcrypto
    OPENSSL_LIBS   += -lcrypto
  endif
endif

# ---- Targets ----------------------------------------------------------------
.PHONY: all clean dirs help

all: dirs $(UI_IN_BIN) $(UI_OUT_BIN) $(CRYPTOR_BIN) $(COMM_BIN)

dirs:
	@mkdir -p $(BIN_DIR)

# ui_in_msg (без внешних либ)
$(UI_IN_BIN): $(COMMON_OBJS) $(UI_IN_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# ui_out_msg (без внешних либ)
$(UI_OUT_BIN): $(COMMON_OBJS) $(UI_OUT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# cryptor (требует только OpenSSL/libcrypto)
$(CRYPTOR_BIN): $(COMMON_OBJS) $(CRYPTOR_OBJS)
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) $^ -o $@ $(OPENSSL_LIBS)

# communicator (сетевые либы не требуются в MVP; добавите по мере необходимости)
$(COMM_BIN): $(COMMON_OBJS) $(COMM_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Общие правила компиляции .c → .o
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(SRC_DIR)/*.o
	@rm -f $(BIN_DIR)/ui_in_msg $(BIN_DIR)/ui_out_msg $(BIN_DIR)/cryptor $(BIN_DIR)/communicator

help:
	@echo "Targets:"
	@echo "  all        - build all binaries (default)"
	@echo "  clean      - remove objects and binaries"
	@echo "  dirs       - create bin/ directory"
	@echo "  help       - this help"
	@echo ""
	@echo "Variables you can override:"
	@echo "  CC=<compiler>    (default: cc)"
	@echo "  CSTD='-std=c11'  (or -std=gnu11)"
	@echo "  BIN_DIR=bin SRC_DIR=src INC_DIR=include"
