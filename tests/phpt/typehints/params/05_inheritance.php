@ok
<?php
class Base {
  public function add(self $base) {
    return $base;
  }

  public function test() {
    echo "Base\n";
  }
}

class Derived extends Base {
  public function test() {
    echo "Derived\n";
  }
}

class Third extends Derived {
}

$d = new Derived();
echo get_class($d->add($d));
echo get_class($d->add(new Third()));
