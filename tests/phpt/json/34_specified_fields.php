@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class MyJsonEncoder extends JsonEncoder {
    const visibility_policy = 'public';
}

/**
 * @kphp-json fields = $id, $name
 */
class User1 {
    public int $id = 0;
    public string $name = 'n';
    public int $friends_count = 0;
    public string $other = 'other';
}

/** @kphp-json fields = $b1 */
class Base {
    public string $b1 = 'b1';
    public string $b2 = 'b2';
}
/** @kphp-json fields = $d11 */
class Derived1 extends Base {
    public ?string $d11 = 'd11';
    public ?string $d12 = 'd12';
}
/** @kphp-json fields = $d21 */
class Derived2 extends Base {
    public ?string $d21 = 'd21';
    public ?string $d22 = 'd22';
}

/**
 * @kphp-json fields = $id
 */
class WithDef {
    /** @kphp-json skip_if_default */
    public int $id = 0;
    public int $another = 2;
}

/**
 * KPHP also allows not to write $ sign
 * @kphp-json fields =   public1  ,   private1
 */
class WithPrivateFields {
    public int $public1 = 0;
    protected int $protected1 = 0;
    private int $private1 = 0;
}

/**
 * @kphp-json rename_policy = snake_case
 * @kphp-json fields = $responseId, $anotherValue
 */
class WithRenames {
    public int $responseId = 0;
    public string $someVar = 's';
    /** @kphp-json rename = av */
    public int $anotherValue = -10;
}


interface KV {}

/** @kphp-json fields = $key, $value */
class KV_orderKV implements KV {
    public string $key = 'k';
    public string $value = 'v';
}
/** @kphp-json fields = $value, $key */
class KV_orderVK implements KV {
    public string $key = 'k';
    public string $value = 'v';
}

class RespKV {
    /** @var KV[] */
    public $kv_items;
}


function test_specific_fields_simple() {
    $json = JsonEncoder::encode(new User1);
    echo $json, "\n";

    $json = '{"id":10, "friends_count":100}';
    $user = JsonEncoder::decode($json, User1::class);
    if ($user->friends_count) throw new Exception("unexpected friends_count");
    var_dump(to_array_debug($user));
}

function test_specific_fields_inherit() {
    $encode_base = function(Base $b): string {
        $json = JsonEncoder::encode($b);
        echo get_class($b), ': ', $json, "\n";
        return $json;
    };
    $json_base = $encode_base(new Base);
    $json_d1 = $encode_base(new Derived1);
    $json_d2 = $encode_base(new Derived2);
}

function test_specific_fields_and_skip_if_default() {
    $o = new WithDef;
    echo JsonEncoder::encode($o), "\n";
    $o->id = 12;
    echo JsonEncoder::encode($o), "\n";
}

function test_specific_fields_and_visibility_policy() {
    echo JsonEncoder::encode(new WithPrivateFields), "\n";
    if (JsonEncoder::encode(new WithPrivateFields) !== MyJsonEncoder::encode(new WithPrivateFields))
        throw new Exception("unexpected !==");
}

function test_specific_fields_and_rename_policy() {
    $wr = new WithRenames;
    $wr->someVar = 'changed';
    $json = JsonEncoder::encode($wr);
    echo $json, "\n";
    $restored_obj = JsonEncoder::decode($json, WithRenames::class);
    var_dump(to_array_debug($restored_obj));
    if ($restored_obj->someVar === 'changed')
        throw new Exception("unexpected changed");
}

function test_specific_fields_order() {
    $resp = new RespKV;
    $resp->kv_items[] = new KV_orderKV();
    $resp->kv_items[] = new KV_orderVK();
    $resp->kv_items[] = new KV_orderKV();
    $json = JsonEncoder::encode($resp);
    echo $json, "\n";
    if ($json !== '{"kv_items":[{"key":"k","value":"v"},{"value":"v","key":"k"},{"key":"k","value":"v"}]}')
        throw new Exception("unexpected !=");
}


test_specific_fields_simple();
test_specific_fields_inherit();
test_specific_fields_and_skip_if_default();
test_specific_fields_and_visibility_policy();
test_specific_fields_and_rename_policy();
test_specific_fields_order();
