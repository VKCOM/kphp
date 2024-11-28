@ok
<?php
require_once 'kphp_tester_include.php';

interface IValue {
}

class IntValue implements IValue {
    public int $value;

    function __construct(int $v) { $this->value = $v; }
}

class StringValue implements IValue {
    /** @kphp-json rename=v2 */
    public string $value;

    function __construct(string $v) { $this->value = $v; }
}

class RawStringValue implements IValue {
    /** @kphp-json raw_string */
    public string $data;

    function __construct(string $v) { $this->data = $v; }
}

class ComplexValue implements IValue {
    /** @kphp-json float_precision = 2 */
    public float $i;
    /** @kphp-json rename=v2 */
    public float $v;

    function __construct(float $i, float $v) { $this->i = $i; $this->v = $v; }
}

class ComplexValue2 extends ComplexValue {
    /** @kphp-json rename = v4 */
    public string $v2 = 'v2';
}


class Response {
    public int $id;
    public ?IValue $data = null;
}

function makeAndEncodeResponse(?IValue $data) {
    static $next_id = 0;
    $r = new Response;
    $r->id = ++$next_id;
    $r->data = $data;

    $json = JsonEncoder::encode($r);
    echo ($data === null ? "null" : get_class($data)), ' ', $json, "\n";
    if (JsonEncoder::getLastError()) {
        echo "err encode: ", JsonEncoder::getLastError(), "\n";
    }
}

// encode() works, but decode() would trigger a compilation error
makeAndEncodeResponse(new IntValue(10));
makeAndEncodeResponse(new StringValue('hello'));
makeAndEncodeResponse(new RawStringValue("[1,2,3]"));
makeAndEncodeResponse(new RawStringValue(JsonEncoder::encode(new IntValue(-9))));
makeAndEncodeResponse(new ComplexValue(1.23456, 6.54321));
makeAndEncodeResponse(new ComplexValue2(1.23456, 6.54321));

class C11 { public int $c11 = 11; }
class C12 extends C11 { public int $c12 = 12; }
class C13 extends C12 { public int $c13 = 13; }

class D11 { public int $d11 = 11; }
class D12 extends D11 { public int $d12 = 12; }
class D13 extends D12 { public int $d13 = 13; }

class E11 { public int $e11 = 11; }
class E12 extends E11 { public int $e12 = 12; }
class E13 extends E12 { public int $e13 = 13; }

echo JsonEncoder::encode(new C11), "\n";
echo JsonEncoder::encode(new C12), "\n";
echo JsonEncoder::encode(new C13), "\n";
echo JsonEncoder::encode(new D12), "\n";
echo JsonEncoder::encode(new D13), "\n";
echo JsonEncoder::encode(new E13), "\n";

class F11 { public int $f11 = 11; }
class F12 extends F11 { public int $f12 = 12; }
class F13 extends F12 { public int $f13 = 13; }

$j13 = JsonEncoder::encode(new F13);
echo $j13, "\n";
$f13 = JsonEncoder::decode($j13, F13::class);
var_dump(to_array_debug($f13));


class Input {
    public ?IntValue $int_v = null;
    public ?RawStringValue $raw_s = null;
}

$in1 = new Input;
var_dump(to_array_debug(JsonEncoder::decode(JsonEncoder::encode($in1), Input::class)));
$in1->int_v = new IntValue(80);
var_dump(to_array_debug(JsonEncoder::decode(JsonEncoder::encode($in1), Input::class)));
$in1->raw_s = new RawStringValue('[1,2,"3"]');
var_dump(to_array_debug(JsonEncoder::decode(JsonEncoder::encode($in1), Input::class)));


interface IValue2 {
}

abstract class Value2 implements IValue2 {
    public function toJson(): ?string {
        return JsonEncoder::encode($this);
    }

    public function debug(): void {
        var_dump(to_array_debug($this));
    }

    /** @return ?static */
    static public function fromJson(string $json) {
        return JsonEncoder::decode($json, static::class);
    }
}

class S1 extends Value2 {
    public int $s1 = 1;
}
class S2 extends Value2 {
    public int $s2 = 2;
}

echo (new S1)->toJson(), "\n";
echo (new S2)->toJson(), "\n";
S1::fromJson('{"s1":11}')->debug();
S2::fromJson('{"s2":12}')->debug();
