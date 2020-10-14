@ok
<?php
interface IFoo {}
interface IBar {}

class FooBar implements IFoo, IBar {}
class BarFoo implements IBar, IFoo {}

function run_bar(IBar $ibar) { var_dump("bar;".get_class($ibar)); }

/**
 * @param $ifoo IFoo
 */
function run_foo($ifoo) { var_dump("foo;".get_class($ifoo)); }

$foo_bar = new FooBar();
run_foo($foo_bar);
run_bar($foo_bar);

$bar_foo = new BarFoo();
run_foo($bar_foo);
run_bar($bar_foo);
