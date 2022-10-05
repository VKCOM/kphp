<?php

class Either008_ints {
  /** @var int[] */
  public $data;
  /** @var mixed[] */
  public $errors;
}


/**
 * @kphp-generic T, TOut
 * @param T[] $arr
 * @param callable(T):TOut $mapper
 * @return TOut[]
 */
function my_map(array $arr, callable $mapper) {
    /** @var TOut[] $result */
    $result = [];
    foreach ($arr as $k => $v)
        $result[$k] = $mapper($v);
    return $result;
}

function test_either_creation() {
    $either = Either008::data(new Either008_ints, [1,2,3]);
    var_dump(Either008::getData($either));
    \Logic008\TestLogic008::printEitherIsError('test_either_creation', $either);
}

function internal_f_non_generic(string $prefix) {
    echo "(internal $prefix)\n";
}

function internal_f_generic(string $prefix, object $obj) {
    echo "(internal gen $prefix)\n";
}
