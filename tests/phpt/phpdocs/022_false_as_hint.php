@ok
<?php

/** @param string|false $s */
function takeStrOrFalse($s) {}

function callTakeStrOrFalse() {
    takeStrOrFalse(false);
    $f = false;
    takeStrOrFalse($f);
}
callTakeStrOrFalse();

function takeBool(bool $s) {}

/** @param false $b */
function pFalseInferFalse($b) {
    takeStrOrFalse($b);
    takeBool($b);
}
pFalseInferFalse(false);

function pInferBool($b) {
    takeBool($b);
}
pInferBool(false);
pInferBool(true);

/**
 * @return false
 */
function getFalse() { return false; }
getFalse();

/** @param bool[] $arr */
function takeBoolArr(array $arr = ['k'=>false]) {}
takeBoolArr(['d' => false]);
