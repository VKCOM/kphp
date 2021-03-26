@kphp_should_fail
/Calling no\-highload function from highload function \(someHighload\(\) call someNoHighload\(\)\)/
/  someHighload\(\) with following colors\: \{highload\}/
/  someNoHighload\(\) with following colors\: \{no\-highload\}/
/Produced according to the following rule:/
/  "highload no-highload" => Calling no-highload function from highload function/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
        "highload no-highload" => "Calling no-highload function from highload function",
    ],
  ];
}

/**
 * @kphp-color highload
 */
function someHighload() {
    someNoHighload();
}

/**
 * @kphp-color no-highload
 */
function someNoHighload() {
    echo 1;
}

someHighload();
