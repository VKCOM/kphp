@ok
<?php

class A { function af() { echo "A f\n"; } }

class ValueInt {
    public int $value = 0;
    function getValue() { return $this->value; }
}
class ValueString {
    public string $value = '';
    function getValue() { return $this->value; }
}
class ValueA {
    public A $value;
    function __construct() { $this->value = new A; }
    function getValue(): A { return $this->value; }
}

/**
 * @kphp-generic T
 * @param T $obj
 * @return T::value
 */
function getValue1($obj) {
    return $obj->value;
}

/**
 * @kphp-generic T
 * @param T $obj
 * @return T::getValue()
 */
function getValue2($obj) {
    return $obj->getValue();
}

/**
 * @kphp-generic T
 * @param T $obj
 * @param T::value $cmp_with
 */
function isValueEqual($obj, $cmp_with): bool {
    $value = $obj->value;
    return $value === $cmp_with;
}

echo getValue1(new ValueInt), "\n";
getValue1(new ValueA)->af();

echo getValue2(new ValueString), "\n";
getValue2(new ValueA)->af();

var_dump(isValueEqual(new ValueInt, 1));
var_dump(isValueEqual(new ValueInt, 0));

