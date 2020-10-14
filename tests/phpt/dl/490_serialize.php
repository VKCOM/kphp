@ok
<?php
  $x = null;
  $y = null;
  var_dump($v1 = serialize($x));
  var_dump($v2 = serialize(false));
  var_dump($v3 = serialize(true));
  var_dump($v4 = serialize(1));
  var_dump($v5 = serialize(1.0));
  var_dump($v6 = serialize("as"));
  var_dump($v7 = serialize(array ($x, "-1" => true, "" => 1, $y => 1.0, 3.4 => "as", array(), "abacaba" => $x, false => 12345)));

  var_dump(unserialize($v1));
  var_dump(unserialize($v2));
  var_dump(unserialize($v3));
  var_dump(unserialize($v4));
  var_dump(unserialize($v5));
  var_dump(unserialize($v6));
  var_dump(unserialize($v7));

  var_dump(unserialize("a:4:{i:1;i:26;b:0;i:27;s:3:\"asd\";i:28;d:1.34;i:29;}"));
  var_dump(unserialize('a:21:{s:2:"id";s:32:"0d4f6aa85fc99bea3c2835dc48219d4e";s:4:"date";i:1369798274;s:4:"lang";i:0;s:4:"page";s:6:"finish";s:7:"friends";a:0:{}s:6:"phones";a:1:{i:-79999999;a:3:{i:0;s:8:"75pinoho";i:1;i:1369778294;i:2;b:0;}}s:7:"site_id";i:-1;s:2:"ip";s:7:"8.8.8.8";s:7:"ip6norm";N;s:5:"front";N;s:4:"port";i:57817;s:7:"ua_hash";s:19:"9310425121950371247";s:10:"first_name";s:4:"Vasa";s:9:"last_name";s:5:"Babik";s:9:"nick_name";s:0:"";s:3:"sex";i:2;s:4:"from";s:0:"";s:4:"stat";i:1088050;s:7:"updated";i:1369778313;s:4:"code";s:2:"75";s:9:"validated";i:-2943;}'));

/**
 * @param int $n
 * @return string
 */
function rand_str_raw($n) {
  $res = "";
  for ($i = 0; $i < $n; $i++) {
    $res .= chr (rand(0, 255));
  }
  return $res;
}

  $s = rand_str_raw (10000);
  $a = array ($s => $s, "name" => 21312321);

  if (unserialize (serialize ($s)) !== $s) {
    echo "ERROR"."\n";
  } 

  if (unserialize (serialize ($a)) !== $a) {
    echo "ERROR"."\n";
  } 
