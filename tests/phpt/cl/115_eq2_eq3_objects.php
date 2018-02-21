@ok
<?php
require_once 'Classes/autoload.php';

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

$v1 = 1;
$v1 = 'd';

echo $a3 == true, "\n";
echo $a3 == "1", "\n";
echo $a3 == false, "\n";
echo $b1 === $b1, "\n";
echo $b1 === $b2, "\n";
echo $b1 === 'b1', "\n";
echo $a1 === $b1, "\n";
echo $a1 === $a1, "\n";
echo $a1 === $a2, "\n";
echo $a1 === $a3, "\n";
echo $a1 === $a4, "\n";
echo $a1 === $a5, "\n";
echo $a1 === $a6, "\n";
echo $a4 === $a3, "\n";
echo $a4 == true, "\n";
echo $a6 === $a7, "\n";
echo $a1 === $a7, "\n";
echo $a1 === $v1, "\n";
echo $a1 === 2, "\n";
echo $a1 == null, "\n";
echo $a3 == true, "\n";
echo $a3 == "1", "\n";
echo $a3 == false, "\n";
echo $b1 === $b1, "\n";
echo $b1 === $b2, "\n";
echo $b1 === 'b1', "\n";
echo $a1 === $b1, "\n";
echo false == $a5, "\n";
echo false == $a5, "\n";
echo $b2 === $a7, "\n";
echo null == $a7, "\n";
echo $v1 === $b1, "\n";
echo $v1 == $b1, "\n";
echo $b1 == $b1, "\n";
echo $b1 == $a1, "\n";
echo $a1 == 0, "\n";
echo $a1 == 0.0, "\n";
echo $a1 == 1, "\n";
echo $a1 == 1.1, "\n";
echo $a1 == 'str', "\n";
echo $a1 == '', "\n";
echo is_object($v1), "\n";

/** @var A[] $arr */
$arr = array(new A);
echo $arr === 'asdf', "\n";
echo $arr[0] == false, "\n";
echo $arr[0] === $a1, "\n";
echo $arr[0] === '2', "\n";
echo $arr[0] === $arr[0], "\n";
echo is_object($arr[0]), "\n";

$a1 = new A;
$a1 = false;
echo $a1 == null, "\n";
echo $a1 == false, "\n";
echo $a1 == 0, "\n";
echo $a1 == 0.0, "\n";
echo $a1 == 1, "\n";
echo $a1 == 1.1, "\n";
echo $a1 == 'str', "\n";
echo $a1 === -9, "\n";
echo $a1 === 'str', "\n";
echo $a1 == '', "\n";
echo !$a1, "\n";
echo !!$a1, "\n";

