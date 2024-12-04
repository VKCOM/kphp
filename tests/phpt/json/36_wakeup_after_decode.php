@ok
<?php
require_once 'kphp_tester_include.php';

interface ISome {}
interface IAnother {}

class A implements ISome {
    public ?int $a;

    function __wakeup() {
        if ($this->a === null)
            $this->a = 11;
    }
}

class B extends A {
}

class C extends B implements IAnother {
    public ?int $c;

    function __wakeup() {
        parent::__wakeup();
        $this->c = 12;
    }
}

class Input {
    public int $input_id;
    /** @var C[] */
    public array $body_of_c;
}


function test_wakeup_self() {
    $a = JsonEncoder::decode('{}', A::class);
    var_dump($a->a);
    if ($a->a !== 11)
        throw new Exception("unexpected !=");
    $a = JsonEncoder::decode('{"a": 99}', A::class);
    var_dump($a->a);
}

function test_wakeup_nested() {
    $in = JsonEncoder::decode('{"input_id": 10, "body_of_c": [{}]}', Input::class);
    var_dump($in->body_of_c[0]->a);
    var_dump($in->body_of_c[0]->c);
    if ($in->body_of_c[0]->a !== 11 || $in->body_of_c[0]->c !== 12)
        throw new Exception("unexpected !=");
}


const ROLE_ADMIN = "admin";
const ROLE_STAFF = "staff";

abstract class User {
  /** @kphp-json skip=decode */
  public ?string $role_name = null;

  function __construct() {
    $this->__wakeup();
  }

  abstract function __wakeup();
}

class Admin extends User {
  function __wakeup() {
    $this->role_name = ROLE_ADMIN;
  }
}
class Stuff extends User {
  function __wakeup() {
    $this->role_name = ROLE_STAFF;
  }
}

function test_wakeup_inherit() {
  echo JsonEncoder::encode(new Admin()), "\n";
  echo JsonEncoder::encode(new Stuff()), "\n";

  $json = '{"role_name":"admin"}';
  $admin = JsonEncoder::decode($json, Admin::class);
  $staff = JsonEncoder::decode($json, Stuff::class);

  if ($admin->role_name !== ROLE_ADMIN || $staff->role_name !== ROLE_STAFF)
    throw new Exception("unexpected role_name");
}

test_wakeup_self();
test_wakeup_nested();
test_wakeup_inherit();


interface IHasWakeup {
    function __wakeup();
}

class AHasWakeup implements IHasWakeup {
    public ?int $a = null;

    function __wakeup() {
        if ($this->a === null)
            $this->a = 0;
    }
}

function test_wakeup_from_interface() {
    JsonEncoder::encode(new AHasWakeup);
    $a = JsonEncoder::decode('{}', AHasWakeup::class);
    var_dump(to_array_debug($a));
}

test_wakeup_from_interface();

/**
 * @kphp-json flatten
 */
class FlattenWithWakeup {
    /** @var string[] */
    public array $strings;

    static public int $n_wakeup = 0;

    public function __wakeup() {
        echo get_class($this), ' ', count($this->strings), ' ', $this->strings[0], "\n";
        self::$n_wakeup++;
    }
}

function test_wakeup_flatten() {
    JsonEncoder::decode('["hello", "world"]', FlattenWithWakeup::class);
    if (FlattenWithWakeup::$n_wakeup !== 1)
        throw new Exception("unexpected != 1");
}

test_wakeup_flatten();

class ThisClassHasWakeupButNotDecoded {
    public int $i;

    function __wakeup() {
        $this->i = 0;
    }
}

new ThisClassHasWakeupButNotDecoded;
