@ok
<?php

interface Node {}

interface CallableNode extends Node {}
interface ConstexprNode extends Node {}

class NodeImpl implements Node {}
class CallableImpl implements CallableNode {}
class IntLiteral implements ConstexprNode {}
class StringLiteral implements ConstexprNode {}

/** @param CallableNode|ConstexprNode $x */
function test1($x) {
  if ($x instanceof CallableImpl) {
    var_dump(get_class($x));
  } else if ($x instanceof IntLiteral) {
    var_dump(get_class($x));
  } else if ($x instanceof StringLiteral) {
    var_dump(get_class($x));
  }

  if ($x instanceof NodeImpl) {
    var_dump(get_class($x));
  }
}

/** @param CallableNode|IntLiteral|StringLiteral $x */
function test2($x) {
  if ($x instanceof CallableImpl) {
    var_dump(get_class($x));
  } else if ($x instanceof IntLiteral) {
    var_dump(get_class($x));
  } else if ($x instanceof StringLiteral) {
    var_dump(get_class($x));
  }
}

/** @param IntLiteral|CallableNode|ConstexprNode $x */
function test3($x) {
  if ($x instanceof CallableImpl) {
    var_dump(get_class($x));
  } else if ($x instanceof IntLiteral) {
    var_dump(get_class($x));
  } else if ($x instanceof StringLiteral) {
    var_dump(get_class($x));
  }
}

/** @param IntLiteral|StringLiteral|CallableImpl $x */
function test4($x) {
  if ($x instanceof CallableImpl) {
    var_dump(get_class($x));
  } else if ($x instanceof IntLiteral) {
    var_dump(get_class($x));
  } else if ($x instanceof StringLiteral) {
    var_dump(get_class($x));
  }
}

test1(new NodeImpl());
test1(new CallableImpl());
test1(new IntLiteral());
test1(new StringLiteral());

test2(new NodeImpl());
test2(new CallableImpl());
test2(new IntLiteral());
test2(new StringLiteral());

test3(new NodeImpl());
test3(new CallableImpl());
test3(new IntLiteral());
test3(new StringLiteral());

test4(new NodeImpl());
test4(new CallableImpl());
test4(new IntLiteral());
test4(new StringLiteral());
