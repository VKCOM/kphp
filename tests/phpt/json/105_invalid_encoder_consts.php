@kphp_should_fail
/MyE::encode\(null\);/
/@kphp-json 'visibility_policy' should be all|public, got 'hm'/
/@kphp-json 'float_precision' should be non negative integer, got '`const float_precision` not int'/
<?php


class MyE extends JsonEncoder {
    const visibility_policy = 'hm';
    const float_precision = 'asdf';
}

MyE::encode(null);
