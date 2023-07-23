#pragma GCC diagnostic push
#include <nk/transport.h>
#include <nk/types.h>
#ifndef ____Einit__COMPONENT_ENDPOINTS__
#define ____Einit__COMPONENT_ENDPOINTS__
enum {
    Einit_iidMax,
};
enum {
    Einit_iidOffset = 0,
};
enum {
    Einit_securityIidMax,
};
enum {
    Einit_component_req_arena_size = 0,
    Einit_component_res_arena_size = 0,
    Einit_component_arena_size = 0,
    Einit_component_req_handles = 0,
    Einit_component_res_handles = 0,
    Einit_component_err_handles = 0,
};
#endif /* ____Einit__COMPONENT_ENDPOINTS__ */

#ifndef ____Einit__TASK_ENDPOINTS__
#define ____Einit__TASK_ENDPOINTS__
enum {
    Einit_entity_req_arena_size =
    Einit_component_req_arena_size,
    Einit_entity_res_arena_size =
    Einit_component_res_arena_size,
    Einit_entity_arena_size =
    Einit_component_arena_size,
    Einit_entity_req_handles =
    Einit_component_req_handles,
    Einit_entity_res_handles =
    Einit_component_res_handles,
    Einit_entity_err_handles =
    Einit_component_err_handles,
};
#endif /* ____Einit__TASK_ENDPOINTS__ */

#pragma GCC diagnostic pop

