@ok callback k2_skip
<?php

#var_dump( array_map(NULL, array()) );

/**
 * @kphp-required
 */
function square ($n) {
  return $n * $n;
}

/**
 * @kphp-required
 */
function sq ($n) {
  return $n * $n;
}

//var_dump(array_map(sq, array()));
var_dump(array_map('sq', array(1)));

var_dump( array_map( 'square',
                     array("key1"=>1, "key2"=>2, "key3"=>3)
                   )
        );

$a = array(1,2,3);

/**
 * @kphp-required
 */
function square_recur($var) {
   if (is_array($var))
     return array_map('square_recur', $var);
   return $var * $var;
}
$rec_array = array(1, 2, array(3, 4, array(5, 2), array() ) );
var_dump( array_map('square_recur', $rec_array) );

var_dump( array_map("square", $a) );

#$string_var = "square";
#var_dump( array_map($string_var, $a) ); 

#var_dump( array_map() );

#var_dump( array_map( 'echo', array(1) ) );
#var_dump( array_map( 'array', array(1) ) );
#var_dump( array_map( 'empty', array(1) ) );
#var_dump( array_map( 'eval', array(1) ) );
#var_dump( array_map( 'exit', array(1) ) );
#var_dump( array_map( 'isset', array(1) ) );
#var_dump( array_map( 'list', array(1) ) );
#var_dump( array_map( 'print', array(1) ) );


