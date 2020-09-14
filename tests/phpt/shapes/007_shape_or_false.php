@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @kphp-infer
 * @param int $arg
 * @return shape(a:string[], n:int, arg:int)
 */
function getShape($arg) {
    return shape(['n' => 1, 'a' => ['str', 'array'], 'arg' => $arg]);
}

/**
 * @kphp-infer
 * @param int|false $arg
 * @return shape(a:string[], n:int, arg:int)|false
 */
function getShapeOrFalse($arg) {
    if(!$arg)
        return false;

    return getShape($arg);
}

/**
 * @kphp-infer
 * @return shape(a:string[], n:int, arg:int)|false
 */
function getAlwaysFalse() {
    return getShapeOrFalse(false);
}

function demo1() {
    $t1 = getShapeOrFalse(88);
    echo $t1 ? "t1 arg = " . $t1['arg'] : "t1 is false", "\n";
}

function demo2() {
    $t1 = getShapeOrFalse(10);
    if ($t1 === false) {
        echo "t1 is false", "\n";
    } else {
        echo "t1 arg = " . $t1['arg'], "\n";
    }

    $t1 = getShapeOrFalse(20);
    if (!$t1) {
        echo "t1 is false", "\n";
    } else {
        echo "t1 arg = " . $t1['arg'], "\n";
    }

    $t1 = getShapeOrFalse(false);
    if (!$t1) {
        echo "t1 is false", "\n";
    } else {
        echo "t1 arg = " . $t1['arg'], "\n";
    }
}

function demo3() {
    $t1 = getAlwaysFalse();
    $t2 = $t1;

    if (!$t1 && !$t2) {
        echo "both false\n";
    }
}

function demo4() {
    $t = getAlwaysFalse();
    if($t) {
        // type inferring works (see .cpp and comment-guaranteed)
        /** @var int $int */
        /** @var string[] $str_array */
        /** @var false|int $int_or */
        $int = $t['n'];
        $str_array = $t['a'];
        $int_or = $t['arg'];
    }
}

demo1();
demo2();
demo3();
demo4();
