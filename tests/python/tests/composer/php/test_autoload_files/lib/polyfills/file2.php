<?php

var_dump(__FILE__ . " required");

if (!function_exists('str_contains')) {
  function str_contains(string $haystack, string $needle): bool {
    return '' === $needle || false !== strpos($haystack, $needle);
  }
}
