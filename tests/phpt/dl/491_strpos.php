@ok k2_skip
<?php
  $str = "abacaba asdas nasdn ansdn andsn d";
  var_dump (mb_strtolower($str, 'UTF-8'));
  var_dump (mb_strtoupper($str, 'UTF-8'));
  $str = "Τάχιστη αλώπηξ βαφής ψημένη γη, δρασκελίζει υπέρ νωθρού κυνός";
  var_dump (mb_strtolower($str, 'UTF-8'));
  var_dump (mb_strtoupper($str, 'UTF-8'));
  $str = "ыФВАфывАЫВАФЫВафывфаыввмсмТлГШщГЦУКЕеМё      abacaba 123124 ABACABA ǅǄǆǈǇǉǲǱǳ";
  $str = "ыФВАфывАЫВАФЫВафывфаыввмсмТлГШщГЦУКЕеМё      abacaba 123124 ABACABA ǄǇǱ";
  var_dump ($str);
  var_dump (mb_strtolower($str, 'UTF-8'));
  var_dump (mb_strtoupper($str, 'UTF-8'));
//  $str = "ыФВАфывАЫВАФЫВафывфаыввмсмТлГШщГЦУКЕеМё      abacaba 123124 ABACABA ǅǄǆǇǉǲǱǳ";
//  $str = "йцукенгшщзхъфывапролджэячсмитьбюqwertyuiopasdfghjklzxcvbnmAQWSEDRFTGYHUJIKOLPZXCVBNM";
//  for ($i = 0; $i < 100000; $i++) {
//    mb_strtoupper($str, 'UTF-8');
//  }

  var_dump(strpos ("aBababaBa", "aba"));
  var_dump(strrpos ("aBababaBa", "aba"));
  var_dump(stripos ("aBababaBa", "aba"));
  var_dump(strripos ("aBababaBa", "aba"));

  var_dump(mb_stripos ("aBababaBa", "aba"));

  var_dump(mb_stripos ("���������", "���", 0, "CP1251"));

  var_dump(mb_strtolower ("�������������423423����� �� �� � ��� 34� �;�; �;  � �  �� ⸨��  � � � ��� ��", "CP1251"));

  var_dump(mb_strtolower ("ыФВАфывАЫВАФЫВафывфаыввмсмТлГШщГЦУКЕеМё", "UTF-8"));
  var_dump(mb_strtoupper ("ыФВАфывАЫВАФЫВафывфаыввмсмТлГШщГЦУКЕеМё", "UTF-8"));

  var_dump(mb_strpos ("ыФВАфывАЫВАФЫВафывфаыввмсмТлГШщГЦУКЕеМё", "вАЫВ", 0, "UTF-8"));
  var_dump(mb_strpos ("ыФВАфывАЫВАФЫВафывфаыввмсмТлГШщГЦУКЕеМё", "вАЫВ", 7, "UTF-8"));

  var_dump(mb_strlen ("ыФВАфывАЫВАФЫВафывфаыввмсмТлГШщГЦУКЕеМё", "CP1251"));
  var_dump(mb_strlen ("ыФВАфывАЫВАФЫВафывфаыввмсмТлГШщГЦУКЕеМё", "UTF-8"));

  var_dump(mb_substr ("ыФВАфывАЫВАФЫВафывфаыввмсмТлГШщГЦУКЕеМё", -15, -7, "CP1251"));
  var_dump(mb_substr ("ыФВАфывАЫВАФЫВафывфаыввмсмТлГШщГЦУКЕеМё", -15, -7, "UTF-8"));

  var_dump(mb_strlen ('-', 'UTF-8'));
  var_dump(mb_substr ('-', 0, 1, 'UTF-8'));
  var_dump(mb_substr ("-1", 0, 1, 'UTF-8'));

  var_dump(mb_strtolower ("�����������", "CP1251"));
//  var_dump(mb_strtolower ("�������������������������������ި"));
  var_dump(mb_strtolower ("ыФВАфывАЫВАФЫВафывфаыввмсмТлГШщГЦУКЕеМё", "UTF-8"));
  $str = "Τάχιστη αλώπηξ βαφής ψημένη γη, δρασκελίζει υπέρ νωθρού κυνός";
  var_dump (mb_strtolower($str, 'UTF-8'));
  var_dump (mb_strtoupper($str, 'UTF-8'));
  // var_dump(strtolower ("���������"));

  var_dump(strpos("abcdABCDefghEFGHefgh", "efgh", -10));
  var_dump(strpos("abcdABCDefghEFGHefgh", "efgh", -16));
  var_dump(strpos("abcdABCDefghEFGh_H", "h", -10));
  var_dump(strpos("abcdABCDefghEFGh_H", "h", -5));
  var_dump(strpos("abcdABCDefghEFGh_H", "h", -2));
  var_dump(strpos("0123456789a123456789b123456789c", "7", -5));
  var_dump(strpos("0123456789a123456789b123456789c", "7", -25));
  var_dump(strpos("0123456789a123456789b123456789c", "c", -1));
  var_dump(strpos("0123456789a123456789b123456789c", "9", -1));

  var_dump(stripos("abcdABCDefghEFGHefgh", "efgh", -10));
  var_dump(stripos("abcdABCDefghEFGHefgh", "efgh", -16));
  var_dump(stripos("abcdABCDefghEFGh_H", "h", -10));
  var_dump(stripos("abcdABCDefghEFGh_H", "h", -5));
  var_dump(stripos("abcdABCDefghEFGh_H", "h", -2));
  var_dump(stripos("0123456789a123456789b123456789c", "7", -5));
  var_dump(stripos("0123456789a123456789b123456789c", "7", -25));
  var_dump(stripos("0123456789a123456789b123456789c", "c", -1));
  var_dump(stripos("0123456789a123456789b123456789c", "9", -1));


  $a = array ("                                             ", "The quick brown fox jumped over the lazy dog.", "A very long woooooooooooord.", "A very long wooooooooooooord.", "A very long woooooooooooord.", "", "A   very  long    wooooooooooooord.", "A   very  long    wooooooooooooord        234234234 e r we we r we rwe f  w     ew r wer we                  wer wer we r wer .");
  foreach ($a as $str) {
    var_dump (nl2br (wordwrap ($str, 1, "\n", true)));
    var_dump (nl2br (wordwrap ($str, 1, "\n", false)));
    var_dump (nl2br (wordwrap ($str, 2, "\n", true)));
    var_dump (nl2br (wordwrap ($str, 2, "\n", false)));
    var_dump (nl2br (wordwrap ($str, 8, "</br >\n", true)));
    var_dump (nl2br (wordwrap ($str, 8, "\n", false)));
    var_dump (nl2br (wordwrap ($str, 20, "\n", true)));
    var_dump (nl2br (wordwrap ($str, 20, "</br >\n", false)));
    var_dump (nl2br (wordwrap ($str, 45, "\n", true)));
    var_dump (nl2br (wordwrap ($str, 45, "</br >\n", false)));
    var_dump (nl2br (wordwrap ($str, 100, "\n", true)));
    var_dump (nl2br (wordwrap ($str, 100, "\n", false)));
  } 

  $foo = "0123456789a123456789b123456789c";

  var_dump(strrpos($foo, '7', -5));

  var_dump(strrpos($foo, '7', 20));

  var_dump(strrpos($foo, '7', 28));

  var_dump(strrpos($foo, "01", -31));

  var_dump(strrpos($foo, "01", -30));

  var_dump(strrpos($foo, $foo, -31));

  var_dump(strrpos($foo, $foo, -30));

  var_dump(strrpos($foo, $foo, -1));

  var_dump(strrpos($foo, substr ($foo, 0, strlen ($foo) - 1), -1));


  var_dump(strripos($foo, '7', -5));

  var_dump(strripos($foo, '7', 20));

  var_dump(strripos($foo, '7', 28));

  var_dump(strripos($foo, "01", -31));

  var_dump(strripos($foo, "01", -30));

  var_dump(strripos($foo, $foo, -31));

  var_dump(strripos($foo, $foo, -30));

  var_dump(strripos($foo, $foo, -1));

  var_dump(strripos($foo, substr ($foo, 0, strlen ($foo) - 1), -1));

  var_dump(strpos("hello world", 1 ? 'o' : false));
