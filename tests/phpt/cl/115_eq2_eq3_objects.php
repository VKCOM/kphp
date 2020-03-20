@ok
<?php
require_once 'polyfills.php';

use Classes\A;
use Classes\B;

$a1 = new A();
$a2 = new A();
$b1 = new B();
$b2 = new B();
$a3 = $a1;
$a4 = $a1;
$a5 = $a4;
$a6 = $a2;
$a7 = $a6;

echo $b1 === $b1, "\n";
echo $b1 === $b2, "\n";
echo $a1 === $b1, "\n";
echo $a1 === $a1, "\n";
echo $a1 === $a2, "\n";
echo $a1 === $a3, "\n";
echo $a1 === $a4, "\n";
echo $a1 === $a5, "\n";
echo $a1 === $a6, "\n";
echo $a4 === $a3, "\n";
echo $a6 === $a7, "\n";
echo $a1 === $a7, "\n";
echo $b1 === $b1, "\n";
echo $b1 === $b2, "\n";
echo $a1 === $b1, "\n";
echo $b2 === $a7, "\n";
echo is_object($a1), "\n";
echo is_object($a5), "\n";

$a5 = null;
$a7 = null;

echo $b1 === $b1, "\n";
echo $b1 === $b2, "\n";
echo $a1 === $b1, "\n";
echo $a1 === $a1, "\n";
echo $a1 === $a2, "\n";
echo $a1 === $a3, "\n";
echo $a1 === $a4, "\n";
echo $a1 === $a5, "\n";
echo $a1 === $a6, "\n";
echo $a4 === $a3, "\n";
echo $a6 === $a7, "\n";
echo $a1 === $a7, "\n";
echo $b1 === $b1, "\n";
echo $b1 === $b2, "\n";
echo $a1 === $b1, "\n";
echo $b2 === $a7, "\n";
echo is_object($a1), "\n";
echo is_object($a5), "\n";

/** @var A[] $arr */
$arr = array(new A);
echo $arr[0] === $a1, "\n";
echo $arr[0] === $arr[0], "\n";
echo $arr[0] === $a7, "\n";
echo is_object($arr[0]), "\n";
echo is_object($arr[100500]), "\n";
echo isset($arr[100500]), "\n";

$a1 = null;
echo !$a1, "\n";
echo !!$a1, "\n";

