@ok
<?php
?>

<?php
echo <<<TAG
fdjsil
fjdksl
jkflds
onclick='prepareReport(\"" + type + "\", 1);'
TAG;

/**
 * @kphp-infer
 * @return string
 */
function f1() {
  global $config;
  return <<<EOF
EOF;
}

/**
 * @kphp-infer
 * @return string
 */
function f2() {

  global $lang;
  return <<<EOF
<div class="feedback_answer_tail_wrap"><div class="feedback_answer_tail"></div></div>
EOF;
}

f1();
echo f2();

?>
