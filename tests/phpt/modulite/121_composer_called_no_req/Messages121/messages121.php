<?php

function messagesFunctionCallingVkStrings() {
    // invalid, because @msg does not require #vk/strings
    echo concatStr('msg', 'call'), "\n";
}
