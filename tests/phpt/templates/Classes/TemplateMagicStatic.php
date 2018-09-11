<?php

namespace Classes;

class TemplateMagicStatic {
    /**
     * @kphp-template $a
     */
    public static function print_magic($a) {
        var_dump("given magic: {$a->magic}");
    }
}
