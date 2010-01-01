@ok no_php
<?php
print "A\n";
#break_file
print "B\n";
#break_file
print "C\n";
#break_file
print "D\n";


//echo $a;
define ('A', 123);
print A;
print "\n";
print defined ('A');
print "\n";

define ('B', 1*2 +3);
print B;
print "\n";


print defined ('B');
print "\n";

?>
