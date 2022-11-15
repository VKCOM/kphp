<?php

global $in_plain_115;
if(!isset($in_plain_115))
    $in_plain_115 = 0;

function plain_115() {
    require_once __DIR__ . '/plain-details/plain-internal.php';
    internal_115();
}
