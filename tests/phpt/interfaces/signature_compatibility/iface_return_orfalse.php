@ok
<?php

interface Iface {
  /** @return string|false */
  public function get();
}

class Impl implements Iface {
  /** @return string */
  public function get() { return 'foo'; }
}

$v = new Impl();
var_dump($v->get());
