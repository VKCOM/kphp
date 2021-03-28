@kphp_should_fail
/Calling function marked as internal outside of functions with the color message\-module \(someFunction\(\) call messageInternals\(\)\)/
/  someFunction\(\)/
/  messageInternals\(\) with following colors\: \{message\-internals\}/
/Produced according to the following rule:/
/  "\* message-internals" => Calling function marked as internal outside of functions with the color message-module/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
        "* message-internals"              => "Calling function marked as internal outside of functions with the color message-module",
        "message-module message-internals" => 1,
    ],
  ];
}

/**
 * @kphp-color message-internals
 */
function messageInternals() {
    echo 1;
}

function someFunction() {
    messageInternals();
}

someFunction();
