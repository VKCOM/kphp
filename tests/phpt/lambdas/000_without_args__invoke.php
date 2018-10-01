@ok
<?php

$fun = function () {
    return 10;
};
var_dump($fun->__invoke());
