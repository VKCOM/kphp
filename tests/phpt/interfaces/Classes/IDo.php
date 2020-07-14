<?php

namespace Classes;

interface IDo {
    /**
     * @kphp-infer
     * @param int $a
     * @param int $b
     */
    public function do_it($a, $b);
}
