@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @param int $arg
 * @return shape(arg:int, arr:int[])
 */
function getT1($arg) {
    return shape(['arg' => $arg, 'arr' => [$arg]]);
}

/**
 * @return shape(s:string, t1:shape(arg:int, arr:int[]))
 */
function getT2() {
    return shape(['t1' => getT1(1), 's' => 'str']);
}

function demo() {
    $a = shape(['n' => 1, 's'=>'string', 'shape' => shape(['one'=>1, 's'=>'str2', 'arr'=>[1,2,3,'vars']]), 'a'=>new Classes\A]);

    // check that types are correctly inferred
    /** @var int $int */
    /** @var string $string */
    /** @var int $int_of_inner_shape */
    /** @var string $str_of_inner_shape */

    $int = $a['n'];
    $string = $a['s'];
    $in_shape = $a['shape'];
    /** @var Classes\A */
    $a_inst = $a['a'];
    $int_of_inner_shape = $a['shape']['one'];
    $str_of_inner_shape = $in_shape['s'];
    $var_2_of_inner_shape_array = $a['shape']['arr'][2];

    echo $int, "\n";
    echo $string, "\n";
    echo $a_inst->setA(9)->a, "\n";
    echo $int_of_inner_shape, "\n";
    echo $str_of_inner_shape, "\n";
    echo $var_2_of_inner_shape_array, "\n";
}

function demo2() {
    // check that types are correctly inferred
    /** @var int $int */
    /** @var string $str */

    $t2 = getT2();                  // shape <shape<...>, string>
    $str = $t2['s'];
    $int = $t2['t1']['arr'][0];
    echo $int, " ", $str, "\n";
}

function demo3() {
    $a = shape(['a' => shape(['b' => 1])]);
    echo $a['a']['b'], "\n";

    $a = $a;
    echo $a['a']['b'], "\n";
}

function demo4() {
    $a = shape(['a' => shape(['a' => 123])]);
    echo $a['a']['a'], "\n";

    $a = $a;
    echo $a['a']['a'], "\n";
}

demo();
demo2();
demo3();
demo4();
