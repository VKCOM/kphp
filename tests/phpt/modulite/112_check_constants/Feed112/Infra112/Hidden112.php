<?php

namespace Feed112\Infra112;

class Hidden112 {
    const HIDDEN_1 = 1;
    const HIDDEN_2 = 2;

    static function demo() {
        (function() {
        echo self::HIDDEN_2, "\n";
        echo DEF_POST_1, "\n";
        })();
    }
}
