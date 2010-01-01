@ok
<?php
echo "\0";
echo "hello\0tests\n";
$a = "hello\n";
echo "$a\0$a";
$a = "hello\0tests\n";
echo $a;

#echo "A|\x00A|B"
