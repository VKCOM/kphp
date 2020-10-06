@ok
<?php

  $_POST = array();
  $data1 = "--ARCFormBoundaryxmrxn51uw86ko6r\r\nContent-Disposition: form-data; name=\"v\"\r\n\r\n4.6\r\n--ARCFormBoundaryxmrxn51uw86ko6r\r\nContent-Disposition: form-data; name=\"lang\"\r\n\r\nru\r\n--ARCFormBoundaryxmrxn51uw86ko6r\r\nContent-Disposition: form-data; name=\"user\"\r\n\r\n56545441\r\n--ARCFormBoundaryxmrxn51uw86ko6r\r\nContent-Disposition: form-data; name=\"access_token\"\r\n\r\ntoken\r\n--ARCFormBoundaryxmrxn51uw86ko6r--\r\n";
#ifndef KPHP
  $_POST = ["v" => "4.6", "lang" => "ru", "user" => "56545441", "access_token" => "token"];
  if (false)
#endif
  parse_multipart ($data1, 'ARCFormBoundaryxmrxn51uw86ko6r');
  var_dump ($_POST);


  $_POST = array();
  $data2 = "------WebKitFormBoundaryURtgMuDGAJrvANlm\r\nContent-Disposition: form-data; name=\"mid\"\r\n\r\n2943\r\n------WebKitFormBoundaryURtgMuDGAJrvANlm\r\nContent-Disposition: form-data; name=\"aid\"\r\n\r\n-3\r\n------WebKitFormBoundaryURtgMuDGAJrvANlm\r\nContent-Disposition: form-data; name=\"gid\"\r\n\r\n0\r\n------WebKitFormBoundaryURtgMuDGAJrvANlm\r\nContent-Disposition: form-data; name=\"hash\"\r\n\r\n3524497eba8d015ff0043662e8d4b787\r\n------WebKitFormBoundaryURtgMuDGAJrvANlm\r\nContent-Disposition: form-data; name=\"rhash\"\r\n\r\n658bd772b0d69d98f1e85cff3d127414\r\n------WebKitFormBoundaryURtgMuDGAJrvANlm\r\nContent-Disposition: form-data; name=\"al\"\r\n\r\n1\r\n------WebKitFormBoundaryURtgMuDGAJrvANlm\r\nContent-Disposition: form-data; name=\"act\"\r\n\r\ncheck_upload\r\n------WebKitFormBoundaryURtgMuDGAJrvANlm\r\nContent-Disposition: form-data; name=\"type\"\r\n\r\nphoto\r\n------WebKitFormBoundaryURtgMuDGAJrvANlm\r\nContent-Disposition: form-data; name=\"ondone\"\r\n------WebKitFormBoundaryURtgMuDGAJrvANlm------\r\n";
#ifndef KPHP
  $_POST = [
    "mid" => "2943", "aid" => "-3", "gid" => "0", "hash" => "3524497eba8d015ff0043662e8d4b787",
    "rhash" => "658bd772b0d69d98f1e85cff3d127414", "al" => "1", "act" => "check_upload", "type" => "photo"
  ];
  if (false)
#endif
  parse_multipart ($data2, '----WebKitFormBoundaryURtgMuDGAJrvANlm');
  var_dump ($_POST);


  $_POST = array();
  $data3 = "\r\n\r\n--Asrf456BGe4h\r\nContent-Disposition: form-data; name=\"DestAddress\"\r\n\r\nbrutal-vasya@example.com\r\n--Asrf456BGe4h\r\nContent-Disposition: form-data; name=\"MessageTitle\"\r\n\r\nЯ негодую\r\n--Asrf456BGe4h\r\nContent-Disposition: form-data; name=\"MessageText\"\r\n\r\nПривет, Василий! Твой ручной лев, которого ты оставил\r\nу меня на прошлой неделе, разодрал весь мой диван.\r\nПожалуйста забери его скорее!\r\nВо вложении две фотки с последствиями.\r\n--Asrf456BGe4h\r\nContent-Disposition: form-data; name=\"AttachedFile1\"; filename=\"horror-photo-1.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n\r\n--Asrf456BGe4h\r\nContent-Disposition: form-data; name=\"AttachedFile2\"; filename=\"horror-photo-2.jpg\"\r\nContent-Type: image/jpeg\r\n\r\nsdfsdfsdafasdf\r\n--Asrf456BGe4h\r\nContent-Disposition: form-data; name=\"temporary\"\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\ntest=1&test2=3.14\r\n--Asrf456BGe4h--\r\n\r\n";
#ifndef KPHP
  $_POST = [
    "DestAddress" => "brutal-vasya@example.com", "MessageTitle" => "Я негодую",
    "MessageText" => "Привет, Василий! Твой ручной лев, которого ты оставил\r\nу меня на прошлой неделе, разодрал весь мой диван.\r\nПожалуйста забери его скорее!\r\nВо вложении две фотки с последствиями.",
    "temporary" => ["test" => "1", "test2" => "3.14"]
  ];
  if (false)
#endif
  parse_multipart ($data3, 'Asrf456BGe4h');
  var_dump ($_POST);
