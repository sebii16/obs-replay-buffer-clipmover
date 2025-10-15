# OBS Replay Buffer Clipmover

**This program watches your clip folder for newly added clips and automatically moves them into the correct subfolder based on the focused window.**

I made this for **personal use** and there might be an OBS plugin for this, but maybe someone finds it useful.

## Installation

### Option 1 - Prebuilt Binary
You can download a **precompiled executable** from the **[Releases](https://github.com/sebii16/obs-replay-buffer-clipmover/releases)** page.


### Option 2 - Build It Yourself
```
git clone https://github.com/sebii16/obs-replay-buffer-clipmover.git
cd obs-replay-buffer-clipmover
```

- Using **nmake** (recommended)
```
nmake
```

- Manually using **CL**
```
cl main.c /nologo /W4 /WX /std:c11 /O2 /MT /DNDEBUG user32.lib /Fobuild\ /Fe:build\clipmover.exe /link /INCREMENTAL:NO /OPT:REF /OPT:ICF
```

- Or using **GCC**
```
gcc main.c -std=c11 -Wall -Wextra -Werror -O2 -DNDEBUG -s -o clipmover.exe -luser32 -lpsapi
```
