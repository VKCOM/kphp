<?php

var_dump(__FILE__ . " required");

if (!function_exists('array_key_first')) {
    function array_key_first(array $a) {
        foreach ($a as $key => $_) {
            return $key;
        }
        return null;
    }
}
