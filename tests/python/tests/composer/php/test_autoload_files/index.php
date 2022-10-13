<?php

var_dump('printed before any files are required');

$global_map = [];

// note: not using require_once here on purpose;
// composer will not require autoload files again;
// using the convoluted path on purpose
require 'vendor/autoload.php';

var_dump('printed after autoload.php is required');

// second require should not include any autoload files
require 'vendor/autoload.php';
require 'vendor/autoload.php'; // same as above

// should do nothing: file is already required by autoload.php
require_once 'vendor/vk/api/functions.php';

var_dump(z_test());

ksort($global_map);

var_dump(
  $global_map,

  array_key_first_custom([]),
  array_key_first_custom([1, 2]),
  array_key_first_custom(['first' => 1, 'second' => 2]),

  str_contains('', ''),
  str_contains('x', ''),
  str_contains('', 'x'),
  str_contains('foo', 'fo'),
  str_contains('foo', 'bar'),

  vk_api_version()
);
