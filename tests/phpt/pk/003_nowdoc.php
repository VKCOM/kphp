@ok
<?php

$v = 123;
$s = <<<    ID
S'o'me "\"t e\txt; \$v = $v"
Some more text
ID;
echo ">$s<\n\n";

$s = <<<    'ID'
S'o'me "\"t e\txt; \$v = $v"
Some more text
ID;
echo ">$s<\n\n";

$x = <<<'DOC'
DOC;

$y = <<<DOC
DOC;

echo "\$x = $x, \$y = $y\n";
