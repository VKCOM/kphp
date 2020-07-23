@ok
<?php
require_once 'polyfills.php';
use Classes\F;

/**
 * @kphp-infer
 * @param int[] $intArr
 */
function moreInt(&$intArr) {
    $intArr[] = 100;
}

/**
 * @param F[] $fArr
 */
function patch($fArr) {
    $fArr[1]->intArr['strkey'] = 8;
    moreInt($fArr[1]->intArr);
}

$f = new F;
$f->appendInt(1);
$f->appendInt(2);
$f->appendString('3');
$f->stringArr[] = '4';
$f->stringArr['strkey'] = 'strvalue';
moreInt($f->intArr);
moreInt($f->intArr);

print_r($f->intArr);
print_r($f->stringArr);

/** @var F[] */
$arr = [new F, new F];
$arr[0]->intArr[] = 1;
moreInt($arr[0]->intArr);

print_r($arr[1]->intArr);
patch($arr);
print_r($arr[1]->intArr);
