@kphp_should_fail
/Calling function marked as internal outside of functions with the color message-module/
/ -> someFunction -> messageInternals@message-internals/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
        "message-internals"              => "Calling function marked as internal outside of functions with the color message-module",
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
