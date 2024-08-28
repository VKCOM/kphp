@ok k2_skip
<?php

function isVector($array, $force_array = false) {
  if (is_array($array)) {
    if ($count = count($array)) {
      for ($i = 0; $i < $count; ++$i) {
        if (!isset($array[$i]) && !array_key_exists($i, $array)) {
          return false;
        }
      }
      return true;
    }
    return $force_array;
  }
  return false;
}

function xmlStripInvalidCharacters($text) {
  $result = '';

  for ($i = 0; $i < strlen($text); $i++) {
    $ch = ord($text[$i]);
    // remove any characters outside the valid XML characters range
    if ($ch == 0x9 || $ch == 0xA || $ch == 0xD || ($ch >= 0x20 && $ch <= 0xD7FF) || ($ch >= 0xE000 && $ch <= 0xFFFD) || ($ch >= 0x10000 && $ch <= 0x10FFFF)) {
      $result .= $text[$i];
    }
  }
  return $result;
}

function convertToXML($data, $level = 0) {
  global $StrictMode;

  $result = "";
  $spaces = str_repeat(' ', $level);
  foreach($data as $k => $v) {
    if ($v === '' || $v === null) {
      $result .= $spaces."<$k/>\n";
    } else {
      if (is_scalar($v)){
        $v = str_replace(array('<', '>', '&'), array('&lt;', '&gt;', '&amp;'), $v);
        $v = $StrictMode ? xmlStripInvalidCharacters($v) : $v;
        $result .= $spaces . "<$k>$v</$k>\n";
      } else if (isVector($v)){
        $result .= $spaces . "<$k list=\"true\">\n";
        foreach ($v as $i => $item){
    if ($item === null) {
      $result .= $spaces." <null/>\n";
    } else if (is_scalar($item)) {
            $result .= convertToXML(array("item"=>$item), $level + 1);
          } else if (isVector($item)) {
            $result .= convertToXML(array("list"=>$item), $level + 1);
          } else {
            $result .= convertToXML($item, $level + 1);
          }
        }
        $result .= $spaces . "</$k>\n";
      } else { // is associative array
        $result .= $spaces . "<$k>\n" . convertToXML($v, $level + 1) . $spaces . "</$k>\n";
      }
    }
  }
  return $result;
}

$StrictMode = false;
var_dump (convertToXML (array ('1' => 2, "a" => 3, '4')));