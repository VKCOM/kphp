@ok
<?php

function test_format_basic(DateTimeInterface $date) {
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
}

function test_format_basic_predefined_constants(DateTimeInterface $date) {
  var_dump( $date->format( DateTime::ATOM) ) ;
  var_dump( $date->format( DateTime::COOKIE) ) ;
  var_dump( $date->format( DateTime::ISO8601) ) ;
  var_dump( $date->format( DateTime::RFC822) ) ;
  var_dump( $date->format( DateTime::RFC850) ) ;
  var_dump( $date->format( DateTime::RFC1036) ) ;
  var_dump( $date->format( DateTime::RFC1123) ) ;
  var_dump( $date->format( DateTime::RFC7231) ) ;
  var_dump( $date->format( DateTime::RFC2822) ) ;
  var_dump( $date->format( DateTime::RFC3339) ) ;
  var_dump( $date->format( DateTime::RFC3339_EXTENDED) ) ;
  var_dump( $date->format( DateTime::RSS) ) ;
  var_dump( $date->format( DateTime::W3C) ) ;
}

function test_format_negative_year(DateTimeInterface $date) {
  var_dump( $date->format( "Y") );
  var_dump( $date->format( "c") );
}

function test_format_leap_year() {
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
}

function test_format_leap_year_immutable() {
  var_dump ((new DateTimeImmutable("92-07-14"))->format( "L"));
  var_dump ((new DateTimeImmutable("96-07-14"))->format( "L"));
  var_dump ((new DateTimeImmutable("100-07-14"))->format( "L"));
  var_dump ((new DateTimeImmutable("104-07-14"))->format( "L"));
  var_dump ((new DateTimeImmutable("196-07-14"))->format( "L"));
  var_dump ((new DateTimeImmutable("200-07-14"))->format( "L"));
  var_dump ((new DateTimeImmutable("204-07-14"))->format( "L"));
  var_dump ((new DateTimeImmutable("300-07-14"))->format( "L"));
  var_dump ((new DateTimeImmutable("300-07-14"))->format( "L"));
  var_dump ((new DateTimeImmutable("396-07-14"))->format( "L"));
  var_dump ((new DateTimeImmutable("400-07-14"))->format( "L"));
  var_dump ((new DateTimeImmutable("404-07-14"))->format( "L"));
}

test_format_basic(new DateTime("2005-07-14 22:30:41"));
test_format_basic(new DateTimeImmutable("2005-07-14 22:30:41"));
test_format_basic_predefined_constants(new DateTime("2005-07-14 22:30:41"));
test_format_basic_predefined_constants(new DateTimeImmutable("2005-07-14 22:30:41"));
test_format_negative_year(new DateTime("-2005-07-14 22:30:41"));
test_format_negative_year(new DateTimeImmutable("-2005-07-14 22:30:41"));
test_format_leap_year();
test_format_leap_year_immutable();
