<?php

global $global_map;
$global_map[__FILE__] = true;

if (!function_exists('array_key_first_custom')) {
    function array_key_first_custom(array $a) {
        foreach ($a as $key => $_) {
            return $key;
        }
        return null;
    }
}
