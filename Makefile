TARGET = clipmover.exe
SRC = main.c clipmover.c

CC = cl
CFLAGS = /nologo /W4 /WX /std:c11

BUILD_DIR = build
OUT = $(BUILD_DIR)\$(TARGET)

all: release

$(BUILD_DIR):
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

debug: $(BUILD_DIR)
	$(CC) $(SRC) $(CFLAGS) /Od /Zi /MTd user32.lib /Fo$(BUILD_DIR)\ /Fd$(BUILD_DIR)\ /Fe:$(OUT) /link /DEBUG:FULL /PDB:$(BUILD_DIR)\$(TARGET:.exe=.pdb)

release: $(BUILD_DIR)
	$(CC) $(SRC) $(CFLAGS) /Os /MT /GL /DNDEBUG user32.lib shell32.lib /Fo$(BUILD_DIR)\ /Fe:$(OUT) /link /INCREMENTAL:NO /OPT:REF /OPT:ICF /DEBUG:NONE /LTCG
	
gcc:
	gcc $(SRC) -std=c11 -Wall -Wextra -Werror -O2 -DNDEBUG -s -o $(OUT) -luser32 -lpsapi

clean:
	if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
