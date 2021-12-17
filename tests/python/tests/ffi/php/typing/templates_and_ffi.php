<?php

#ifndef KPHP
if (false)
#endif
  FFI::load(__DIR__ . '/../c/vector.h');

/**
 * @kphp-template T
 * @param T $ffi_vec
 */
function printXY($ffi_vec) {
    echo $ffi_vec->x, ' ', $ffi_vec->y, "\n";
}

/**
 * @kphp-template T
 * @kphp-param T $ffi_vec
 * @kphp-return T
 */
function getEqVec($ffi_vec) {
    return $ffi_vec;
}

/**
 * @kphp-template T
 * @kphp-param T $vec_arr
 * @kphp-return T::data[]
 */
function getInnerVecOfArr($vec_arr) {
    return [$vec_arr->data];
}

/**
 * @kphp-template T
 * @kphp-param T $ffi_vec
 */
function getLambdaCapturingFfiTpl($ffi_vec): callable {
    return function($add) use($ffi_vec) {
        return fn() => $ffi_vec->y + $add;
    };
}

/**
 * @kphp-template T
 * @param T $scope
 * @param any $value
 */
function createAndAssign($scope, $value) {
    $foo = $scope->new('struct Foo');
    $foo->value = $value;
    var_dump($foo->value);
}



function test1() {
    $lib = FFI::scope('vector');
    $vec1 = $lib->new('struct Vector2');
    $vec2 = $lib->new('struct Vector2f');
    printXY($vec1);
    printXY($vec2);
}

function test2() {
    $lib = FFI::scope('vector');
    var_dump(getEqVec($lib->new('struct Vector2'))->y);
    var_dump(getEqVec($lib->new('struct Vector2f'))->x);
    printXY(getEqVec($lib->new('struct Vector2')));
}

function test3() {
    $lib = FFI::scope('vector');
    $vec = getInnerVecOfArr($lib->new('struct Vector2Array'));
    echo $vec[0]->y, "\n";
}

function test4() {
    $lib = FFI::scope('vector');
    $lamb = getLambdaCapturingFfiTpl($lib->new('struct Vector2'));
    echo $lamb(10)(), "\n";
    echo getLambdaCapturingFfiTpl($lib->new('struct Vector2'))(20)(), "\n";
}

function test5() {
    $cdef1 = FFI::cdef('
        struct Foo {
            int8_t value;
        };
    ');
    $cdef2 = FFI::cdef('
        struct Foo {
            bool value;
        };
    ');

    createAndAssign($cdef1, 10);
    createAndAssign($cdef2, true);
}

test1();
test2();
test3();
test4();
test5();
