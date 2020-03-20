@ok
<?php

require_once("polyfills.php");

function get_class_name($c) {
    var_dump(get_class($c));
}

get_class_name(new Classes\A());
get_class_name(new Classes\B());
var_dump(Classes\IDo::class);


