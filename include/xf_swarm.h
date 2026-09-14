#ifndef XF_SWARM_H
#define XF_SWARM_H
#include <stdint.h>
#if defined(_WIN32)
# if defined(XF_BUILD)
#  define XF_API __declspec(dllexport)
# else
#  define XF_API __declspec(dllimport)
# endif
# define XF_CALL __cdecl
#else
# define XF_API __attribute__((visibility("default")))
# define XF_CALL
#endif
#ifdef __cplusplus
extern "C" {
#endif

#define XF_ABI_VERSION 1u
#define XF_ID_SIZE 64u
#define XF_MAX_MEMBERS 128u
typedef int32_t xf_result;
#define XF_OK 0
#define XF_INVALID_ARGUMENT 1
#define XF_NOT_READY 2
#define XF_BUFFER_TOO_SMALL 3
#define XF_STALE 4
#define XF_CAPACITY 5
#define XF_STOPPED 6
#define XF_VENDOR_ERROR 7
#define XF_NO_MEMORY 8
#define XF_INTERNAL_ERROR 9
typedef struct xf_swarm xf_swarm;
typedef struct { double x, y, z; } xf_vec3;
typedef struct { double x, y, z, w; } xf_quaternion;
typedef struct { xf_vec3 position; double yaw; } xf_leader_pose;
typedef struct {
    uint32_t struct_size;
    uint32_t abi_version;
    double control_period_sec;
    double max_speed_mps;
    double max_vertical_speed_mps;
    double max_yaw_rate_rps;
    double separation_m;
    double telemetry_timeout_sec;
} xf_config;
typedef struct {
    char id[XF_ID_SIZE];
    xf_vec3 offset;
    uint32_t local;
} xf_member;
typedef struct {
    char id[XF_ID_SIZE];
    double capture_time_sec;
    xf_vec3 position;
    xf_quaternion orientation;
    xf_vec3 velocity;
    double confidence;
    uint32_t tracking_valid;
    uint32_t velocity_valid;
} xf_vehicle_state;
#define XF_DIAG_OK 0u
#define XF_DIAG_MISSING_POSE 1u
#define XF_DIAG_MISSING_SLOT 2u
#define XF_DIAG_INCOMPLETE_TELEMETRY 3u
#define XF_DIAG_INVALID_INPUT 4u
#define XF_DIAG_STOPPED 5u
#define XF_DIAG_NOT_READY 6u
typedef struct {
    char id[XF_ID_SIZE];
    xf_vec3 velocity;
    double yaw_rate;
    uint32_t diagnostic;
} xf_command;

XF_API uint32_t XF_CALL xf_swarm_abi_version(void);
XF_API xf_result XF_CALL xf_swarm_default_config(xf_config* out);
XF_API xf_result XF_CALL xf_swarm_create(const xf_config* config, xf_swarm** out);
XF_API void XF_CALL xf_swarm_destroy(xf_swarm* handle);
XF_API xf_result XF_CALL xf_swarm_set_formation(xf_swarm* handle, const xf_member* members, uint32_t count);
XF_API xf_result XF_CALL xf_swarm_set_leader(xf_swarm* handle, const xf_leader_pose* current);
XF_API xf_result XF_CALL xf_swarm_set_goal(xf_swarm* handle, const xf_leader_pose* goal);
XF_API xf_result XF_CALL xf_swarm_push_state(xf_swarm* handle, const xf_vehicle_state* state);
XF_API xf_result XF_CALL xf_swarm_step(xf_swarm* handle, double source_now_sec, xf_command* out, uint32_t capacity, uint32_t* count);
XF_API xf_result XF_CALL xf_swarm_stop(xf_swarm* handle);
XF_API xf_result XF_CALL xf_swarm_reset(xf_swarm* handle);

#ifdef __cplusplus
}
#endif
#endif
