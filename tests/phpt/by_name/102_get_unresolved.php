@kphp_should_fail k2_skip
/Syntax 'new \$class_name' not resolved: __construct\(\) not found in I/
/Syntax '\$class_name::method\(\)' not resolved: method unexistingSm not found in class A/
/Syntax '\$class_name::method\(\)' works only for static methods, but A::im is an instance method/
/Syntax '\$class_name::CONST' not resolved: const UNEXISTING_CONST not found in class A/
/Syntax '\$class_name::\$field' not resolved: field \$unexisting_field not found in class A/
/Syntax '\$class_name::\$field' works only for static fields, but A::\$idx is an instance field/
<?php

interface I {}
class A implements I {
    public int $idx = 0;
    function im() {}
}

/**
 * @kphp-generic T
 * @param class-string<T> $c
 */
function gCtor($c) { new $c; }

/**
 * @kphp-generic T
 * @param class-string<T> $c
 */
function gCallMethod($c) { $c::unexistingSm(); $c::im(); }

/**
 * @kphp-generic T
 * @param class-string<T> $c
 */
function gGetConst($c) { echo $c::UNEXISTING_CONST[0]; }

/**
 * @kphp-generic T
 * @param class-string<T> $c
 */
function gGetField($c) { $c::$unexisting_field += $c::$idx; }


gCtor(I::class);
gCallMethod(A::class);
gGetConst(A::class);
gGetField(A::class);
