#ifndef XF_EOTS_H
#define XF_EOTS_H
#include "xf_swarm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct xf_eots xf_eots;
#define XF_EOTS_TRACK 0u
#define XF_EOTS_CLEAR 1u
#define XF_EOTS_STOP 2u
#define XF_EOTS_SOT 0u
#define XF_EOTS_MOT 1u
#define XF_EOTS_APPEARED 0u
#define XF_EOTS_UPDATED 1u
#define XF_EOTS_LOCKED 2u
#define XF_EOTS_LOST 3u
#define XF_EOTS_RECOVERED 4u
#define XF_EOTS_RESET 5u
#define XF_EOTS_OVERRIDDEN 6u
#define XF_EOTS_STATE_IDLE 0u
#define XF_EOTS_STATE_SEARCHING 1u
#define XF_EOTS_STATE_TRACKING 2u
#define XF_EOTS_STATE_LOST 3u
#define XF_EOTS_STATE_RECOVERING 4u
#define XF_EOTS_STATE_OVERRIDDEN 5u
typedef xf_result (XF_CALL *xf_eots_command_handler)(void* user, uint32_t action, uint32_t mode);
typedef struct {
    int32_t track_id, class_id;
    char label[32];
    double confidence;
    int32_t x, y, width, height;
    double center_offset_x, center_offset_y;
    double velocity_x, velocity_y;
    uint32_t locked;
} xf_eots_target;
typedef struct {
    int32_t width, height;
    int64_t capture_ms;
    int32_t seq;
} xf_eots_frame;
typedef struct {
    uint32_t event, has_primary;
    xf_eots_target primary;
    xf_eots_frame frame;
    double fps;
} xf_eots_observation;
typedef struct {
    uint32_t present, enabled, mode, state;
    uint32_t has_primary, has_frame, has_fps;
    double fps;
    xf_eots_target primary;
    xf_eots_frame frame;
} xf_eots_snapshot;
XF_API uint32_t XF_CALL xf_eots_abi_version(void);
XF_API xf_result XF_CALL xf_eots_create(xf_eots_command_handler handler, void* user, xf_eots** out);
XF_API void XF_CALL xf_eots_destroy(xf_eots* handle);
XF_API xf_result XF_CALL xf_eots_command(xf_eots* handle, uint32_t action, uint32_t mode);
XF_API xf_result XF_CALL xf_eots_feed(xf_eots* handle, const xf_eots_observation* observation);
XF_API xf_result XF_CALL xf_eots_read(xf_eots* handle, xf_eots_snapshot* out);
#ifdef __cplusplus
}
#endif
#endif
