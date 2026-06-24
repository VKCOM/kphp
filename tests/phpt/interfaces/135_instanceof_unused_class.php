@ok
<?php

// Regression test for https://github.com/VKCOM/kphp/issues/68
// A class that appears only inside instanceof (never directly instantiated)
// must compile without crashing with "Can't find header".

class Bar {}
class Foo extends Bar {}

function check_foo(Bar $obj): bool {
  return $obj instanceof Foo;
}

// Foo is never instantiated — only used in instanceof.
// Before the fix, kphp crashed trying to include cl/C@Foo.h
// which was never generated because Foo was never mark_as_used().
$bar = new Bar();
var_dump(check_foo($bar));   // bool(false)

$foo = new Foo();
var_dump(check_foo($foo));   // bool(true)
