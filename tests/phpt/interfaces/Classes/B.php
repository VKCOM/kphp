<?php

namespace Classes;

class B implements IDo {
    /**
     * @param int $a
     * @param int $b
     */
    public function do_it($a, $b) { var_dump("B; a=".$a."; b=".$b); }
}

