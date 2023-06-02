@ok php8
<?php
require_once 'kphp_tester_include.php';

use Classes\A;

use Classes\B;

const CONST_A = new A(42);
const B1 = new B(CONST_A);
const B2 = new B(CONST_A);
const B3 = new B(new A(42));

echo(B1);
echo(B3);
echo(B2);
var_dump(B1 === B2);
var_dump(B1 === B3);
var_dump(B2 === B3);
