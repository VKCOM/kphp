@ok
<?php

function test_int_casts() {
  var_dump ((int) 1);
  var_dump ((int) 1.5);
  var_dump ((int) "12");
  var_dump ((int) "1.2");
  var_dump ((int) "hello");
  var_dump ((int) null);
  var_dump ((int) false);
  var_dump ((int) true);
}

function test_float_casts() {
  var_dump ((float) 1);
  var_dump ((float) 1.5);
  var_dump ((float) "12");
  var_dump ((float) "1.2");
  var_dump ((float) "hello");
  var_dump ((float) null);
  var_dump ((float) false);
  var_dump ((float) true);
}

function test_double_casts() {
  var_dump ((double) 1);
  var_dump ((double) 1.5);
  var_dump ((double) "12");
  var_dump ((double) "1.2");
  var_dump ((double) "hello");
  var_dump ((double) null);
  var_dump ((double) false);
  var_dump ((double) true);
}

function test_string_casts() {
  var_dump ((string) 1);
  var_dump ((string) 1.5);
  var_dump ((string) "12");
  var_dump ((string) "1.2");
  var_dump ((string) "hello");
  var_dump ((string) null);
  var_dump ((string) false);
  var_dump ((string) true);
}

function test_array_casts() {
  var_dump ((array) 1);
  var_dump ((array) 1.5);
  var_dump ((array) "12");
  var_dump ((array) "1.2");
  var_dump ((array) "hello");
  var_dump ((array) []);
  var_dump ((array) null);
  var_dump ((array) false);
  var_dump ((array) true);
}

function test_bool_casts() {
  var_dump ((bool) 0);
  var_dump ((bool) 1);
  var_dump ((bool) 1.5);
  var_dump ((bool) "");
  var_dump ((bool) "12");
  var_dump ((bool) "1.2");
  var_dump ((bool) "hello");
  var_dump ((bool) []);
  var_dump ((bool) null);
  var_dump ((bool) false);
  var_dump ((bool) true);
}

function test_boolean_casts() {
  var_dump ((boolean) 0);
  var_dump ((boolean) 1);
  var_dump ((boolean) 1.5);
  var_dump ((boolean) "");
  var_dump ((boolean) "12");
  var_dump ((boolean) "1.2");
  var_dump ((boolean) "hello");
  var_dump ((boolean) []);
  var_dump ((boolean) null);
  var_dump ((boolean) false);
  var_dump ((boolean) true);
}

test_int_casts();
test_float_casts();
test_double_casts();
test_string_casts();
test_array_casts();
test_bool_casts();
test_boolean_casts();
