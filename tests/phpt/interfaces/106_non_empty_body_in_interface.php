@kphp_should_fail
<?php
interface IDo {
    public function do_it($a, $b) {
      echo "world!\n";
    }
}

class DoImpl implements IDo {
    public function do_it($a, $b) {
      echo "world!\n";
    }
}

$x = new DoImpl;
$x->do_it(1, 2);
