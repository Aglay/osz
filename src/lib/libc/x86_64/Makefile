include ../../../../Config
SRCS = $(wildcard *.c)
SRCS+= $(filter-out crt0.S,$(wildcard *.S))
TMP = $(SRCS:.c=.o)
OBJS = $(TMP:.S=.o)
LIBS =

CFLAGS = -Wall -fpic -ffreestanding -fPIC -fno-stack-protector -nostdinc
ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG=1
endif

all: $(OBJS) ok

ok:
	@cp -f platform.h ../../../../etc/include/sys 2>/dev/null 1>/dev/null || true
	@true

%.o: %.S
	@#echo "  src		$<"
	@$(CC) -I../../../../etc/include -I../../../core/$(ARCH) $(OPT) $(CFLAGS) -c $< -o $@

%.o: %.c
	@#echo "  src		$<"
	@$(CC) -I../../../../etc/include $(OPT) $(CFLAGS) -c $< -o ../$@

clean:
	@rm *.o 2>/dev/null || true
