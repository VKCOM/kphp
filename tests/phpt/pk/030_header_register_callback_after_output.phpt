@ok
<?php

echo "one";

header_register_callback(function () {
    echo "three";
});

echo "two";
