@ok php7_4
KPHP_ERROR_ON_WARNINGS=1
<?php

function nonnull_foo(Foo $foo) {}
function nonnull_bar(Bar $bar) {}

class Foo {
  public Bar $bar;
  public ?Bar $nullable_bar;
  public ?Foo $nullable_foo;

  public function __construct(Foo $foo = null) {
    $this->bar = new Bar();
    $this->nullable_bar = new Bar();
    $this->nullable_foo = $foo;
  }

  public static function static_nonnull_foo(Foo $foo) {}
  public static function static_nonnull_bar(Bar $bar) {}

  public function nonnull_foo(Foo $foo) {}
  public function nonnull_bar(Bar $bar) {}
}

class Bar {}

function test_nonnull(Foo $foo) {
  nonnull_bar($foo->bar);
}

function test_ternary(Foo $foo) {
  nonnull_bar($foo->nullable_bar !== null ? $foo->nullable_bar : new Bar());
  nonnull_bar(isset($foo->nullable_bar) ? $foo->nullable_bar : new Bar());
  nonnull_bar($foo->nullable_bar ? $foo->nullable_bar : new Bar());
  nonnull_bar((bool)$foo->nullable_bar ? $foo->nullable_bar : new Bar());

  nonnull_bar($foo->nullable_bar === null ? new Bar() : $foo->nullable_bar);
  nonnull_bar(!isset($foo->nullable_bar) ? new Bar() : $foo->nullable_bar);
  nonnull_bar(!$foo->nullable_bar ? new Bar() : $foo->nullable_bar);
  nonnull_bar(!(bool)$foo->nullable_bar ? new Bar() : $foo->nullable_bar);
}

function test_isnull(Foo $foo) {
  if (!is_null($foo->nullable_bar)) {
    nonnull_bar($foo->nullable_bar);
    Foo::static_nonnull_bar($foo->nullable_bar);
  }

  if (is_null($foo->nullable_bar)) {
  } else {
    nonnull_bar($foo->nullable_bar);
    Foo::static_nonnull_bar($foo->nullable_bar);
  }
}

function test_redundant(Foo $foo) {
  if ($foo->nullable_bar) {
    if (isset($foo->nullable_bar)) {
      nonnull_bar($foo->nullable_bar);
    }
  }

  if ($foo->nullable_bar) {
    if (isset($foo->nullable_bar)) {
      if ($foo->nullable_bar !== null) {
        nonnull_bar($foo->nullable_bar);
      }
    }
  }
}

function test_implicit(Foo $foo) {
  if ($foo->nullable_bar) {
    nonnull_bar($foo->nullable_bar);
    Foo::static_nonnull_bar($foo->nullable_bar);
  }

  if (!$foo->nullable_bar) {
  } else {
    nonnull_bar($foo->nullable_bar);
    Foo::static_nonnull_bar($foo->nullable_bar);
  }
}

function test_implicit2(Foo $foo) {
  if ($foo->nullable_bar && $foo->nullable_foo) {
    nonnull_foo($foo->nullable_foo);
    nonnull_bar($foo->nullable_bar);
  }

// TODO?
//   if (!$foo->nullable_bar && !$foo->nullable_foo) {
//   } else {
//     nonnull_foo($foo->nullable_foo);
//     nonnull_bar($foo->nullable_bar);
//   }
}

function test_nested_repeated_implicit(Foo $foo) {
  if ($foo->nullable_bar) {
    if ($foo->nullable_bar) {
      nonnull_bar($foo->nullable_bar);
      Foo::static_nonnull_bar($foo->nullable_bar);
    }
    nonnull_bar($foo->nullable_bar);
    Foo::static_nonnull_bar($foo->nullable_bar);
  }
}

function test_nested_repeated_implicit2(Foo $foo) {
  if ($foo->nullable_bar) {
    nonnull_bar($foo->nullable_bar);
  }
  if ($foo->nullable_foo) {
    nonnull_foo($foo->nullable_foo);
  }
  if ($foo->nullable_bar) {
    nonnull_bar($foo->nullable_bar);
  }
}

function test_nested_implicit(Foo $foo) {
  if ($foo->nullable_bar) {
    if ($foo->nullable_foo) {
      nonnull_bar($foo->nullable_bar);
      nonnull_foo($foo->nullable_foo);
    }
    nonnull_bar($foo->nullable_bar);

  }
}

function test_eq3_null(Foo $foo) {
  if ($foo->nullable_bar !== null) {
    nonnull_bar($foo->nullable_bar);
  }

  if ($foo->nullable_bar === null) {
  } else {
    nonnull_bar($foo->nullable_bar);
  }
}

function test_nested_repeated_eq3_null(Foo $foo) {
  if ($foo->nullable_bar !== null) {
    if ($foo->nullable_bar !== null) {
      nonnull_bar($foo->nullable_bar);
      Foo::static_nonnull_bar($foo->nullable_bar);
    }
    nonnull_bar($foo->nullable_bar);
    Foo::static_nonnull_bar($foo->nullable_bar);
  }
}

function test_nested_eq3_null(Foo $foo) {
  if ($foo->nullable_bar !== null) {
    if ($foo->nullable_foo !== null) {
      nonnull_bar($foo->nullable_bar);
      nonnull_foo($foo->nullable_foo);
      Foo::static_nonnull_bar($foo->nullable_bar);
      Foo::static_nonnull_foo($foo->nullable_foo);
      $foo->nonnull_bar($foo->nullable_bar);
      $foo->nonnull_foo($foo->nullable_foo);
    }
    nonnull_bar($foo->nullable_bar);
  }
}

function test_isset(Foo $foo) {
  if (isset($foo->nullable_bar)) {
    nonnull_bar($foo->nullable_bar);
  }

  if (!isset($foo->nullable_bar)) {
  } else {
    nonnull_bar($foo->nullable_bar);
  }
}

function test_isset2(Foo $foo) {
  if (isset($foo->nullable_bar, $foo->nullable_foo)) {
    nonnull_bar($foo->nullable_bar);
    nonnull_foo($foo->nullable_foo);
  }

  if (isset($foo->nullable_bar) && isset($foo->nullable_foo)) {
    nonnull_bar($foo->nullable_bar);
    nonnull_foo($foo->nullable_foo);
  }
}

function test_nested_repeated_isset(Foo $foo) {
  if (isset($foo->nullable_bar)) {
    if (isset($foo->nullable_bar)) {
      nonnull_bar($foo->nullable_bar);
      Foo::static_nonnull_bar($foo->nullable_bar);
    }
    nonnull_bar($foo->nullable_bar);
    Foo::static_nonnull_bar($foo->nullable_bar);
  }
}

function test_nested_isset(Foo $foo) {
  if (isset($foo->nullable_bar)) {
    if (isset($foo->nullable_foo)) {
      nonnull_bar($foo->nullable_bar);
      nonnull_foo($foo->nullable_foo);
      Foo::static_nonnull_bar($foo->nullable_bar);
      Foo::static_nonnull_foo($foo->nullable_foo);
      $foo->nonnull_bar($foo->nullable_bar);
      $foo->nonnull_foo($foo->nullable_foo);
    }
    nonnull_bar($foo->nullable_bar);
  }
}

function test_early_return(Foo $foo) {
  if ($foo->nullable_bar === null) {
    return;
  }
  if ($foo->nullable_foo === null) {
    return;
  }
  nonnull_bar($foo->nullable_bar);
  nonnull_foo($foo->nullable_foo);
}

function test_early_return2(Foo $foo) {
  if ($foo->nullable_bar === null || $foo->nullable_foo === null) {
    return;
  }
  nonnull_bar($foo->nullable_bar);
  nonnull_foo($foo->nullable_foo);
}

$foo = new Foo(new Foo(null));

test_ternary($foo);

test_isnull($foo);

test_redundant($foo);

test_implicit($foo);
test_implicit2($foo);
test_nested_repeated_implicit($foo);
test_nested_repeated_implicit2($foo);
test_nested_implicit($foo);

test_eq3_null($foo);
test_nested_repeated_eq3_null($foo);
test_nested_eq3_null($foo);

test_isset($foo);
test_isset2($foo);
test_nested_repeated_isset($foo);
test_nested_isset($foo);

test_early_return($foo);
test_early_return2($foo);
