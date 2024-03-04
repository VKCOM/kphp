@ok php8
<?php
require_once 'kphp_tester_include.php';

use Classes\A;

const OBJ = new A(42);

class B {
    const X = OBJ;
}

echo (B::X);
var_dump(B::X === OBJ);
var_dump(B::X === new A(42));
