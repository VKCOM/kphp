@ok k2_skip
<?php

$strings = [
  '',
  ' ',
  'string without tags',
  '<p>incomplete tag',
  'incomplete tag</p>',
  '<p id="foo"></p>',
  '< p id="foo" >< / p >',
  ' <p id="foo">text</p> ',
  'text<br>',
  'text<br/>',
  'text<br id="foo">',
  'text<br id="foo"/>',
  'text<br />',
  '<br>',
  '<br trap=">">',
  ' <HTML><BODY><P>upper case tags</P> text</BODY></HTML>',
  '<html><body><p>upper case tags</p> text</body></html> ',
  '<HTML><Body><p>mixed case tags</p> text</Body></HTML>',
  '<img class="emoji"><p>ok</p><img src="foo" class="emoji"><br><img src="foo">',
];

function run_string_test(string $s, string $allow) {
  var_dump(["strip_tags('$s', '$allow')" => strip_tags($s, $allow)]);
}

/** @param string[] $allow */
function run_array_test(string $s, array $allow) {
  $parts = implode(',', $allow);
  var_dump(["strip_tags('$s', [$parts])" => strip_tags($s, $allow)]);
}

foreach ($strings as $s) {
  var_dump(strip_tags($s));
  var_dump(strip_tags($s, ''));
  var_dump(strip_tags($s, []));
  var_dump(strip_tags($s, ['']));
  var_dump(strip_tags($s, ['', '']));

  // not a valid usage, but should fail in identical way;
  // all these $allowed arguments are silently (sic) ignored
  var_dump(strip_tags($s, 'p'));
  var_dump(strip_tags($s, '<br/>'));
  var_dump(strip_tags($s, '</br>'));
  var_dump(strip_tags($s, '</body>'));
  var_dump(strip_tags($s, '<p/>'));
  var_dump(strip_tags($s, '?'));
  var_dump(strip_tags($s, '< p >'));
  var_dump(strip_tags($s, ' <p>'));
  var_dump(strip_tags($s, '<img class="emoji">'));

  // single string as allow filter
  var_dump(strip_tags($s, '<br>'));
  var_dump(strip_tags($s, '<Br>'));
  var_dump(strip_tags($s, '<p>'));

  // string with multiple tags as allow filter
  var_dump(strip_tags($s, '<p><br>'));
  var_dump(strip_tags($s, '<p><img>'));
  var_dump(strip_tags($s, '<p><br><html>'));

  // string array allowlist
  run_array_test($s, ['p']);
  run_array_test($s, ['p', 'BR']);
  run_array_test($s, ['first' => 'p', 'second' => 'BR']);
  run_array_test($s, ['p', 'br', 'HTML', 'body']);

  // <> are not needed when arrays are used,
  // '<' and '>' are appended to every string argument
  // var_dump(strip_tags($s, ['<p>']));
  run_array_test($s, ['<p>', '<BR/>']);
}
