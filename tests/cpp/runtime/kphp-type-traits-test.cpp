#include <gtest/gtest.h>

#include "runtime/kphp_core.h"
#include "runtime/kphp_type_traits.h"
#include "runtime/refcountable_php_classes.h"

struct Stub : refcountable_php_classes<Stub> {};

TEST(kphp_type_traits_test, test_is_class_instance_inside) {
  static_assert(!is_class_instance_inside<int64_t>{}, "except false");
  static_assert(!is_class_instance_inside<mixed>{}, "except false");
  static_assert(!is_class_instance_inside<string>{}, "except false");
  static_assert(!is_class_instance_inside<array<bool>>{}, "except false");
  static_assert(!is_class_instance_inside<array<array<mixed>>>{}, "except false");
  static_assert(!is_class_instance_inside<Optional<int64_t>>{}, "except false");
  static_assert(!is_class_instance_inside<std::tuple<int64_t, string>>{}, "except false");
  static_assert(!is_class_instance_inside<std::tuple<int64_t, string, Optional<array<mixed>>>>{}, "except false");
  static_assert(!is_class_instance_inside<std::tuple<shape<std::index_sequence<1, 2>, string, mixed>, string, Optional<array<mixed>>>>{}, "except false");

  static_assert(is_class_instance_inside<class_instance<Stub>>{}, "except true");
  static_assert(is_class_instance_inside<array<class_instance<Stub>>>{}, "except true");
  static_assert(is_class_instance_inside<Optional<array<class_instance<Stub>>>>{}, "except true");
  static_assert(is_class_instance_inside<Optional<array<Optional<class_instance<Stub>>>>>{}, "except true");
  static_assert(is_class_instance_inside<array<array<class_instance<Stub>>>>{}, "except true");
  static_assert(is_class_instance_inside<array<Optional<array<class_instance<Stub>>>>>{}, "except true");

  static_assert(is_class_instance_inside<Optional<class_instance<Stub>>>{}, "except true");
  static_assert(is_class_instance_inside<Optional<Optional<class_instance<Stub>>>>{}, "except true");

  static_assert(is_class_instance_inside<std::tuple<class_instance<Stub>>>{}, "except true");
  static_assert(is_class_instance_inside<std::tuple<string, class_instance<Stub>>>{}, "except true");
  static_assert(is_class_instance_inside<std::tuple<mixed, class_instance<Stub>>>{}, "except true");
  static_assert(is_class_instance_inside<std::tuple<class_instance<Stub>, class_instance<Stub>>>{}, "except true");

  static_assert(is_class_instance_inside<std::tuple<Optional<class_instance<Stub>>>>{}, "except true");
  static_assert(is_class_instance_inside<Optional<std::tuple<class_instance<Stub>>>>{}, "except true");

  static_assert(is_class_instance_inside<std::tuple<array<class_instance<Stub>>>>{}, "except true");
  static_assert(is_class_instance_inside<array<std::tuple<class_instance<Stub>>>>{}, "except true");
  static_assert(is_class_instance_inside<array<Optional<std::tuple<class_instance<Stub>>>>>{}, "except true");
  static_assert(is_class_instance_inside<std::tuple<array<string>, class_instance<Stub>>>{}, "except true");

  static_assert(is_class_instance_inside<std::tuple<std::tuple<class_instance<Stub>>>>{}, "except true");
  static_assert(is_class_instance_inside<std::tuple<std::tuple<std::tuple<class_instance<Stub>>>>>{}, "except true");
  static_assert(is_class_instance_inside<std::tuple<array<std::tuple<std::tuple<class_instance<Stub>>>>>>{}, "except true");

  static_assert(is_class_instance_inside<shape<std::index_sequence<1>, class_instance<Stub>>>{}, "except true");
  static_assert(is_class_instance_inside<shape<std::index_sequence<1, 2>, string, class_instance<Stub>>>{}, "except true");
  static_assert(is_class_instance_inside<shape<std::index_sequence<1>, array<class_instance<Stub>>>>{}, "except true");
  static_assert(is_class_instance_inside<shape<std::index_sequence<1>, Optional<array<class_instance<Stub>>>>>{}, "except true");
  static_assert(is_class_instance_inside<shape<std::index_sequence<1>, std::tuple<class_instance<Stub>>>>{}, "except true");
  static_assert(is_class_instance_inside<shape<std::index_sequence<1>, array<std::tuple<class_instance<Stub>>>>>{}, "except true");
  static_assert(is_class_instance_inside<shape<std::index_sequence<1>, shape<std::index_sequence<1>, class_instance<Stub>>>>{}, "except true");

  static_assert(is_class_instance_inside<std::tuple<shape<std::index_sequence<1>, class_instance<Stub>>>>{}, "except true");
  static_assert(is_class_instance_inside<std::tuple<Optional<shape<std::index_sequence<1>, class_instance<Stub>>>>>{}, "except true");
  static_assert(is_class_instance_inside<std::tuple<array<shape<std::index_sequence<1>, class_instance<Stub>>>>>{}, "except true");
  static_assert(is_class_instance_inside<std::tuple<shape<std::index_sequence<1>, std::tuple<class_instance<Stub>>>>>{}, "except true");
  static_assert(is_class_instance_inside<
                  std::tuple<string, shape<std::index_sequence<1>, Optional<std::tuple<array<shape<std::index_sequence<1>, class_instance<Stub>>>>>>>>{},
                "except true");
}
