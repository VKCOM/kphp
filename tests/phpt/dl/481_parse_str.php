@ok k2_skip
<?php
  $array = array (array ('bar[' => array()), 'foo', array ('bar[' => array()), array ('bar[' => array()));
  $query = http_build_query ($array);
  var_dump ($query);


  $array = array ('foo', array ('bar[' => array ('baz]' => array (1, 2, 3))), 'test[]' => array (2, 3, 4));
  $query = http_build_query ($array);
  var_dump ($query);
  var_dump (urldecode ($query));
  parse_str ($query, $vars);
  var_dump (print_r ($vars, true));

  $str = "first=value&arr[]=foo+bar&arr[]=baz";

  parse_str ($str, $output);
  echo $output['first'];  // value
  echo $output['arr'][0]; // foo bar
  echo $output['arr'][1]; // baz
