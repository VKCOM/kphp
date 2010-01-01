@ok
<?php
  for ($i = 0; $i < 3; ++$i) {
    $str = 'test<br>test';
    $str = strip_tags($str, '<br>');
    echo $str."\n";
  }

  var_dump(strip_tags("<br/>", "<br>"));
  $text = '<p>Параграф.</p><!-- Комментарий --> <a href="#fragment">Еще текст</a>';
  echo strip_tags($text);
  echo "\n";

  echo strip_tags($text, '<p><a>');

  $new = htmlspecialchars("<a href='test'>Test</a>");
  echo $new;

  $str = "<p>this -&gt; &quot;</p>\n";

  echo htmlspecialchars_decode($str);

  echo htmlspecialchars_decode($str, ENT_QUOTES);

  $orig = "I'll \"walk\" the <b>dog</b> now";

  $a = htmlentities($orig);

  $b = html_entity_decode(strval($a."&#039;&qwe;&leqwer;"));

  echo $a; // I'll &quot;walk&quot; the &lt;b&gt;dog&lt;/b&gt; now

  echo $b; // I'll "walk" the <b>dog</b> now

  $str = "A 'quote' is <b>bold</b>\n";

  var_dump (htmlspecialchars($str));
  var_dump (htmlentities($str));

  $str = "A 'цитата' ЕСТЬ \"<b>bold</b>!@#$%^&*(){}[]=-:\";'/.,<>?`~\n";

  for ($i = 0; $i <= 255; $i++) {
    if ($i != 0x98) {
      $s .= chr ($i);
    }
  }

  var_dump ($str, bin2hex ($str));
  var_dump ($s, bin2hex ($s));

#ifndef KittenPHP

  var_dump (bin2hex (htmlentities($str, ENT_COMPAT | ENT_HTML401, 'cp1251')));

  var_dump (bin2hex (htmlspecialchars($str, ENT_COMPAT | ENT_HTML401, 'cp1251')));

  var_dump (bin2hex (htmlentities($s, ENT_COMPAT | ENT_HTML401, 'cp1251')));

//  var_dump (mb_convert_encoding ($s, 'HTML-ENTITIES', 'cp1251'));

  if (0) {

#endif

  var_dump (bin2hex (htmlentities($str)));

  var_dump (bin2hex (htmlspecialchars($str)));

  var_dump (bin2hex (htmlentities($s)));

//  var_dump (htmlspecialchars_decode (htmlentities($s)));

#ifndef KittenPHP

  }

#endif
