<?php

function internal() {
    static $ss;
    $cb = function() {
    global $gg;
    $gg += 2;
    };
    $cb();
}

