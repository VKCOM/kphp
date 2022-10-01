@kphp_should_fail
/Variadic generic can be used only for a function with \.\.\.\$variadic parameter/
<?php

/**
 * @kphp-generic ...TArgs
 */
function vg($args) {
    var_dump($args);
}

vg([1]);
