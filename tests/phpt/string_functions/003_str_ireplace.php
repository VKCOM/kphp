@ok k2_skip
<?php

$strings = ["hello", "Hello world", "Hello World hello"];
$searches = ["h", "hell", "Hell", "World", "Hello", "hello world"];
$replacements = ["heaven", "Heaven", "John"];

foreach ($strings as $string) {
    foreach ($searches as $search) {
        foreach ($replacements as $replacement) {
            $count = 0;
            var_dump(str_ireplace($search, $replacement, $string, $count));
            var_dump($count);
        }
    }
}

foreach ($strings as $string) {
    foreach ($replacements as $replacement) {
        $count = 0;
        var_dump(str_ireplace($searches, $replacement, $string));
        var_dump($count);
    }
}

foreach ($strings as $string) {
    $count = 0;
    var_dump(str_ireplace($searches, $replacements, $string));
    var_dump($count);
}

$count = 0;
var_dump(str_ireplace($searches, $replacements, $strings));
var_dump($count);

// mixed
$a = 100;

$strings_mixed = [$a ? 1 : "hello", $a ? 1 : "Hello world", $a ? 1 : "Hello World hello"];
$searches_mixed = ["1", $a ? 1 : "hell", $a ? 1 : "Hell", $a ? 1 : "World", $a ? 1 : "Hello", $a ? 1 : "hello world"];
$replacements_mixed = [$a ? 1 : "heaven", $a ? 1 : "Heaven", $a ? 1 : "John"];

foreach ($strings_mixed as $string) {
    foreach ($searches_mixed as $search) {
        foreach ($replacements_mixed as $replacement) {
            $count = 0;
            var_dump(str_ireplace($search, $replacement, $string, $count));
            var_dump($count);
        }
    }
}
