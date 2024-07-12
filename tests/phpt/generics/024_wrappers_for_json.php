@ok k2_skip
<?php
require_once 'kphp_tester_include.php';


class A {
    public int $a = 10;
}
class B {
    public string $b;
    function __construct(string $b) { $this->b = $b; }
}

/**
 * @kphp-generic T
 * @param T $obj
 */
function encodeObjToJson($obj, bool $pretty_print = false): string {
    return JsonEncoder::encode($obj, $pretty_print ? JSON_PRETTY_PRINT : 0);
}

/**
 * @kphp-generic T
 * @param class-string<T> $out_classname
 * @return T
 */
function decodeJsonToClass(string $json_str, $out_classname) {
    $obj = JsonEncoder::decode($json_str, $out_classname);
    if (!$obj)
        throw new Exception("json decode error");
    return $obj;
}

/**
 * @kphp-generic T
 * @param T $of_this_class
 * @return T
 */
function decodeJsonToObj(string $json_str, object $of_this_class) {
    return JsonEncoder::decode($json_str, classof($of_this_class));
}


echo encodeObjToJson(new A), "\n";
echo encodeObjToJson(new B('b'), true), "\n";

var_dump(to_array_debug(decodeJsonToClass('{"a":9}', A::class)));
var_dump(decodeJsonToClass(encodeObjToJson(new B('b2')), B::class)->b);

try {
    echo decodeJsonToClass('{', B::class)->b;
} catch (Exception $ex) {
    echo $ex->getMessage(), "\n";
}

echo decodeJsonToObj('{"a":4}', new A)->a, "\n";
echo decodeJsonToObj('{"b":"4"}', new B(''))->b, "\n";

