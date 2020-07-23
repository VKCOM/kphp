@ok
<?php

require_once("polyfills.php");

/**
 * @kphp-infer
 * @param \Classes\IDo $c
 */
function get_class_name($c) {
    var_dump(get_class($c));
}

get_class_name(new Classes\A());
get_class_name(new Classes\B());
var_dump(Classes\IDo::class);


