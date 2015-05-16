@ok
<?php


echo http_build_query(array("v"=>1)) . "\n";
echo http_build_query(array("v"=>"1")) . "\n";
echo http_build_query(array("w"=>array("v"=>1))) . "\n";
echo http_build_query(array("v"=>1, "w"=>"1", "u"=>array(1))) . "\n";
