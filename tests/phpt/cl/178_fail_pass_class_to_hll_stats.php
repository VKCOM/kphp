@kphp_should_fail
/conversion from A to mixed is forbidden/
<?php
class A {}
function demo() {
  var_dump(vk_stats_hll_merge(new A));
}
demo();
