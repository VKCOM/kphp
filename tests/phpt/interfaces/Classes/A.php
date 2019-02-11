<?php

namespace Classes;

class A implements IDo {
    public function do_it($a, $b) { var_dump("A; a=".$a."; b=".$b); }
}
