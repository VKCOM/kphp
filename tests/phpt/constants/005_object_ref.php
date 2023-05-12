@ok php8
<?php
require_once 'kphp_tester_include.php';

use Classes\A;

const ZERO1 = new A(0);
const ZERO2 = new A(0);
const ZERO3 = ZERO1;

var_dump(ZERO1 === ZERO1);
var_dump(ZERO1 === ZERO2);
var_dump(ZERO1 === ZERO3);

var_dump(ZERO2 === ZERO1);
var_dump(ZERO2 === ZERO2);
var_dump(ZERO2 === ZERO3);

var_dump(ZERO3 === ZERO1);
var_dump(ZERO3 === ZERO2);
var_dump(ZERO3 === ZERO3);
