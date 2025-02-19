@ok
<?php
  $url = 'http://username:password@hostname.ru:2323/path/path2?arg=value&arg2=value#http://username:password@hostname.ru:2323/path/path2?arg=value&arg2=value';
  $res = parse_url ($url);
  ksort ($res);
  var_dump ($res);

  for ($i = 0; $i < 8; $i++) {
    var_dump (parse_url ($url, $i));
  }

  var_dump (parse_url ($url, PHP_URL_SCHEME));
  var_dump (parse_url ($url, PHP_URL_HOST));
  var_dump (parse_url ($url, PHP_URL_PORT));
  var_dump (parse_url ($url, PHP_URL_USER));
  var_dump (parse_url ($url, PHP_URL_PASS));
  var_dump (parse_url ($url, PHP_URL_PATH));
  var_dump (parse_url ($url, PHP_URL_QUERY));
  var_dump (parse_url ($url, PHP_URL_FRAGMENT));

  $url = 'http:/hostname.ru/path/path2?arg=value&arg2=value#http://';
  $res = parse_url ($url);
  ksort ($res);
  var_dump ($res);

  for ($i = 0; $i < 8; $i++) {
    var_dump (parse_url ($url, $i));
  }


  $trans = array("h" => "-", "hello" => "hi", "hi" => "hello");
  echo strtr("hi all, I said hello world", $trans);

  $trans = array("נ" => "-", "נףההש" => "נר", "נר" => "נףההש");
  echo strtr("נר פההב ״ ûפרג נףההש צשךהג", $trans);

  echo strtr("baab", "ab", "01"),"\n";

  echo strtr("טפפט", "פט", "01"),"\n";

  $trans = array("ab" => "01");
  echo strtr("baab", $trans);

  $unencripted = "hello";
  $from = "abcdefghijklmnopqrstuvwxyz";
  $to =    "zyxwvutsrqponmlkjihgfedcba";
  echo strtr($unencripted, $from, $to)."\n";

  $unencripted = "נףההש";
  $from = "פטסגףאןנרמכהüעשחיךûודלצקםÿ";
  $to =    "ÿםקצלדוûךיחשעüהכמרנןאףגסטפ";
  echo strtr($unencripted, $from, $to)."\n";
