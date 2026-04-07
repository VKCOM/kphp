@ok
This test is copy of ext/standard/tests/strings/fprintf_variation_009.phpt of php-src repo
--TEST--
Test fprintf() function (variation - 9)
--FILE--
<?php

$string_variation = array( "%5s", "%-5s", "%05s", "%'#5s" );
$strings = array( NULL, "abc", 'aaa' );

if (!($fp = fopen('php://stdout', 'w')))
   return;

$counter = 1;
/* string type variations */
fprintf($fp, "\n*** Testing fprintf() for string types ***\n");
foreach( $string_variation as $string_var ) {
  fprintf( $fp, "\n-- Iteration %d --\n",$counter);
  foreach( $strings as $str ) {
    fprintf( $fp, "\n");
    fprintf( $fp, $string_var, $str );
  }
  $counter++;
}

fclose($fp);

echo "\nDone";

?>
--EXPECT--
*** Testing fprintf() for string types ***

-- Iteration 1 --

     
  abc
  aaa
-- Iteration 2 --

     
abc  
aaa  
-- Iteration 3 --

00000
00abc
00aaa
-- Iteration 4 --

#####
##abc
##aaa
Done
