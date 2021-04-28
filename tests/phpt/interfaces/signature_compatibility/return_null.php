@ok
<?php

// TODO: this code should probably fail;
// If we move types info from phpdoc to typehints,
// PHP will give a fatal error.

class Foo {}

interface Iface {
  /** @return Foo */
  public function get();
}

class Impl implements Iface {
  /** @return null */
  public function get() { return null; }
}


