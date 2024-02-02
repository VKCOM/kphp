//crc64:d2d22fdea2baa6fc
//crc64_with_comments:30c2cbcea2072215
#include "runtime-headers.h"
#include "src_dev2ecd55ec1895a080.h"
#include "demo.h"


//source = [dev.php]
//3: function demo() {
Optional < bool > f$src_dev2ecd55ec1895a080(script_context_t * ctx) noexcept  {
//4:     $x = 10;
//5:     $d = 10 * $x / 4.5 + $x * 100 * 2.5;
//6:     $r = 200 * $d / 100;
//7:     return $r;
//8: }
//9: 
//10: demo();
  f$demo(ctx);
  return Optional<bool>{};
}

void run_script(script_context_t * ctx) noexcept {
  f$src_dev2ecd55ec1895a080(ctx);
}



