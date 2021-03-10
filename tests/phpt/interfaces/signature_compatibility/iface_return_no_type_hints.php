@ok
<?php

// A sanity test that checks that empty signatures cause no havoc.

interface Iface {
  public function f();
}

class Impl implements Iface {
  public function f() {}
}

/** @var Iface[] $list */
$list = [new Impl()];
