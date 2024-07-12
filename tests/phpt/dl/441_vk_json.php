@ok benchmark k2_skip
<?php
  $data1 = array('hello world', '~!@#$%^&*()_+|', 'ashdasdfhasdfbhsdfb hfgh jhweg 23g 3hhg3 12 123 123 !@#!@#  !@#!@#@#$#%$^%&^*^&(*)(_)_"><:<"><:><"><">:"<">"<:><>"<||||{}|{}|{}|[]\\[]\\[]\\[]\[\]\[\]\[\]\[]\\[\]\[]\[\]\\[]\[\\\\\\\\\\\\\\\\\\\\');

  for ($i = 0; $i < 500000; $i++) {
    $data_encoded = vk_json_encode ($data1);
  }

  var_dump($data_encoded);
  var_dump(strlen($data_encoded));

