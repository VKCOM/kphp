@ok
<?php
require_once 'kphp_tester_include.php';


class C {
  /**
   * @kphp-json rename = c_foo
   * @kphp-json skip = false
   */
  public string $c_foo;
}

class B extends C {
  /** @kphp-json skip */
  public string $b_foo = '';
}

class A extends B {
  public string $a_foo;
  /** @kphp-json skip = true */
  public string $a_bar = '';

  function init() {
    $this->a_foo = "a_foo";
    $this->a_bar = "a_bar";
    $this->b_foo = "b_foo";
    $this->c_foo = "c_foo";
  }
}

class AllSkipped {
  /** @kphp-json skip */
  public string $a = '';
  /** @kphp-json skip */
  public string $b = '';

  function init() {
    $this->a = "a";
    $this->b = "b";
  }
}

#ifndef KPHP
class RpcConnection {}
#endif

class Response1 {
  /** @kphp-json skip = encode */
  public array $ids = [1];
  /** @kphp-json skip = decode */
  public array $numValues = [1,2,3];
  /** @kphp-json skip */
  public ?\RpcConnection $conn = null;
}


class UserFull {
  public string $name;
  public string $login;
  public int $age;
  /** @kphp-json skip=encode */
  public string $password;

  public function __construct(string $name, string $login, int $age, string $password) {
    $this->name = $name;
    $this->login = $login;
    $this->age = $age;
    $this->password = $password;
  }
}


const ROLE_ADMIN = "admin";
const ROLE_STAFF = "staff";

abstract class User {
  /** @kphp-json skip=decode */
  public ?string $role_name = null;
}

class Admin extends User {
  public function __construct() {
    $this->role_name = ROLE_ADMIN;
  }
}
class Stuff extends User {
  public function __construct() {
    $this->role_name = ROLE_STAFF;
  }
}


function test_skip_field() {
  $obj = new A;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "A");
  var_dump(to_array_debug($restored_obj));
}

function test_skip_all_fields() {
  $obj = new AllSkipped;
  $obj->init();
  $json = JsonEncoder::encode($obj);
  $restored_obj = JsonEncoder::decode($json, "AllSkipped");
  var_dump(to_array_debug($restored_obj));
}

function test_skip_encode_decode() {
  echo JsonEncoder::encode(new Response1), "\n";

  $json = '{"ids":[-900], "numValues":[-900]}';
  $r = JsonEncoder::decode($json, Response1::class);
  var_dump(to_array_debug($r));
}

function test_skip_encode() {
  $json = JsonEncoder::encode(new UserFull("Alex", "alex_super", 25, "qwerty"));
  echo $json, "\n";
  var_dump(strpos($json, 'password'));
}

function test_skip_decode_inherit() {
  echo JsonEncoder::encode(new Admin()), "\n";
  echo JsonEncoder::encode(new Stuff()), "\n";

  $json = '{"role_name":"admin"}';
  $admin = JsonEncoder::decode($json, Admin::class);
  $staff = JsonEncoder::decode($json, Stuff::class);

  var_dump($admin->role_name === null); // true (__construct is not called on deserialization)
  var_dump($staff->role_name === null); // true (__construct is not called on deserialization)
}

test_skip_field();
test_skip_all_fields();
test_skip_encode_decode();
test_skip_encode();
test_skip_decode_inherit();
