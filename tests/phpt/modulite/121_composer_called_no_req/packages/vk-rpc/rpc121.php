<?php

function rpcFunctionCallingVkStrings() {
    // invalid, because "vk/rpc" composer.json "require" does mention vk/strings
    // (which is automatically transformed into require #vk/strings from implicit #vk/rpc modulite)
    // so, while it works in PHP (and in KPHP with modulite disabled), it's an error
    echo concatStr('rpc', 'call'), "\n";
}
