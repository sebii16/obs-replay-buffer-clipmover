#include "clipmover.h"

int main(int argc, char **argv) {

  printf("Clipmover %s by sebii16 (github.com/sebii16)\n\n", VERSION);

  config_t cfg = load_config(CONFIG_FILE);

  if (cfg.missing) {
    if (setup() != 0) {
      return 1;
    }

    cfg = load_config(CONFIG_FILE);
  }

  if (cfg.use_conhost)
    relaunch_with_conhost(argc, argv);

  puts("Make sure OBS Replay Buffer is turned on\n");

  move_new_clips(&cfg);

  return 0;

  (void)argc;
}
