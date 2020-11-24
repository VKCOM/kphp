@kphp_should_fail
/@kphp-warn-performance bad tag: there are no any inspection\
<?php

/**
 *  @kphp-warn-performance
 */
function a1() {}

a1();
