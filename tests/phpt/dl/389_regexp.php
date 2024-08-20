@ok k2_skip
<?php
  var_dump (preg_match ('/\s/', json_decode ('"\u00a0"')));
  var_dump (preg_match ('/\s/u', json_decode ('"\u00a0"')));

  var_dump(preg_match('/\b./u', 'f z', $matches));
  var_dump($matches);
  var_dump(preg_match('/\b./u', 'б д', $matches));
  var_dump($matches);
