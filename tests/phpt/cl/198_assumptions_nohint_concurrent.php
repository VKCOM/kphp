@ok
<?php

class A { function f() { echo "A f\n"; } }

// this all now works even without return typehints
function getA0() { $a = new A; return $a; }

// these functions are processed in parallel by different threads,
// but they have interdependencies also
// this test checks how it works with multithreading
function getA1() { $a = getA0(); return $a; }
function getA2() { $a = getA1(); return $a; }
function getA3() { $a = getA2(); return $a; }
function getA4() { $a = getA3(); return $a; }
function getA5() { $a = getA4(); return $a; }
function getA6() { $a = getA5(); return $a; }
function getA7() { $a = getA6(); return $a; }
function getA8() { $a = getA7(); return $a; }
function getA9() { $a = getA8(); return $a; }
function getA10() { $a = getA9(); return $a; }
function getA11() { $a = getA10(); return $a; }
function getA12() { $a = getA11(); return $a; }
function getA13() { $a = getA12(); return $a; }
function getA14() { $a = getA13(); return $a; }
function getA15() { $a = getA14(); return $a; }
function getA16() { $a = getA15(); return $a; }
function getA17() { $a = getA16(); return $a; }
function getA18() { $a = getA17(); return $a; }
function getA19() { $a = getA18(); return $a; }

getA19()->f();
getA12()->f();
getA5()->f();
