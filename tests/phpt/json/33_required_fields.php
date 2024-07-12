@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

// 1) @kphp-json required
class A2 {
    /** @kphp-json required */
    public float $foo = 0.0;
    /** @kphp-json required */
    public ?string $bar = null;
}

// 2) non-nullable fields are auto required
class A {}

class NonNullableField1 {
    public string $s;
}
class NonNullableField2 {
    public A $a;
}
class NonNullableField3 {
    /** @var int[]|false */
    public $arr;
}

class NullableField1 {
    public ?string $s;
}
class NullableField2 {
    /** @var int|true|false|null */
    public $smth;
}
class NullableField3 {
    /** @var ?A */
    public $a;
}

class ExplicitNonRequired {
    /** @kphp-json required = false */
    public string $role;

    function __construct() {
        $this->role = 'in ctor';
    }
}


function test_nulls_fail() {
    $obj = JsonEncoder::decode('{"foo":null, "bar":null}', A2::class);
    var_dump(JsonEncoder::getLastError());
}

function test_non_nulls_ok() {
    $obj = JsonEncoder::decode('{"foo":12.34, "bar":"baz"}', A2::class);
    var_dump(to_array_debug($obj));
}

function test_absent_values_fails() {
    $obj = JsonEncoder::decode('{"foo":12.34}', A2::class);
    var_dump(JsonEncoder::getLastError());

    $obj = JsonEncoder::decode('{"bar":"baz"}', A2::class);
    var_dump(JsonEncoder::getLastError());
}

function test_non_nullable_auto_required() {
    $f1 = JsonEncoder::decode('{}', NonNullableField1::class);
    echo JsonEncoder::getLastError(), "\n";
    $f2 = JsonEncoder::decode('{}', NonNullableField2::class);
    echo JsonEncoder::getLastError(), "\n";
    $f3 = JsonEncoder::decode('{}', NonNullableField3::class);
    echo JsonEncoder::getLastError(), "\n";

    $n1 = JsonEncoder::decode('{}', NullableField1::class);
    echo JsonEncoder::getLastError(), "\n";
    var_dump(to_array_debug($n1));
    $n2 = JsonEncoder::decode('{}', NullableField2::class);
    echo JsonEncoder::getLastError(), "\n";
    var_dump(to_array_debug($n2));
    $n3 = JsonEncoder::decode('{}', NullableField3::class);
    echo JsonEncoder::getLastError(), "\n";
    var_dump(to_array_debug($n3));
}

function test_explicit_non_required() {
    $restored_obj = JsonEncoder::decode('{}', ExplicitNonRequired::class);
    if (!$restored_obj)
        throw new Exception("unexpected !restored_obj");
    $restored_obj->role = 'after decoded';
    var_dump(to_array_debug($restored_obj));
}

test_nulls_fail();
test_non_nulls_ok();
test_absent_values_fails();
test_non_nullable_auto_required();
test_explicit_non_required();
