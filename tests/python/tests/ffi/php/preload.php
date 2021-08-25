<?php

$to_load = [
  getenv('PHP_FFI_LOAD1'),
  getenv('PHP_FFI_LOAD2'),
  getenv('PHP_FFI_LOAD3'),
];

foreach ($to_load as $name) {
  if (!$name) {
    continue;
  }
  FFI::load(__DIR__ . '/../c/' . $name . '.h');
}
