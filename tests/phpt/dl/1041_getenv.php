@ok k2_skip
<?php

function test_getenv_all() {
  $envs = getenv();
  // clear internal KPHP variables
  unset($envs['ASAN_OPTIONS']);
  unset($envs['UBSAN_OPTIONS']);
  unset($envs['TZ']);
  var_dump($envs);
}

function test_getenv() {
  var_dump(getenv(''));
  var_dump(getenv('', true));
  var_dump(getenv('unexisting_var'));
  var_dump(getenv('unexisting_var', true));
  var_dump(getenv('home'));
  var_dump(getenv('HOME'));
  var_dump(getenv('HOME', true));
  var_dump(getenv('SHELL'));
  var_dump(getenv('SHELL', true));
}

test_getenv_all();
test_getenv();
