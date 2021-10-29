<?php

const LIBC_SEEK_END = 2;

function test() {
  $libc = FFI::cdef('
      extern int errno;

      const char *getenv(const char *name);

      int abs(int number);
      long int labs(long int number);

      int puts(const char *s);

      size_t strlen(const char *s);

      int atoi(const char *s);
      long int atol(const char *s);

      struct File;

      struct File *fopen(const char *filename, const char *mode);
      int fclose(struct File *f);
      int fseek(struct File *f, long int offset, int whence);
      long int ftell(struct File *f);
      int fgetc (struct File *f);
  ', 'libc.so.6');


  $libc->puts("hello\0");
  $libc->puts($libc->getenv("HOME\0") . "\0");

#ifndef KPHP
  flush();
#endif

  var_dump($libc->getenv("HOME\0"));

  var_dump([$libc->abs(-10), $libc->abs(10)]);
  var_dump([$libc->labs(-100), $libc->labs(100)]);

  $php_str = "foo\0bar";
  var_dump([strlen($php_str), $libc->strlen($php_str)]);

  var_dump($libc->atoi("134"));
  var_dump($libc->atol("13434"));

  $file = $libc->fopen(__FILE__, "r");
  var_dump(['ftell' => $libc->ftell($file)]);
  $read_result = '';
  for ($i = 0; $i < strlen("<?php"); ++$i) {
    $read_result .= chr($libc->fgetc($file));
  }
  var_dump($read_result); // Should be "<?php"
  var_dump(['ftell' => $libc->ftell($file)]);
  var_dump(['fseek' => $libc->fseek($file, 0, LIBC_SEEK_END)]);
  var_dump(['ftell' => $libc->ftell($file)]);
  var_dump(['close file' => $libc->fclose($file)]);

  $libc->fopen('/a/b/c/nonexisting', 'r');
  var_dump($libc->errno);
  $libc->errno = 5;
  var_dump($libc->errno);
}

test();
