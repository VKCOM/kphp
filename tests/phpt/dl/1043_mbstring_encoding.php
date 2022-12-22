@ok
<?php

function test_mb_string_encoding(string $encoding) {
  var_dump(mb_internal_encoding($encoding));
  var_dump(mb_internal_encoding());

  $bad_utf8 = substr('👾', 1);

  var_dump(mb_check_encoding($bad_utf8));
  var_dump(mb_check_encoding($bad_utf8, 'cp1251'));
  var_dump(mb_check_encoding($bad_utf8, 'utf-8'));

  echo mb_strlen('👾'), "\n";
  echo mb_strlen('👾', 'cp1251'), "\n";
  echo mb_strlen('👾', 'utf-8'), "\n";

  echo mb_strtolower('HIGH'), "\n";
  echo mb_strtolower('HIGH', 'cp1251'), "\n";
  echo mb_strtolower('HIGH', 'utf-8'), "\n";

  echo mb_strtoupper('über'), "\n";
  echo mb_strtoupper('über', 'cp1251'), "\n";
  echo mb_strtoupper('über', 'utf-8'), "\n";

  echo mb_strpos('Τάχιστη αλώπηξ βαφής ψημένη', 'βαφής', 0), "\n";
  echo mb_strpos('Τάχιστη αλώπηξ βαφής ψημένη', 'βαφής', 0, 'cp1251'), "\n";
  echo mb_strpos('Τάχιστη αλώπηξ βαφής ψημένη', 'βαφής', 0, 'utf-8'), "\n";

  echo mb_stripos('Τάχιστη αλώπηξ βαφής ψημένη', 'βαφής', 0), "\n";
  echo mb_stripos('Τάχιστη αλώπηξ βαφής ψημένη', 'βαφής', 0, 'cp1251'), "\n";
  echo mb_stripos('Τάχιστη αλώπηξ βαφής ψημένη', 'βαφής', 0, 'utf-8'), "\n";

  echo mb_substr('Τάχιστη αλώπηξ βαφής', 8, 6), "\n";
  echo mb_substr('Τάχιστη αλώπηξ βαφής', 8, 6, 'cp1251'), "\n";
  echo mb_substr('Τάχιστη αλώπηξ βαφής', 8, 6, 'utf-8'), "\n";
}

test_mb_string_encoding('utf-8');
test_mb_string_encoding('cp1251');
