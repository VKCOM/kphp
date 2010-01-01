@ok
<?php
echo "hello
world
\n
end";
echo "TEST: escape characters\n";
echo "fusk\rtest\n";
echo "a\tb\ncd\tef\n";
echo "hmm... \v\n";
#echo "escape character \e hope it can be seen\n";
#echo "form feed \f ---\n";
echo "backslash \\n\n";
echo "dollar sign \$ ---\n";
echo "  \"double-quotes\"\n";
echo "octal notation \1 \12\0122 \8 \78\n";
echo "hex notation \xZ \xA \x0A \xG \xAG\n";

echo "Test simple variables";
$tmp = 1;
$_tmp = 2;
$juices = array ("apple", "orange");

echo "\$tmp\n";
echo "$juices\n";
echo "{$tmp}\n" 
?>
