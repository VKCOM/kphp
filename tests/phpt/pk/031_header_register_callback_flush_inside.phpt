@ok
<?php

echo "Zero ";
header_register_callback(function () {
    echo "Two ";
    flush();
    echo "Three ";
});
echo "One ";

