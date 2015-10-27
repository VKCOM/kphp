@ok
<?php
  var_dump (preg_match ('/\s/', json_decode ('"\u00a0"')));
  var_dump (preg_match ('/\s/u', json_decode ('"\u00a0"')));
