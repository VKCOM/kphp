@ok
<?php

class Expr {}

class UnaryOp extends Expr {
  public $arg = null;

  public function __construct(Expr $arg) {
    $this->arg = $arg;
  }
}
class UnaryMinus extends UnaryOp {}
class UnaryPlus extends UnaryOp {}

class Literal extends Expr {}
class StringLiteral extends Literal { public $string_value = 'foo'; }
class IntLiteral extends Literal { public $int_value = 54; }

/** @param StringLiteral|IntLiteral $x */
function test1($x) {
  if ($x instanceof StringLiteral) {
    var_dump($x->string_value);
  } else if ($x instanceof IntLiteral) {
    var_dump($x->int_value);
  }
}

/** @param StringLiteral|IntLiteral|UnaryOp $x */
function test2($x) {
  if ($x instanceof StringLiteral) {
    var_dump($x->string_value);
  } else if ($x instanceof IntLiteral) {
    var_dump($x->int_value);
  } else if ($x instanceof UnaryOp) {
    var_dump(get_class($x->arg));
  }
}

/** @param StringLiteral|UnaryOp|IntLiteral $x */
function test3($x) {
  if ($x instanceof StringLiteral) {
    var_dump($x->string_value);
  } else if ($x instanceof IntLiteral) {
    var_dump($x->int_value);
  } else if ($x instanceof UnaryOp) {
    var_dump(get_class($x->arg));
  }
}

/** @param UnaryOp|Literal $x */
function test4($x) {
  if ($x instanceof StringLiteral) {
    var_dump($x->string_value);
  } else if ($x instanceof IntLiteral) {
    var_dump($x->int_value);
  } else if ($x instanceof UnaryOp) {
    var_dump(get_class($x->arg));
  }
}

/** @param Literal|UnaryOp $x */
function test5($x) {
  if ($x instanceof StringLiteral) {
    var_dump($x->string_value);
  } else if ($x instanceof IntLiteral) {
    var_dump($x->int_value);
  } else if ($x instanceof UnaryOp) {
    var_dump(get_class($x->arg));
  }
}

/** @param UnaryMinus|Literal|UnaryPlus $x */
function test6($x) {
  if ($x instanceof UnaryPlus) {
    var_dump(['unary plus' => get_class($x->arg)]);
  } else if ($x instanceof StringLiteral) {
    var_dump($x->string_value);
  } else if ($x instanceof IntLiteral) {
    var_dump($x->int_value);
  } else if ($x instanceof UnaryMinus) {
    var_dump(['unary minus' => get_class($x->arg)]);
  }
}

/** @param UnaryMinus|UnaryPlus|StringLiteral $x */
function test7($x) {
  if ($x instanceof UnaryPlus) {
    var_dump(['unary plus' => get_class($x->arg)]);
  } else if ($x instanceof StringLiteral) {
    var_dump($x->string_value);
  } else if ($x instanceof UnaryMinus) {
    var_dump(['unary minus' => get_class($x->arg)]);
  }
}

test1(new StringLiteral());
test1(new IntLiteral());

test2(new UnaryOp(new IntLiteral()));
test2(new StringLiteral());
test2(new IntLiteral());

test3(new UnaryOp(new IntLiteral()));
test3(new StringLiteral());
test3(new IntLiteral());

test4(new UnaryOp(new IntLiteral()));
test4(new StringLiteral());
test4(new IntLiteral());

test5(new UnaryOp(new IntLiteral()));
test5(new StringLiteral());
test5(new IntLiteral());

test6(new UnaryMinus(new IntLiteral()));
test6(new UnaryPlus(new StringLiteral()));
test6(new StringLiteral());
test6(new IntLiteral());

test6(new UnaryMinus(new IntLiteral()));
test6(new UnaryPlus(new UnaryMinus(new IntLiteral())));
test6(new StringLiteral());
