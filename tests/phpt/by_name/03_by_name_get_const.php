@ok k2_skip
<?php

class HasSomeFields {
    const ONE = 1;
    const INT_DOUBLE_ARR = [[0], [1]];
}

class EmtpyCtor {
    // some consts of HasSomeFields
    const ONE = 2;
    const INT_DOUBLE_ARR = [[1], [2+3*2+self::ONE, self::ONE+HasSomeFields::INT_DOUBLE_ARR[0][0]]];

    function __construct() { }
}

class A extends HasSomeFields {
    public int $value;

    function __construct(int $value) { $this->value = $value; }
}


/**
 * @kphp-generic T
 * @param class-string<T> $c
 */
function printSomeConstants($c) {
    echo $c, '::ONE = ', $c::ONE, ', ARR[1][0] = ', $c::INT_DOUBLE_ARR[1][0], ', c=', count($c::INT_DOUBLE_ARR[1]), "\n";
}


printSomeConstants(EmtpyCtor::class);
printSomeConstants(HasSomeFields::class);
printSomeConstants(A::class);

