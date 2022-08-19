@ok
<?php

$date = new DateTime("2005-07-14 22:30:41");

// basic
var_dump( $date->format( "F j, Y, g:i a") );
var_dump( $date->format( "m.d.y") );
var_dump( $date->format( "j, n, Y") );
var_dump( $date->format( "Ymd") );
var_dump( $date->format( 'h-i-s, j-m-y, it is w Day') );
var_dump( $date->format( '\i\t \i\s \t\h\e jS \d\a\y.') );
var_dump( $date->format( "D M j G:i:s T Y") );
var_dump( $date->format( 'H:m:s \m \i\s\ \m\o\n\t\h') );
var_dump( $date->format( "H:i:s") );
var_dump( $date->format( "c") );
var_dump( $date->format( "r") );
var_dump( $date->format( "U") );

// negative year
$date2 = new DateTime("-2005-07-14 22:30:41");
var_dump( $date2->format( "Y") );
var_dump( $date2->format( "c") );

// leap year
var_dump ((new DateTime("92-07-14"))->format( "L"));
var_dump ((new DateTime("96-07-14"))->format( "L"));
var_dump ((new DateTime("100-07-14"))->format( "L"));
var_dump ((new DateTime("104-07-14"))->format( "L"));
var_dump ((new DateTime("196-07-14"))->format( "L"));
var_dump ((new DateTime("200-07-14"))->format( "L"));
var_dump ((new DateTime("204-07-14"))->format( "L"));
var_dump ((new DateTime("300-07-14"))->format( "L"));
var_dump ((new DateTime("300-07-14"))->format( "L"));
var_dump ((new DateTime("396-07-14"))->format( "L"));
var_dump ((new DateTime("400-07-14"))->format( "L"));
var_dump ((new DateTime("404-07-14"))->format( "L"));
