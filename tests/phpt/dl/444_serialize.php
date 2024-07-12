@ok benchmark k2_skip
<?php

  $str[0] = 'a:40:{s:2:"id";s:7:"6354412";s:5:"email";s:13:"levlam@levlam";s:10:"first_name";s:7:"Алексей";s:9:"last_name";s:5:"Левин";s:3:"sex";s:1:"2";s:5:"photo";s:15:"9xxxbxxxd8x:001";s:10:"university";s:1:"1";s:15:"university_name";s:5:"СПбГУ";s:12:"faculty_name";s:23:"Математико-механический";s:10:"graduation";s:4:"2010";s:7:"faculty";s:1:"7";s:6:"domain";s:6:"levlam";s:6:"joined";s:10:"1200875212";s:6:"logged";s:10:"1252407393";s:10:"photo_data";s:0:"";s:6:"server";s:4:"4347";s:7:"server2";s:1:"0";s:11:"uni_country";s:1:"1";s:8:"uni_city";s:1:"2";s:11:"time_offset";s:1:"0";s:14:"services_bits1";s:9:"633211943";s:14:"services_bits2";s:9:"562802944";s:14:"services_bits3";s:4:"1880";s:14:"services_bits4";s:2:"87";s:8:"bday_day";s:1:"9";s:10:"bday_month";s:1:"9";s:9:"bday_year";s:4:"1987";s:15:"bday_visibility";s:1:"1";s:12:"mobile_phone";s:0:"";s:10:"home_phone";s:0:"";s:4:"lang";s:1:"0";s:7:"site_id";s:1:"0";s:9:"reg_phone";s:11:"12345678901";s:16:"reg_phone_status";s:1:"2";s:9:"is_banned";s:1:"0";s:8:"reg_step";s:1:"0";s:12:"profile_type";s:1:"0";s:7:"version";i:6;s:14:"first_name_lat";s:6:"Alexey";s:13:"last_name_lat";s:5:"Levin";}';
  $str[1] = 'a:40:{s:2:"id";i:6354412;s:5:"email";i:13;s:10:"first_name";i:7;s:9:"last_name";i:5;s:3:"sex";i:2;s:5:"photo";i:15;s:10:"university";i:1;s:15:"university_name";i:5;s:12:"faculty_name";i:2;s:10:"graduation";i:2010;s:7:"faculty";i:7;s:6:"domain";i:6;s:6:"joined";i:1200875212;s:6:"logged";i:1252407393;s:10:"photo_data";i:-1;s:6:"server";i:4347;s:7:"server2";i:0;s:11:"uni_country";i:1;s:8:"uni_city";i:2;s:11:"time_offset";i:0;s:14:"services_bits1";i:633211943;s:14:"services_bits2";i:562802944;s:14:"services_bits3";i:1880;s:14:"services_bits4";i:87;s:8:"bday_day";i:9;s:10:"bday_month";i:9;s:9:"bday_year";i:1987;s:15:"bday_visibility";i:1;s:12:"mobile_phone";i:0;s:10:"home_phone";i:0;s:4:"lang";i:1;s:7:"site_id";i:1;s:9:"reg_phone";i:1234567891;s:16:"reg_phone_status";i:2;s:9:"is_banned";i:0;s:8:"reg_step";i:1;s:12:"profile_type";i:1;s:7:"version";i:6;s:14:"first_name_lat";i:6;s:13:"last_name_lat";i:5;}';
  $str[2] = 'a:40:{s:2:"id";d:6354412.0;s:5:"email";d:13.0;s:10:"first_name";d:7.0;s:9:"last_name";d:5.0;s:3:"sex";d:2.0;s:5:"photo";d:15.0;s:10:"university";d:1.0;s:15:"university_name";d:5.0;s:12:"faculty_name";d:2.0;s:10:"graduation";d:2010.0;s:7:"faculty";d:7.0;s:6:"domain";d:6.0;s:6:"joined";d:1200875212.0;s:6:"logged";d:1252407393.0;s:10:"photo_data";d:-1.0;s:6:"server";d:4347.0;s:7:"server2";d:0.0;s:11:"uni_country";d:1.0;s:8:"uni_city";d:2.0;s:11:"time_offset";d:0.0;s:14:"services_bits1";d:633211943.0;s:14:"services_bits2";d:562802944.0;s:14:"services_bits3";d:1880.0;s:14:"services_bits4";d:87.0;s:8:"bday_day";d:9.0;s:10:"bday_month";d:9.0;s:9:"bday_year";d:1987.0;s:15:"bday_visibility";d:1.0;s:12:"mobile_phone";d:0.0;s:10:"home_phone";d:0.0;s:4:"lang";d:1.0;s:7:"site_id";d:1.0;s:9:"reg_phone";d:1234567891.0;s:16:"reg_phone_status";d:2.0;s:9:"is_banned";d:0.0;s:8:"reg_step";d:1.0;s:12:"profile_type";d:1.0;s:7:"version";d:6.0;s:14:"first_name_lat";d:6.0;s:13:"last_name_lat";d:5.0;}';

  for ($t = 0; $t < 3; $t++) {
    $s = unserialize ($str[$t]);
//    for ($i = 0; $i < 30000; $i++) {
//      serialize ($s);
//    }
    var_dump (serialize ($s));
  }

  for ($t = 0; $t < 3; $t++) {
    $s = $str[$t];
    for ($i = 0; $i < 30000; $i++) {
      unserialize ($s);
    }
    var_dump (unserialize ($s));
  }

