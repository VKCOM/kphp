@kphp_should_fail
/Field WithTuple::\$tupe is @var tuple\(int, string\), it's incompatible with json/
/Field WithShape::\$shape is @var shape\(a:int, b:string\), it's incompatible with json/
/Field WithBuiltin::\$ex is @var \?ArgumentCountError, it's incompatible with json/
/Field WithBuiltin::\$conn is @var RpcConnection, it's incompatible with json/
<?php

class WithTuple {
  /** @var tuple(int, string) */
  public $tupe;
}

class WithShape {
  /** @var shape(a:int, b:string) */
  public $shape;
}

class WithBuiltin {
  /** @var ?ArgumentCountError */
  public $ex;
  /** @var RpcConnection */
  public $conn;
}
class WithBuiltinDerived extends WithBuiltin {
}


function test_tuples_forbidden() {
  JsonEncoder::encode(new WithTuple);
}

function test_shapes_forbidden() {
  JsonEncoder::encode(new WithShape);
}

function test_builtin_forbidden() {
  JsonEncoder::decode('', WithBuiltinDerived::class);
}

test_tuples_forbidden();
test_shapes_forbidden();
test_builtin_forbidden();
