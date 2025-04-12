// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/kprintf.h"
#include "common/server/signals.h"
#include "common/tl/compiler/tl-parser-new.h"
#include "common/version-string.h"

#define        VERSION_STR        "tlc-0.01"


int output_expressions_fd = -1;
int schema_version = 4;
static void usage_and_exit() {
  printf("%s\n", get_version_string());
  printf("usage: tl-compiler [-v] [-h] <TL-schema-file-list>\n"
         "\tTL compiler\n"
         "\t-v\toutput statistical and debug information into stderr\n"
         "\t-E <file>\twhenever is possible output to file expressions\n"
         "\t-e <file>\texport serialized schema to file\n"
         "\t-w\t custom version of serialized schema (2 - very old, 3 - old, 4 - current (default))\n"
  );
  exit(2);
}

int vkext_write(const char *filename) {
  int f = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0640);
  assert (f >= 0);
  write_types(f);
  close(f);
  return 0;
}

int main(int argc, char **argv) {
  init_version_string(VERSION_STR);
  int i;
  char *vkext_file = 0;
  char *output_expressions_filename = 0;
  set_debug_handlers();
  while ((i = getopt(argc, argv, "E:ho:ve:w:")) != -1) {
    switch (i) {
      case 'E':
        output_expressions_filename = optarg;
        break;
      case 'o':
        //unused
        break;
      case 'h':
        usage_and_exit();
        return 2;
      case 'e':
        vkext_file = optarg;
        break;
      case 'w':
        schema_version = atoi(optarg);
        break;
      case 'v':
        verbosity++;
        break;
    }
  }

  assert(2 <= schema_version && schema_version <= 4);

  if (argc == optind) {
    usage_and_exit();
  }

  if (output_expressions_filename) {
    output_expressions_fd = open(output_expressions_filename, O_CREAT | O_WRONLY | O_TRUNC, 0640);
    assert (output_expressions_fd >= 0);
  }

  int files_count = argc - optind;
  struct parse files[files_count];
  for (i = optind; i != argc; ++i) {
    if (!tl_init_parse_file(argv[i], files + (i - optind))) {
      return 2;
    }
  }
  struct tree *T = tl_parse_lex(files, files_count);
  if (!T) {
    fprintf(stderr, "Error in parse\n");
    tl_print_parse_error();
    return 3;
  }
  if (verbosity) {
    fprintf(stderr, "Parse ok\n");
  }
  if (!tl_parse(T)) {
    if (verbosity) {
      fprintf(stderr, "Fail\n");
    }
    return 1;
  }
  if (verbosity) {
    fprintf(stderr, "Ok\n");
  }


  if (vkext_file) {
    vkext_write(vkext_file);
  }
  if (output_expressions_filename) {
    close(output_expressions_fd);
  }
  return 0;
}
