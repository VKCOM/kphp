<?php

namespace Feed112;

define('DEF_POST_1', 1);
define('DEF_POST_2', 2);
define('DEF_POST_PUB', 3);

class Post112 {
    const ONE = 1;
    const TWO = 2;
    const THREE = 3;

    static function demo() {
        echo self::ONE, self::TWO, "\n";
        echo Infra112\Strings112::STR_NAME, "\n";
        echo Infra112\Strings112::STR_HIDDEN, "\n";
        echo Infra112\Hidden112::HIDDEN_1, "\n";

        echo DEF_POST_1, DEF_STRINGS_1, DEF_STRINGS_2, "\n";
    }
}
