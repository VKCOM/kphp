@kphp_should_fail k2_skip
/return string\|false\|null from getHtml/
/\$result is string\|false\|null/
/str_replace returns string\|false\|null/
/\$html is string\|false\|null/
/preg_replace returns string\|false\|null/
/\$html is string\|false\|null/
/"1" is string/
<?php

/**
 * @return string
 */
function getHtml() {
  $html = '1';
  if (1) {
    $html = preg_replace('~>\s+<~', '><', $html);
  }

  $result = str_replace('<span></span>', '<div></div>', $html);
  return $result;
}

getHtml();
