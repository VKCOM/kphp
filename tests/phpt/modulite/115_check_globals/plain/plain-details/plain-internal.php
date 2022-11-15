<?php

function internal_115() {
    static $ss;
    $cb = function() {
    global $gg;
    $gg += 2;
    };
    $cb();
}

