#pragma once

#include <stddef.h>

#include "vkext/vk_zend.h"


PHP_FUNCTION (vk_stats_hll_merge);
PHP_FUNCTION (vk_stats_hll_count);
PHP_FUNCTION (vk_stats_hll_create);
PHP_FUNCTION (vk_stats_hll_add);
PHP_FUNCTION (vk_stats_hll_pack);
PHP_FUNCTION (vk_stats_hll_unpack);
PHP_FUNCTION (vk_stats_hll_is_packed);
