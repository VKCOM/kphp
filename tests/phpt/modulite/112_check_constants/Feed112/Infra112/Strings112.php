<?php

namespace Feed112\Infra112;

define('DEF_STRINGS_1', 1);
define('DEF_STRINGS_2', 2);

class Strings112 {
    const STR_NAME = 'name';
    const STR_HIDDEN = 'hidden';

    static function demo() {
        echo self::STR_NAME, ' ', self::STR_HIDDEN, ' ', Hidden112::HIDDEN_1, Hidden112::HIDDEN_2;
        Hidden112::demo();
    }
}
