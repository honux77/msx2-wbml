# wbml Windows reference emulator — MinGW + vendored SDL2.
# Run from repo root:  mingw32-make -C src/win-ref
# Output: build/wbml-ref.exe (+ SDL2.dll copied alongside)

ROOT    := ../..
SDL     := $(ROOT)/third_party/SDL2
BUILD   := $(ROOT)/build

CC      := gcc
CFLAGS  := -O2 -std=c11 -Wall -Wextra -I$(SDL)/include/SDL2 -Iz80
LDFLAGS := -L$(SDL)/lib
LIBS    := -lmingw32 -lSDL2main -lSDL2

SRC := main.c machine.c mc8123.c video.c z80/z80.c
OBJ := $(SRC:.c=.o)

EXE := $(BUILD)/wbml-ref.exe

all: $(EXE)

$(EXE): $(SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SRC) -o $(EXE) $(LDFLAGS) $(LIBS) -mwindows
	@cp -u $(SDL)/bin/SDL2.dll $(BUILD)/SDL2.dll

# console build keeps stdout/stderr visible for debugging
console: $(SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SRC) -o $(EXE) $(LDFLAGS) $(LIBS)
	@cp -u $(SDL)/bin/SDL2.dll $(BUILD)/SDL2.dll

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -f $(EXE)

.PHONY: all console clean
