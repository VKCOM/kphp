@ok
<?php
require_once 'kphp_tester_include.php';

class A {}

function takeInt(int $a) { echo $a, "\n"; }
/** @param int[] $a */
function takeArrInt($a) { echo "n ", count($a), "\n"; }
function takeA(A $a) {}

/** @return int[] */
function getArrInt(): array {
    /** @var ?int[] $arr */
    $arr = [];
    return dropNull($arr);
}

/**
 * @kphp-generic T
 * @param ?T $data
 * @return not_null(T)
 */
function dropNull($data) {
  return not_null($data);
}

/**
 * @kphp-generic T
 * @param T $data
 * @return not_false<T>
 */
function dropFalse($data) {
  return not_false($data);
}

/**
 * @kphp-generic T
 * @param T $data
 * @return not_false<not_null<T>>
 */
function dropNullFalse($data) {
  return not_false(not_null($data));
}

function demo1() {
  /** @var ?int $i_or_null */
  $i_or_null = 3;
  takeInt(dropNull($i_or_null));
  takeInt(dropNull/*<int>*/(3+4 ? 1 : null));
  takeInt(dropNull(dropFalse($i_or_null)));
  takeArrInt([dropNull($i_or_null)]);
}

function demo2() {
    /** @var int|false $i_or_false */
    $i_or_false = 3 ? 3 : false;
    takeInt(dropFalse($i_or_false));
    takeInt(dropFalse/*<int|false>*/(3 ? 3 : false));
    $a = [];
    for ($i = 0; $i < 10; ++$i)
        $a[] = dropNull(dropFalse($i_or_false));
    takeArrInt($a);
    /** @var int|false|null $ifn */
    $ifn = 3;
    takeInt(dropFalse/*<int|false>*/(not_null($ifn)));
}

function demo3() {
    takeA(new A);
    takeA(dropFalse(new A));
    if (0) takeA(dropNull/*<A>*/(null));
}

function demo4() {
    /** @var mixed $v1 */
    $v1 = dropNull(3);
    /** @var string $v2 */
    $v2 = dropNull/*<string>*/('s');
    /** @var string $v3 */
    $v3 = dropNull/*<string>*/('s' ?: null);
    /** @var string $v4 */
    $v4 = dropNull/*<?string>*/('s');
    /** @var string $v5 */
    $v5 = dropNull/*<?string>*/('s' ?: null);
}

/** @param int|null|false $i */
function demo5($i) {
    /** @var int|null|false $i */
    takeInt(dropNullFalse($i));
}

/** @param not_null(int) $i */
function demo6($i) {
    /** @var not_false(not_null(int|false|null)) $j */
    $j = 0;
    takeInt($i);
    takeInt($j);
}

demo1();
demo2();
demo3();
demo4();
demo5(4);
demo6(10);
