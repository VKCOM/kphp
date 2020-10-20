#ifndef __VKEXT_JSON_H__
#define __VKEXT_JSON_H__

#include <stdbool.h>

#include "vkext/vk_zend.h"

bool vk_json_encode(zval *val TSRMLS_DC);

#endif
