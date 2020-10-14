@ok
<?php

/**
 * @return string[]
 */
function h() {
    return array_filter(explode('|', "1|2|3|4|5|7"), function ($x) { return (bool)($x & 1); });
}

var_dump(h());
