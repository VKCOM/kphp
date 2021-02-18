@kphp_should_fail
KPHP_ERROR_ON_WARNINGS=1
/test_global_bad_modification_1\(A::\$ids\[0\]\);/
/Dangerous undefined behaviour function call, \[var = A::\$ids\]/
/subcall3\(\$v\);/
/Dangerous undefined behaviour function call, \[foreach var = \$yy3\]/
/accepts5\(A::\$ids, \$v\);/
/Dangerous undefined behaviour function call, \[var = A::\$ids\]/
/A::\$ids\[\] = 0;/
/Dangerous undefined behaviour push_back, \[foreach var = A::\$ids\]/
/test_global_bad_modification_8\(\$a_id_ref\);/
/Dangerous undefined behaviour function call, \[foreach var = A::\$ids\]/
/test_global_bad_modification_18\(\$a_id_ref\);/
/Dangerous undefined behaviour function call, \[foreach var = A::\$ids\]/
/test_global_bad_modification_20\(A::\$ids\[0\]\);/
/Dangerous undefined behaviour function call, \[var = A::\$ids\]/
<?php

class A {
    static public $ids = [1,2,3];
    static public $user_id = 100;
}

// --------------

function subcall1() {
    A::$ids[] = 1;
}

function test_global_bad_modification_1(&$v) {
    subcall1();
}

function demo1() {
    test_global_bad_modification_1(A::$ids[0]);
}

demo1();


// --------------

$yy3 = [];

function subcall3(&$v) {
    global $yy3;
    $v = 1;
    $yy3[] = $v;
}

function test_global_bad_modification_3(&$arr) {
    foreach ($arr as &$v) {
        subcall3($v);
    }
}

test_global_bad_modification_3($yy3);

// --------------

function accepts5(&$a, &$b) {
}

function test_global_bad_modification_5(&$v) {
    accepts5(A::$ids, $v);
}

test_global_bad_modification_5(A::$ids);     // unsafe, as there become two refs to a single global

// --------------

function test_global_bad_modification_6(&$arr) {
    foreach ($arr as &$ref) {
        A::$ids[] = 0;
        $ref = 0;
    }
    var_dump($arr);
}

test_global_bad_modification_6(A::$ids);


// --------------

function test_global_bad_modification_8(&$v) {
    A::$ids[] = 10;
    echo $v;
}

foreach (A::$ids as &$a_id_ref) {
    test_global_bad_modification_8($a_id_ref);
}


// --------------

function test_global_bad_modification_18(&$v) {
    A::$ids[] = 10;
    echo $v;
}

function demo18(&$arr) {
    foreach ($arr as &$a_id_ref) {
        test_global_bad_modification_18($a_id_ref);
    }
}

function wrap18(&$arr) {
    demo18($arr);
}

wrap18(A::$ids);


// --------------


function subcall20_1() {
    subcall20_2();
}

function subcall20_2() {
    if (0) subcall20_1();
    subcall20_3();
}

function subcall20_3() {
    if (0) subcall20_1();
    if (0) subcall20_2();
    if (0) subcall20_3();
    if (1) {
        A::$ids[] = 1;
    }
}

function test_global_bad_modification_20(&$item) {
    subcall20_1();
}

test_global_bad_modification_20(A::$ids[0]);
