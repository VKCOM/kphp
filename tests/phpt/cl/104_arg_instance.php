@ok
<?php
require_once 'polyfills.php';

use Classes\A;

/**
 * @return Classes\A[]
 */
function getAArr() {
  $a1 = new A();
  $a1->a = 999;
  return array($a1, new A());
}

function printAOf(A $a) {
  $a->printA();
}

/**
 * @param $a A
 */
function printAOf2($a) {
  $a->printA();
}

/**
 * @param A $a
 */
function printAOf3($a) {
  $a->printA();
}


/**
 * @param Classes\A[] $arr
 */
function printAOfArr($arr) {
  printAOf($arr[0]);
  if (count($arr) > 0)
    $arr[0]->printA();
}

/**
 * @param $arr Classes\A[]
 */
function printAOfArr2($arr) {
  printAOf2($arr[0]);
  printAOf3($arr[0]);
  if (count($arr) > 0)
    $arr[0]->printA();
}

printAOfArr(getAArr());
printAOfArr2(getAArr());
