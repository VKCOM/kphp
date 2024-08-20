@ok geoip k2_skip
<?php

function _dechex($u) {
  return sprintf('%x', $u);
}

function get_geoip ($ip) {
  global $tt, $ttt, $eng;
  #if (is_string ($ip)) {
  #  $ip = ip_to_int ($ip);
  #}
  $ip = ip2long ($ip);
//  $dip = $ip & 0x7fffffff;
  $r = array ();
// $key = "geoip.00." . ip_to_hex ($ip);
  $ttt += -microtime (true);

  for ($z = 1; $z <= 32; $z++) {
    if ($z == 32) {
      $key = "geoip.32." . dechex($ip);
    } else if ($z < 10) {
      $key = "geoip.0$z." . dechex(($ip >> (32 - $z)) & 0x7fffffff);
    } else {
      $key = "geoip.$z." . dechex(($ip >> (32 - $z)) & 0x7fffffff);
    }
    $r[] = $key;
  }

/*
  for ($z = 0; $z < 32; $z++) {
    $r[] = sprintf ('geoip.%02d.%x', $z, ($ip >> (32 - $z)) & 0x7fffffff);
  }
  $r[] = sprintf ('geoip.32.%x', $ip);
*/ 
  $ttt += microtime (true);
  $tt += -microtime (true);
//  $r = $eng->get ($r);
  $tt += microtime (true);
  #print "Engine: " . $tt . " time elapsed <br />\n";
  if (!$r) {
    return "??";
  }
  $max_key = max(array_keys($r));
  return $r[$max_key];
}


$port = 12039;

//$eng = new Memcache;
//$eng->connect('localhost', $port, 2) or die ("Could not connect");

$t = -microtime (true);
$ip = $argv[1];
if (isset ($_GET["ip"])) {
   $ip = $_GET['ip'];
}  
 
for ($i = 0; $i < 1000; $i++) { 
   $res = get_geoip ($ip) . "<br />\n";
}

$t += microtime (true);
print $res;

?>