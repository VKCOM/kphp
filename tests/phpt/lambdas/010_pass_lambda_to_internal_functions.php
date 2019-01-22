@ok
<?php

print_r(array_map(function ($n, $z = 10) {
    return $n * $n;
}, [1, 2, 3, 4, 5]));

register_shutdown_function(function () {
});
