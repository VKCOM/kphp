@ok
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
        "highload no-highload" => "Calling no-highload function from highload function",
    ],
  ];
}

function withoutColor(int $a) {
    if ($a) {
        withoutColorToo($a);
    }
}

function withoutColorToo(int $a) {
    if ($a) {
        withoutColor($a);
    }
}

withoutColor((int)$_POST["id"]);

/**
 * @kphp-color highload
 */
function highload(int $a) {
    if ($a) {
        highloadToo($a);
    }

    echo 1;
}

/**
 * @kphp-color highload
 */
function highloadToo(int $a) {
    if ($a) {
        highload($a);
    }
}
