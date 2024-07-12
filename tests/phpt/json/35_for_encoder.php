@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class MyJsonEncoder extends \JsonEncoder {
  const visibility_policy = 'public';
}
class MyJsonEncoder2 extends \JsonEncoder {
  const rename_policy = 'camelCase';
}


class AdminEncoder extends JsonEncoder {
}

class GuestEncoder extends JsonEncoder {
}

class User {
  /**
   * @kphp-json skip = true
   * @kphp-json for AdminEncoder skip=false
   * @kphp-json for AdminEncoder rename=super_aa
   */
  public string $aa = 'aa';

  /**
   * @kphp-json for AdminEncoder skip
   * @kphp-json for GuestEncoder rename=super_bb
   */
  public string $bb = 'bb';
}

class HasRenameSpec {
    /**
     * @kphp-json rename = id2
     * @kphp-json for MyJsonEncoder rename = my_id
     * @kphp-json for MyJsonEncoder skip_if_default = 0
     * @kphp-json for MyJsonEncoder raw_string = false
     */
    public int $id = 0;

    function __construct(int $id) { $this->id = $id; }
}

class HasSkipSpec {
    /**
     * @kphp-json skip
     * @kphp-json for MyJsonEncoder skip = false
     */
    public int $response_id = 0;
    /**
     * @kphp-json for MyJsonEncoder2 skip = false
     */
    private int $priv = 0;

    function __construct(int $response_id, int $priv) {
        $this->response_id = $response_id;
        $this->priv = $priv;
    }
}

/**
 * @kphp-json fields = $id, $name
 * @kphp-json for MyJsonEncoder fields = $id
 * @kphp-json for MyJsonEncoder2 fields = $id, $name, $password
 */
class HasFieldsSpec {
    public int $id;
    public string $name;
    /** @kphp-json rename = pwd */
    private string $password = 'pwd';

    function __construct(int $id) {
        $this->id = $id;
        $this->name = (string)$id;
    }
}

/**
 * @kphp-json fields = f2, f1
 * @kphp-json for MyJsonEncoder2 fields = f3, f2, f1
 * @kphp-json for MyJsonEncoder fields = f3, f1, f2
 */
class ForTestFieldsOrder {
    public int $f1 = 1;
    public int $f2 = 2;
    public int $f3 = 3;
}

function test_has_rename_spec() {
    echo __FUNCTION__, "\n";;

    $json = JsonEncoder::encode(new HasRenameSpec(1));
    echo $json, "\n";
    if ($json !== '{"id2":1}')
        throw new Exception("unexpected !=");
    $back = JsonEncoder::decode($json, HasRenameSpec::class);
    var_dump(to_array_debug($back));
    if ($back->id !== 1)
        throw new Exception("unexpected back");

    $json = MyJsonEncoder::encode(new HasRenameSpec(2));
    echo $json, "\n";
    if ($json !== '{"my_id":2}')
        throw new Exception("unexpected !=");
    $back = MyJsonEncoder::decode($json, HasRenameSpec::class);
    var_dump(to_array_debug($back));

    $json = MyJsonEncoder2::encode(new HasRenameSpec(3));
    echo $json, "\n";
    if ($json !== '{"id2":3}')
        throw new Exception("unexpected !=");
    $back = MyJsonEncoder2::decode($json, HasRenameSpec::class);
    var_dump(to_array_debug($back));
}

function test_has_skip_spec() {
    echo __FUNCTION__, "\n";;

    $json = JsonEncoder::encode(new HasSkipSpec(1, 1));
    echo $json, "\n";
    if ($json !== '{"priv":1}')
        throw new Exception("unexpected !=");
    $back = JsonEncoder::decode($json, HasSkipSpec::class);
    var_dump(to_array_debug($back));

    $json = MyJsonEncoder::encode(new HasSkipSpec(2, 2));
    echo $json, "\n";
    if ($json !== '{"response_id":2}')
        throw new Exception("unexpected !=");
    $back = MyJsonEncoder::decode($json, HasSkipSpec::class);
    var_dump(to_array_debug($back));

    $json = MyJsonEncoder2::encode(new HasSkipSpec(3, 3));
    echo $json, "\n";
    if ($json !== '{"priv":3}')
        throw new Exception("unexpected !=");
    $back = MyJsonEncoder2::decode($json, HasSkipSpec::class);
    var_dump(to_array_debug($back));
}

function test_has_fields_spec() {
    echo __FUNCTION__, "\n";;

    $json = JsonEncoder::encode(new HasFieldsSpec(1));
    echo $json, "\n";
    if ($json !== '{"id":1,"name":"1"}')
        throw new Exception("unexpected !=");

    $json = MyJsonEncoder::encode(new HasFieldsSpec(2));
    echo $json, "\n";
    if ($json !== '{"id":2}')
        throw new Exception("unexpected !=");

    $json = MyJsonEncoder2::encode(new HasFieldsSpec(3));
    echo $json, "\n";
    if ($json !== '{"id":3,"name":"3","pwd":"pwd"}')
        throw new Exception("unexpected !=");
}

function test_has_fields_spec_order() {
    $j1 = JsonEncoder::encode(new ForTestFieldsOrder);
    if ($j1 !== '{"f2":2,"f1":1}') throw new Exception("unexpected j1: " . $j1);
    $j2 = MyJsonEncoder::encode(new ForTestFieldsOrder);
    if ($j2 !== '{"f3":3,"f1":1,"f2":2}') throw new Exception("unexpected j2: " . $j2);
    $j3 = MyJsonEncoder2::encode(new ForTestFieldsOrder);
    if ($j3 !== '{"f3":3,"f2":2,"f1":1}') throw new Exception("unexpected j3: " . $j3);
}

function test_skips_for_encoder() {
    var_dump(JsonEncoder::encode(new User()));
    echo "\n";
    var_dump(AdminEncoder::encode(new User()));
    echo "\n";
    var_dump(GuestEncoder::encode(new User()));
    echo JsonEncoder::getLastError(), "\n";
}

test_has_rename_spec();
test_has_skip_spec();
test_has_fields_spec();
test_has_fields_spec_order();
test_skips_for_encoder();
