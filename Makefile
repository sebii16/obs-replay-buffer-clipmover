TARGET = clipmover.exe
SRC = main.c

CC = cl
CFLAGS = /nologo /W4 /WX /std:c11

BUILD_DIR = build
OUT = $(BUILD_DIR)\$(TARGET)

all: release

$(BUILD_DIR):
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

debug: $(BUILD_DIR)
	$(CC) $(SRC) $(CFLAGS) /Od /Zi /MTd /Fo$(BUILD_DIR)\ /Fd$(BUILD_DIR)\ /Fe:$(OUT) /link /DEBUG:FULL /PDB:$(BUILD_DIR)\$(TARGET:.exe=.pdb) user32.lib

release: $(BUILD_DIR)
	$(CC) $(SRC) $(CFLAGS) /O2 /MT /DNDEBUG user32.lib /Fo$(BUILD_DIR)\ /Fe:$(OUT) /link /INCREMENTAL:NO /OPT:REF /OPT:ICF

clean:
	if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
