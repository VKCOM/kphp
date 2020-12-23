@ok
<?php

// this seems strange, but works (vk.com has a lot of such cases, maybe will stop working in the future, see TypeData::use_or_false)

/**
 * @param false $b
 */
function f1($b) {}

f1(false);
f1(true);

/**
 * @return false
 */
function f2(bool $b = true) { return $b; }

f2();
