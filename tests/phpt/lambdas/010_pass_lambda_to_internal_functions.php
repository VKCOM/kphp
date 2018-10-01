@ok
<?php

print_r(array_map(function ($n) {
    return $n * $n;
}, [1, 2, 3, 4, 5]));

register_shutdown_function(function () {
});
