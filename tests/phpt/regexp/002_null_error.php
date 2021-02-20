@ok
<?php

function test_preg_replace_error() {
  $result = preg_replace('/ds', '_', 'hello_world');
  var_dump($result, $result === null);
}

function test_preg_replace_array_error() {
  $result = preg_replace('/ds', [], 'hello_world');
  var_dump($result, $result === false);
}

function test_preg_replace_callback_error() {
  $result = preg_replace_callback('/ds/', function ($m) { return '_'; }, 'hello_world');
  var_dump($result, $result === null);
}

test_preg_replace_error();
test_preg_replace_callback_error();
