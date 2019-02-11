<?php

namespace Classes;

class B implements IDo {
    public function do_it($a, $b) { var_dump("B; a=".$a."; b=".$b); }
}

