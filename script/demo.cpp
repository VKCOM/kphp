//crc64:7dd090160b52aeed
//crc64_with_comments:51a4ee10c02c8c18
#include "runtime-headers.h"
#include "demo.h"
//source = [dev.php]
//3: function demo() {
void f$demo(script_context_t * ctx) noexcept  {
  Optional < string > v$a;
  array< mixed > v$array;
  //4:     $a = "string";
  v$a = string("string");
  //5:     if (rand() % 2 == 0) {
  if (eq2 (modulo (f$rand(), 2_i64), 0_i64)) {
    //6:         $a = null;
    v$a = Optional<bool>{};
  };
  //7:     }
  //8:
  //9:     $array = ["string", 404, 2];
  v$array = array< mixed >::create(string("string"), 404_i64, 2_i64);
  //10:     if (is_null($a)) {
  if (f$is_null(v$a)) {
    //11:         $array[] = 200;
    (v$array).push_back (200_i64);
    //12:         $array[] = 12;
    (v$array).push_back (12_i64);
  };
  //13:     }
  //14:
  //15:     if (rand() % 2 == 0) {
  if (true) {
    //16:         echo $array[0];
    {
      f$echo(ctx, f$strval (v$array.get_value (0_i64)));
    };
    //17:     } else {
    //18:         echo $array[1];
  } else {
    {
      f$echo(ctx, f$strval (v$array.get_value (1_i64)));
    };
  };
  return ;
}


