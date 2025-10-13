# OBS Replay Buffer clipmover

**This program watches your clip folder for newly added clips and automatically moves them into the correct subfolder based on the focused window.**

I made this for **personal use** and there might be an OBS plugin for this, but maybe someone finds it useful.

## Installation

### Option 1 - Prebuilt Binary
You can download a **precompiled executable** from the **[Releases](https://github.com/sebii16/obs-replay-buffer-clipmover/releases)** page. (Coming soon!)


### Option 2 - Build It Yourself
```
git clone https://github.com/sebii16/obs-replay-buffer-clipmover.git
cd obs-replay-buffer-clipmover
```

- Using **nmake** (recommended)
```
nmake
```

- Manually using **MSVC**
```
cl main.c /nologo /W4 /WX /std:c11 /O2 /GL /DNDEBUG /Fe:clipmover.exe /link /LTCG /INCREMENTAL:NO /OPT:REF /OPT:ICF
```

- Or using **GCC**
```
gcc main.c -std=c11 -Wall -Wextra -Werror -O2 -flto -DNDEBUG -s -o clipmover.exe -luser32 -lpsapi
```

## Example Usage

```
.\clipmover.exe c:\clips
```

**You can create a shortcut so you don't have to run it manually every time.**

Also if you use **Windows Terminal** and care about **minimum RAM usage** while gaming, run it through **conhost** as **WT** adds about **15-25 MB** of unnecessary RAM overhead.
Use this as **"Target"** in your shortcut:
```
conhost -- .\clipmover.exe c:\clips
```
