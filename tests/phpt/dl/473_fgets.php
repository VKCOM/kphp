@ok
<?php

function test_fgets() {
    $stream = fopen(__DIR__ . '/fgets.txt', 'r');

    var_dump (fgets ($stream)); // line = "123\n"

    var_dump (fgets ($stream, 100)); // `length` greater than length of the line. line = "gucci\n"

    var_dump (fgets ($stream)); // line = "\n"

    var_dump (fgets ($stream, 4)); // `length` less than length of the line. line = "php < kphp\n"
    var_dump (fgets ($stream, 1)); // always false
    var_dump (fgets ($stream)); // rest of the line

    var_dump (fgets ($stream)); // last line. line = "bang"

    fclose($stream);
}

function test_fgets_edge_cases() {
    $stream = fopen(__DIR__ . '/fgets.txt', 'r');

    var_dump (fgets ($stream, 4)); // `length` = line.len() (without '\n'). line = "123\n"
    var_dump (fgets ($stream)); // rest of the line

    var_dump (fgets ($stream, 7)); // `length` = line.len() + 1 (with '\n'). line = "gucci\n"

    // skip all lines except the last one
    var_dump (fgets ($stream));
    var_dump (fgets ($stream));

    var_dump (fgets ($stream, 4)); // last line with `length` = line.len(). line = "bang"

    fclose($stream);
}

function test_fgets_edge_case2() {
    $stream = fopen(__DIR__ . '/fgets.txt', 'r');

    // skip all lines except the last one
    var_dump (fgets ($stream));
    var_dump (fgets ($stream));
    var_dump (fgets ($stream));
    var_dump (fgets ($stream));

    var_dump (fgets ($stream, 5)); // last line with `length` = line.len() + 1. line = "bang"

    fclose($stream);
}

test_fgets();
test_fgets_edge_cases();
test_fgets_edge_case2();