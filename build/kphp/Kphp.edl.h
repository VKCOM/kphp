#pragma GCC diagnostic push
#include <nk/transport.h>
#include <nk/types.h>
#ifndef __kphp__Kphp__COMPONENT_ENDPOINTS__
#define __kphp__Kphp__COMPONENT_ENDPOINTS__
enum {
    kphp_Kphp_iidMax,
};
enum {
    kphp_Kphp_iidOffset = 0,
};
enum {
    kphp_Kphp_securityIidMax,
};
enum {
    kphp_Kphp_component_req_arena_size = 0,
    kphp_Kphp_component_res_arena_size = 0,
    kphp_Kphp_component_arena_size = 0,
    kphp_Kphp_component_req_handles = 0,
    kphp_Kphp_component_res_handles = 0,
    kphp_Kphp_component_err_handles = 0,
};
#ifdef NK_USE_UNQUALIFIED_NAMES
#define Kphp_iidMax kphp_Kphp_iidMax
#define Kphp_iidOffset kphp_Kphp_iidOffset
#define Kphp_securityIidMax kphp_Kphp_securityIidMax
#define Kphp_component_req_arena_size kphp_Kphp_component_req_arena_size
#define Kphp_component_res_arena_size kphp_Kphp_component_res_arena_size
#define Kphp_component_arena_size kphp_Kphp_component_arena_size
#define Kphp_component_req_handles kphp_Kphp_component_req_handles
#define Kphp_component_res_handles kphp_Kphp_component_res_handles
#define Kphp_component_err_handles kphp_Kphp_component_err_handles
#endif /* NK_USE_UNQUALIFIED_NAMES */
#endif /* __kphp__Kphp__COMPONENT_ENDPOINTS__ */

#ifndef __kphp__Kphp__TASK_ENDPOINTS__
#define __kphp__Kphp__TASK_ENDPOINTS__
enum {
    kphp_Kphp_entity_req_arena_size =
    kphp_Kphp_component_req_arena_size,
    kphp_Kphp_entity_res_arena_size =
    kphp_Kphp_component_res_arena_size,
    kphp_Kphp_entity_arena_size =
    kphp_Kphp_component_arena_size,
    kphp_Kphp_entity_req_handles =
    kphp_Kphp_component_req_handles,
    kphp_Kphp_entity_res_handles =
    kphp_Kphp_component_res_handles,
    kphp_Kphp_entity_err_handles =
    kphp_Kphp_component_err_handles,
};
#ifdef NK_USE_UNQUALIFIED_NAMES
#define Kphp_entity_req_arena_size kphp_Kphp_entity_req_arena_size
#define Kphp_entity_res_arena_size kphp_Kphp_entity_res_arena_size
#define Kphp_entity_arena_size kphp_Kphp_entity_arena_size
#define Kphp_entity_req_handles kphp_Kphp_entity_req_handles
#define Kphp_entity_res_handles kphp_Kphp_entity_res_handles
#define Kphp_entity_err_handles kphp_Kphp_entity_err_handles
#endif /* NK_USE_UNQUALIFIED_NAMES */
#endif /* __kphp__Kphp__TASK_ENDPOINTS__ */

#pragma GCC diagnostic pop

