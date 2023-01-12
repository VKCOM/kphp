@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/pass int\|false to argument \$a of takeInt/
/pass \?int to argument \$a of takeInt/
<?php


function takeInt(int $a) {}

/**
 * @kphp-generic T
 * @param ?T $data
 * @return not_null(T)
 */
function dropNull($data) {
  return not_null($data);
}

/**
 * @kphp-generic T
 * @param T $data
 * @return not_false<T>
 */
function dropFalse($data) {
  return not_null($data);
}

/**
 * @kphp-generic T
 * @param T $data
 * @return not_false<not_null<T>>
 */
function dropNullFalse($data) {
  return not_false(not_null($data));
}

/** @var int|null|false $i1 */
$i1 = 0;
takeInt(dropNull(dropNull($i1)));
takeInt(dropFalse/*<?int>*/(dropNullFalse($i1) ?? null));

