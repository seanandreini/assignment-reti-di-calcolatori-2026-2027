#include <stddef.h>
#include <stdio.h>

#include "../lib/argparse/argparse.h"

static const char *const usages[] = {
  "tcp [options] [[--] args]",
  "tcp [options]",
  NULL
};

int main(int argc, const char **argv){
  int port = 12345;
  int listening_mode = 0;

  struct argparse_option options[] = {
    OPT_HELP(),
    OPT_GROUP("basic options"),
    OPT_BOOLEAN('l', "port", &listening_mode, "run in listening mode (server)", NULL, 0, 0),
    OPT_INTEGER('p', "port", &port, "port of connection", NULL, 0, 0),
    OPT_END()
  };
  
  struct argparse argparse;
  argparse_init(&argparse, options, usages, 0);
  argparse_describe(&argparse, "prova", "prova2");

  argc = argparse_parse(&argparse, argc, argv);

  printf("%d %d", listening_mode, port);

  return 0;
}