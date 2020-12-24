<?php

global $global_map;
$global_map[__FILE__] = true;

if (!function_exists('str_contains')) {
  function str_contains(string $haystack, string $needle): bool {
    return '' === $needle || false !== strpos($haystack, $needle);
  }
}
