@ok
<?php

function init() {
    $stream = fopen(__DIR__.'/fgets.txt', 'w');
    fwrite ($stream, "123\n");
    fwrite ($stream, "gucci\n");
    fwrite ($stream, "\n");
    fwrite ($stream, "php < kphp\n");
    fwrite ($stream, "bang");
    fclose ($stream);
}

function test_fgets() {
    $stream = fopen(__DIR__ . '/fgets.txt', 'r');

    var_dump (fgets ($stream)); // line = "123\n"

    var_dump (fgets ($stream, 100)); // `length` greater than length of the line. line = "gucci\n"

    var_dump (fgets ($stream)); // line = "\n"

    var_dump (fgets ($stream, 4)); // `length` less than length of the line. line = "php < kphp\n"
//     var_dump (fgets ($stream, 1));   kphp expects "", php expects false
    var_dump (fgets ($stream)); // rest of the line

    var_dump (fgets ($stream)); // last line. line = "bang"

    var_dump (fgets ($stream)); // eof
    var_dump (fgets ($stream)); // eof

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

function test_fgets_mixed() {
    $stream = fopen(__DIR__ . '/fgets.txt', 'r+');

    var_dump (fgets ($stream)); // 123\n
    var_dump (fread ($stream, 3)); // guc
    var_dump (fgets ($stream)); // ci\n
    var_dump (fwrite ($stream, "new string")); // \nphp < kphp\n -> new stringp\n
    var_dump (fgets ($stream)); // p\n
    var_dump (fread ($stream, 2)); // ba
    var_dump (fwrite ($stream, "new string 2")); // ng -> new string 2
    var_dump (fgets ($stream)); // eof

    fclose($stream);
}

init();
test_fgets();
test_fgets_edge_cases();
test_fgets_edge_case2();
test_fgets_mixed();
