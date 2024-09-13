@kphp_should_fail k2_skip
/in a lambda inside castO2ToTypeofO1<BB, D1>/
/classof\(\) used incorrectly: it's a keyword to be used only for a single variable which is a generic parameter/
<?php

class BB {
    function method() { echo "B method\n"; return 1; }
}
class D1 extends BB {
    function dMethod() { echo "d1\n"; }
}

/**
 * @kphp-generic T1, T2
 * @param T1 $o1
 * @param T2 $o2
 * @return ?T1
 */
function castO2ToTypeofO1($o1, $o2) {
   /** @var T1 */
   $casted = null;
   // an error is missing use()
   (function() use($casted, $o2) {
        $casted = instance_cast($o2, classof($o1));
   })();
   echo get_class($casted), "\n";
   return $casted;
}

/** @var BB */
$bb = new D1;
castO2ToTypeofO1($bb, new D1);


interface I {}

class A implements I {
    public int $a;
    function __construct(int $a = 0) { $this->a = $a; }
    function af() { echo "A{$this->a}\n"; }
}
class B implements I {
    public int $b;
    function __construct(int $b = 0) { $this->b = $b; }
    function bf() { echo "B{$this->b}\n"; }
}

/** @var I[] */
$i_arr = [new A(1), new A(2), new B(10), new A(3), null, new B(11)];

/**
 * @kphp-generic T
 * @param class-string<T> $class_name
 * @return T[]
 */
function havingClass(array $i_arr, $class_name) {
    /** @var T[] */
    $result = [];
    foreach ($i_arr as $i) {
        $casted = instance_cast($i, $class_name);
        if ($casted)
            $result[] = $casted;
    }
    $result2 = array_filter(array_map(fn($i) => instance_cast($i, $class_name), $i_arr));
    if (count($result) != count($result2)) {
        echo "error: count != for $class_name\n";
    }
    return $result2;
}

/**
 * @kphp-generic T
 * @param T $obj
 * @return T[]
 */
function havingClassLike(array $i_arr, $obj) {
    return havingClass($i_arr, classof($obj));
}

/**
 * @kphp-generic T
 * @param T $obj
 * @param callable(T):void $fn
 */
function invokeForClassLike(array $i_arr, $obj, callable $fn) {
    foreach (havingClassLike($i_arr, $obj) as $i)
        $fn($i);
}


function demo1() {
    global $i_arr;
    foreach (havingClass($i_arr, A::class) as $a)
        $a->af();
    foreach (havingClass($i_arr, B::class) as $b)
        $b->bf();
}

function demo3() {
    global $i_arr;
    foreach (havingClassLike($i_arr, new A) as $a)
        $a->af();
    foreach (havingClassLike($i_arr, new B) as $b)
        $b->bf();
}

function demo4() {
    global $i_arr;
    invokeForClassLike($i_arr, new A, fn($a) => $a->af());
    invokeForClassLike($i_arr, new B, fn($b) => $b->bf());
}

demo1(); echo "\n";
demo3(); echo "\n";
demo4(); echo "\n";

