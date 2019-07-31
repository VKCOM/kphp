#include <cassert>

#include "runtime/storage.h"
#include "runtime/tl/rpc_response.h"

template<> int Storage::tagger<bool>::get_tag() { return 0; }
template<> int Storage::tagger<int>::get_tag() { return 0; }
template<> int Storage::tagger<OrFalse<int>>::get_tag() { return 0; }
template<> int Storage::tagger<void>::get_tag() { return 0; }
template<> int Storage::tagger<thrown_exception>::get_tag() { return 0; }
template<> int Storage::tagger<var>::get_tag() { return 0; }
template<> int Storage::tagger<array<var>>::get_tag() { return 0; }
template<> int Storage::tagger<OrFalse<string>>::get_tag() { return 0; }
template<> int Storage::tagger<OrFalse<array<var>>>::get_tag() { return 0; }
template<> int Storage::tagger<array<array<var>>>::get_tag() { return 0; }
template<> int Storage::tagger<class_instance<C$VK$TL$RpcResponse>>::get_tag() { return 0; }
template<> int Storage::tagger<array<class_instance<C$VK$TL$RpcResponse>>>::get_tag() { return 0; }
template<> Storage::loader<var>::loader_fun Storage::loader<var>::get_function(int) { return nullptr; }

void init_php_scripts() {
  assert(0 && "this code shouldn't be executed and only for linkage test");
}
void global_init_php_scripts() {
  assert(0 && "this code shouldn't be executed and only for linkage test");
}
