@ok
<?php
require_once 'kphp_tester_include.php';

class MyJsonEncoder extends JsonEncoder {
  const visibility_policy = "public";
}

class B {
  public string $b_public;
  protected string $b_protected = '';
  private string $b_private = '';
  /** @kphp-json skip = 0 */
  private string $b_private_unskip;

  function init() {
    $this->b_public = "public";
    $this->b_protected = "protected";
    $this->b_private = "private";
    $this->b_private_unskip = "private unskip";
  }
}

class A extends B {
  public string $a_public;
  private string $a_private = '';
  /** @kphp-json skip = false */
  private string $a_private_unskip;

  function init() {
    parent::init();
    $this->a_public = "public";
    $this->a_private = "private";
    $this->a_private_unskip = "private unskip";
  }
}

function test_visibility_policy_decode() {
  $json_a = '{"a_public":"public","a_private":"private","b_public":"public","b_protected":"protected","b_private":"private"}';
  $json_b = '{"b_public":"public","b_protected":"protected","b_private":"private"}';
  $my_json_a = '{"a_public":"public","b_public":"public"}';
  $my_json_b = '{"b_public":"public"}';

  $dump_json_a = to_array_debug(JsonEncoder::decode($json_a, A::class));
  $dump_json_b = to_array_debug(JsonEncoder::decode($json_b, B::class));
  $dump_my_json_a = to_array_debug(MyJsonEncoder::decode($my_json_a, A::class));
  $dump_my_json_b = to_array_debug(MyJsonEncoder::decode($my_json_b, B::class));

  var_dump($dump_json_a);
  var_dump($dump_json_b);
  var_dump($dump_my_json_a);
  var_dump($dump_my_json_b);
}

function test_visibility_policy() {
  $a = new A;
  $b = new B;
  $a->init();
  $b->init();
  $json_a = JsonEncoder::encode($a);
  $json_b = JsonEncoder::encode($b);
  $my_json_a = MyJsonEncoder::encode($a);
  $my_json_b = MyJsonEncoder::encode($b);

  $dump_json_a = to_array_debug(JsonEncoder::decode($json_a, A::class));
  $dump_json_b = to_array_debug(JsonEncoder::decode($json_b, B::class));
  $dump_my_json_a = to_array_debug(MyJsonEncoder::decode($my_json_a, A::class));
  $dump_my_json_b = to_array_debug(MyJsonEncoder::decode($my_json_b, B::class));

  var_dump($dump_json_a);
  var_dump($dump_json_b);
  var_dump($dump_my_json_a);
  var_dump($dump_my_json_b);
}

/** @kphp-json visibility_policy=public */
class BTag {
  public string $b_public;
  protected string $b_protected = '';
  private string $b_private = '';
  /** @kphp-json skip = false */
  private string $b_private_unskip;

  function init() {
    $this->b_public = "public";
    $this->b_protected = "protected";
    $this->b_private = "private";
    $this->b_private_unskip = "private unskip";
  }
}

class ATag extends BTag {
  public string $a_public;
  private string $a_private = 'ap';

  function init() {
    parent::init();
    $this->a_public = "public";
    $this->a_private = "private";
  }
}

interface IWillBePrivate {}

class WithPrivateI {
    public int $response_id = 0;
    private ?IWillBePrivate $i = null;
    /** @var tuple(int) */
    private $tup;
}

function test_visibility_policy_with_tag() {
  $a = new ATag;
  $b = new BTag;
  $a->init();
  $b->init();
  $json_a = JsonEncoder::encode($a);
  $json_b = JsonEncoder::encode($b);
  $my_json_a = MyJsonEncoder::encode($a);
  $my_json_b = MyJsonEncoder::encode($b);

  $dump_json_a = to_array_debug(JsonEncoder::decode($json_a, ATag::class));
  $dump_json_b = to_array_debug(JsonEncoder::decode($json_b, BTag::class));
  $dump_my_json_a = to_array_debug(MyJsonEncoder::decode($my_json_a, ATag::class));
  $dump_my_json_b = to_array_debug(MyJsonEncoder::decode($my_json_b, BTag::class));

  var_dump($dump_json_a);
  var_dump($dump_json_b);
  var_dump($dump_my_json_a);
  var_dump($dump_my_json_b);
}

function test_ok_interface_auto_skipped() {
    $obj = MyJsonEncoder::decode('{"response_id":10}', WithPrivateI::class);
    echo $obj->response_id, "\n";
}


test_visibility_policy_decode();
test_visibility_policy();
test_visibility_policy_with_tag();
test_ok_interface_auto_skipped();
