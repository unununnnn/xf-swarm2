#include "xf_swarm.h"
#include "xf_eots.h"
#include <stdio.h>
#include <string.h>

static xf_result XF_CALL demo_vendor(void* user, uint32_t action, uint32_t mode) {
    (void)user;
    printf("demo EOTS action=%u mode=%u\n",(unsigned)action,(unsigned)mode);
    return XF_OK; /* Replace with actual vendor acceptance. */
}
int main(void) {
    xf_swarm* swarm=NULL;
    xf_eots* eots=NULL;
    xf_config config;
    xf_member member={0};
    xf_vehicle_state state={0};
    xf_leader_pose leader={{5,0,3},0};
    xf_leader_pose goal={{10,0,3},0};
    xf_command output[XF_MAX_MEMBERS];
    uint32_t count=0;
    xf_eots_observation observation={0};
    xf_eots_snapshot snapshot;
    int result=1;

    if(xf_swarm_abi_version()!=XF_ABI_VERSION || xf_eots_abi_version()!=XF_ABI_VERSION) return 2;
    if(xf_swarm_default_config(&config)!=XF_OK || xf_swarm_create(&config,&swarm)!=XF_OK) goto cleanup;
    strcpy(member.id,"uav0");member.local=1;
    if(xf_swarm_set_formation(swarm,&member,1)!=XF_OK) goto cleanup;
    if(xf_swarm_set_leader(swarm,&leader)!=XF_OK || xf_swarm_set_goal(swarm,&goal)!=XF_OK) goto cleanup;

    strcpy(state.id,"uav0");state.capture_time_sec=42.0;
    state.position.z=3;state.orientation.w=1;
    state.confidence=1;state.tracking_valid=1;state.velocity_valid=1;
    /* This demo supplies a measured stationary vehicle; use actual telemetry. */
    if(xf_swarm_push_state(swarm,&state)!=XF_OK) goto cleanup;
    if(xf_swarm_step(swarm,42.0,output,XF_MAX_MEMBERS,&count)!=XF_OK) goto cleanup;
    if(count!=1 || output[0].diagnostic!=XF_DIAG_OK) goto cleanup;
    printf("%s velocity=(%.3f,%.3f,%.3f), yaw_rate=%.3f\n",output[0].id,
        output[0].velocity.x,output[0].velocity.y,output[0].velocity.z,output[0].yaw_rate);

    if(xf_eots_create(demo_vendor,NULL,&eots)!=XF_OK) goto cleanup;
    if(xf_eots_command(eots,XF_EOTS_TRACK,XF_EOTS_SOT)!=XF_OK) goto cleanup;
    observation.event=XF_EOTS_LOCKED;observation.has_primary=1;
    observation.primary.track_id=7;strcpy(observation.primary.label,"marker");
    observation.primary.confidence=0.9;
    observation.primary.x=100;observation.primary.y=80;
    observation.primary.width=120;observation.primary.height=140;
    observation.primary.center_offset_x=-0.5;observation.primary.center_offset_y=-0.375;
    observation.frame.width=640;observation.frame.height=480;
    observation.frame.capture_ms=42000;observation.frame.seq=1;observation.fps=30;
    if(xf_eots_feed(eots,&observation)!=XF_OK || xf_eots_read(eots,&snapshot)!=XF_OK) goto cleanup;
    printf("EOTS state=%u target=%d\n",(unsigned)snapshot.state,snapshot.primary.track_id);
    result=0;
cleanup:
    if(eots) (void)xf_eots_command(eots,XF_EOTS_STOP,XF_EOTS_SOT);
    /* For a real vendor, stop/unregister and join callbacks before destroy. */
    xf_eots_destroy(eots);
    if(swarm) (void)xf_swarm_stop(swarm);
    xf_swarm_destroy(swarm);
    return result;
}
