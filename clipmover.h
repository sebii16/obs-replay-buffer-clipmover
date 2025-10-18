#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define VERSION "v1.1.1"
#define CONFIG_FILE L".\\clipmover.ini"
#define DEFAULT_MOVE_DELAY 300

typedef struct {
  wchar_t clip_folder[MAX_PATH];
  uint16_t move_delay;
  bool auto_adj_delay, use_conhost, missing;
} config_t;

config_t load_config(const wchar_t *path);
void move_new_clips(config_t *config);
int setup(void);
void relaunch_with_conhost(int argc, char **argv);
