<?php

namespace Classes;

class TemplateMagic {
    /** @var int */
    var $magic = 0;

    /**
     * @kphp-template $a
     */
    public function print_magic($a) {
        $s = get_class($a);
        var_dump("$s: {$this->magic}");
        var_dump("given magic: {$a->magic}");
    }
}
