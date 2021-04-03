@ok
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
        "danger-zone *"           => "Calling function without color danger-zone in a function with color danger-zone",
        "danger-zone danger-zone" => 1,
    ],
  ];
}
// disabled until any support
//
// /**
//  * @kphp-color danger-zone
//  */
// function dangerZone() {
//     dangerZoneToo(); // ok, has color
//     echo 1; // ok, is extern
// }
//
// /**
//  * @kphp-color danger-zone
//  */
// function dangerZoneToo() {
//     echo 1;
// }
//
// dangerZone();
