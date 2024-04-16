#include "shelly_.h"

char cmd[MAX_CMD_SIZE], *argv[MAX_CMDS];
int argc;
operand_counter data;


int main() {
  struct passwd *p = getpwent();

  while (TRUE) {
    _print_cmdline(p);
    _read_cmd(&argc, argv, &data);
    _process_cmd(&argc, argv, &data);
  }
  return 0;
}