@ok k2_skip
<?php

interface MyStringable { function __toString(); }
class A implements MyStringable {
    public int $value = 111;
    function __toString() { return "A $this->value"; }
}
class B implements MyStringable {
    public string $value = 'b222';
    function __toString() { return "B $this->value"; }
}

function f2(int $a, string $b) {
    echo "f2opt: a $a b $b\n";
}

function f2opt(int $a, string $b, int $c = 0) {
    echo "f2opt: a $a b $b c $c\n";
}

function f2optopt(int $a, string $b, int $c = 0, int $d = 0) {
    echo "f2optopt: a $a b $b c $c d $d\n";
}

function fWithNullable(?int $a, ?string $b = null) {
    echo "fWithNullable a $a b $b\n";
}

class WithF2 {
    function __construct(int $a, string $b, int $c = 0) {
        echo "__construct: a $a b $b c $c\n";
    }

    function f2optopt(int $a, string $b, int $c = 0, int $d = 0) {
        echo "WithF2::f2optopt: a $a b $b c $c d $d\n";
    }

    /**
     * @kphp-generic ...TArgs
     * @param TArgs ...$args
     */
    function callFWithNullable(...$args) {
        fWithNullable(...$args);
    }
}

function f3(int $a, string $b, int $c) {
    echo "f3: a $a b $b c $c\n";
}

/**
 * @kphp-generic ...TArgs
 * @param TArgs ...$args
 */
function callF2(...$args) {
    echo "count = ", count($args), "\n";
    f2(...$args);
    (new WithF2(...$args))->f2optopt(...$args);
}

/**
 * @kphp-generic ...TArgs
 * @param TArgs ...$args
 */
function callF3(...$args) {
    echo "count = ", count($args), "\n";
    f2opt(...$args);
    f3(...$args);
}

function demoManualProvide() {
    $f = new WithF2(0, '');
    $f->callFWithNullable/*<?int>*/(null);
    $f->callFWithNullable/*<?int, ?string>*/(null, null);
    $f->callFWithNullable/*<?int, ?string>*/(33, 's33');
}

/**
 * @kphp-generic ...TArgs
 * @param TArgs ...$all
 */
function printAllAsStrings(string $prefix, ...$all) {
    $arr = [...$all];
    foreach ($arr as $v) {
        echo $prefix, (string)$v, "\n";
    }
}

/**
 * @kphp-generic ...TArgs
 * @param TArgs ...$all
 */
function printAllAsStringsTwice(string $prefix, ...$all) {
    foreach ([...$all, ...$all] as $v) {
        echo $prefix, (string)$v, "\n";
    }
}

/**
 * @kphp-generic TSecond, ...TArgs
 * @param TSecond $second
 * @param TArgs ...$rest
 * @return mixed[]
 */
function constructArrStrange($second, ...$rest) {
    return [null, (string)$second, ...$rest, null];
}

/**
 * @kphp-generic ...TArgs
 * @param TArgs ...$args
 */
function callF2optoptWithArgsAppended(...$args) {
    f2optopt(...[...$args, 999]);
}

/**
 * @kphp-generic ...TArgs
 * @param TArgs ...$args
 */
function callF2optoptWithArgsPrepended(...$args) {
    f2optopt(...[997, ...$args]);
    f2optopt(998, ...$args);
    f2optopt(999, ...[...[...$args]]);
}


callF2(1, 's1');
callF2(2, 's2');

callF3(3, 's3', 3);
callF3(4, 's4', 4);

demoManualProvide();

printAllAsStrings('once: ', new A, new B);
printAllAsStringsTwice('twice: ', new A, new B);

constructArrStrange(new A, 1, 's');

callF2optoptWithArgsAppended(90, 's90_1');
callF2optoptWithArgsAppended(90, 's90_2', -90);
callF2optoptWithArgsPrepended('s91_1');
callF2optoptWithArgsPrepended('s91_2', 91, 91);
