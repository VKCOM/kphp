@ok php8
<?php

function wrong() {
  return "wrong";
}

echo match (true) {
  'truthy' => wrong(),
  ['truthy'] => wrong(),
  1 => wrong(),
  1.0 => wrong(),
  true => "true\n",
};
