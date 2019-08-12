@ok
<?php

require_once("Classes/autoload.php");

class A implements Classes\IDo {
    public function do_it($a, $b) {
        var_dump("A");
    }
}

class B {
    public static function run() {
        $a = new A();
        $a->do_it(1, 2);
    }
}

B::run();
