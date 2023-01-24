@kphp_should_fail
/Too few arguments in call to JsonEncoder::encode\(\), expected 1, have 0/
/Too few arguments in call to MyE::decode\(\), expected 2, have 0/
<?php

class MyE extends JsonEncoder {}

JsonEncoder::encode();
MyE::decode();

