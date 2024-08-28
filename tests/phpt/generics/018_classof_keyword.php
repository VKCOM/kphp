@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class B {
    const NAME = 'B';
    function method() { echo "B method\n"; return 1; }
}
class D1 extends B {
    const NAME = 'D1';
    function dMethod() { echo "d1\n"; }
}
class D2 extends B {
    const NAME = 'D2';
    function dMethod() { echo "d2\n"; }
}

/**
 * @kphp-generic T, DstClass
 * @param T $obj
 * @param class-string<DstClass> $to_classname
 * @return DstClass
 */
function my_cast($obj, $to_classname) {
    return instance_cast($obj, $to_classname);
}

/**
 * @kphp-generic T, DstClass
 * @param T $obj
 * @param class-string<DstClass> $if_classname
 */
function callDMethodIfNotNull($obj, $if_classname) {
    /** @var DstClass */
    $casted = my_cast($obj, $if_classname);
    if ($casted)
        $casted->dMethod();
    else
        echo "cast to $if_classname is null: obj is ", get_class($obj), "\n";
}

/** @var B */
$b = new D1;
my_cast($b, D1::class)->dMethod();

callDMethodIfNotNull(new D1, D1::class);
callDMethodIfNotNull(new D1, D2::class);
callDMethodIfNotNull(new D2, D1::class);
callDMethodIfNotNull(new D2, D2::class);

/**
 * @kphp-generic TElem, ToName
 * @param TElem[] $arr
 * @param class-string<ToName> $to
 * @return ToName[]
 */
function my_array_cast($arr, $to) {
    $out = [];
    array_reserve_from($out, $arr);
    foreach ($arr as $k => $v) {
        $out[$k] = instance_cast($v, $to);
    }
    return $out;
}

/**
 * @param B[] $arr
 */
function demoCastAndPrintAllD1(array $arr) {
    $casted_arr = my_array_cast($arr, D1::class);
    foreach ($casted_arr as $obj)
        if ($obj) $obj->dMethod();
}

demoCastAndPrintAllD1([new D1, new D2, new D1]);

/**
 * @kphp-generic T1, T2
 * @param T1 $o1
 * @param T2 $o2
 * @return ?T1
 */
function castO2ToTypeofO1($o1, $o2) {
   /** @var T1 */
   $casted = instance_cast($o2, classof($o1));
   echo get_class($casted), "\n";
   return $casted;
}

/** @var B */
$bb = new D1;
castO2ToTypeofO1($bb, new D1);


/**
 * @kphp-generic T
 * @param T $o1
 */
function castToClassofLocal($o1) {
    $casted = instance_cast($o1, classof($o1));
    echo $casted ? "can cast\n" : "can't cast\n";
}

castToClassofLocal(new D1);
castToClassofLocal(new D2);



/**
 * @kphp-generic T
 * @param class-string<T> $class_name
 */
function printNameConst($class_name) {
    echo 'NAME=', $class_name::NAME, "\n";
}

function acceptO(object $o) {
    printNameConst(classof($o));
}

acceptO(new B);
acceptO(new D2);
