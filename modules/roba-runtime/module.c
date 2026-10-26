// SPDX-License-Identifier: MIT
// Uses ZMK behavior/Custom RPC APIs; no changes to HID, BLE, keymap or PMW drivers.
#define DT_DRV_COMPAT roba_dynamic_macro
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/workqueue.h>
#include <zmk/studio/custom.h>
#include <pb_encode.h>
#include <string.h>
#include <errno.h>
#include "core.h"
LOG_MODULE_DECLARE(zmk,CONFIG_ZMK_LOG_LEVEL);
static rs_store store;
K_MUTEX_DEFINE(store_mutex);
K_MUTEX_DEFINE(player_mutex);
static rs_record playing;
static struct zmk_behavior_binding_event play_event;
static unsigned play_index;
static bool playing_busy, key_down;
static void play_work(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(player,play_work);
static void play_work(struct k_work *work) {
    ARG_UNUSED(work);
    k_mutex_lock(&player_mutex,K_FOREVER);
    uint8_t *b=playing.b;
    struct zmk_behavior_binding kp={.behavior_dev=DEVICE_DT_NAME(DT_NODELABEL(kp)),.param1=rs_key(b[36+play_index],b[28]),.param2=0};
    play_event.timestamp=k_uptime_get();
    if(!key_down) {
        int rc=zmk_behavior_invoke_binding(&kp,play_event,true);
        if(rc<0) { zmk_behavior_invoke_binding(&kp,play_event,false);playing_busy=false;k_mutex_unlock(&player_mutex);return; }
        key_down=true;
        k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(),&player,K_MSEC(rs_u16(b+34)));
    } else {
        zmk_behavior_invoke_binding(&kp,play_event,false);
        key_down=false;
        if(++play_index>=b[30])playing_busy=false;
        else k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(),&player,K_MSEC(rs_u16(b+32)));
    }
    k_mutex_unlock(&player_mutex);
}
static int pressed(struct zmk_behavior_binding *binding,struct zmk_behavior_binding_event event) {
    if(binding->param1>=RS_SLOTS)return ZMK_BEHAVIOR_OPAQUE;
    k_mutex_lock(&player_mutex,K_FOREVER);
    if(!playing_busy) {
        k_mutex_lock(&store_mutex,K_FOREVER);
        rs_record *r=&store.records[binding->param1];
        if(rs_valid(r,0)&&!r->b[31]&&binding->param2==rs_u32(r->b+8)) {
            playing=*r;play_event=event;play_index=0;key_down=false;playing_busy=true;
            k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(),&player,K_NO_WAIT);
        }
        k_mutex_unlock(&store_mutex);
    }
    k_mutex_unlock(&player_mutex);
    return ZMK_BEHAVIOR_OPAQUE;
}
static int released(struct zmk_behavior_binding *binding,struct zmk_behavior_binding_event event) { return ZMK_BEHAVIOR_OPAQUE; }
static const struct behavior_parameter_value_metadata slots[]={ {.display_name="Slot",.type=BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,.range={.min=0,.max=RS_SLOTS-1}} };
static const struct behavior_parameter_value_metadata tokens[]={ {.display_name="Owner token",.type=BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE,.range={.min=1,.max=0x7fffffff}} };
static const struct behavior_parameter_metadata_set meta_set={.param1_values=slots,.param1_values_len=1,.param2_values=tokens,.param2_values_len=1};
static const struct behavior_parameter_metadata metadata={.sets=&meta_set,.sets_len=1};
static const struct behavior_driver_api api={.binding_pressed=pressed,.binding_released=released,.parameter_metadata=&metadata};
static int behavior_init(const struct device *dev) { return 0; }
#define RS_INST(n) BEHAVIOR_DT_INST_DEFINE(n,behavior_init,NULL,NULL,NULL,POST_KERNEL,CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,&api);
DT_INST_FOREACH_STATUS_OKAY(RS_INST)
static int init_store(void) { rs_init(&store);return 0; }
SYS_INIT(init_store,APPLICATION,CONFIG_APPLICATION_INIT_PRIORITY);
static int settings_set(const char *name,size_t len,settings_read_cb read_cb,void *cb_arg) {
    if(!name||strlen(name)!=3||name[0]<'0'||name[0]>'7'||name[1]!='/'||name[2]<'0'||name[2]>'1'||len!=RS_SIZE)return -EINVAL;
    rs_record record;
    if(read_cb(cb_arg,&record,sizeof(record))!=sizeof(record))return -EIO;
    // A damaged newest bank is ignored; the previous CRC-valid bank remains usable.
    if(!rs_valid(&record,0))return 0;
    return rs_load(&store,name[0]-'0',name[2]-'0',&record);
}
SETTINGS_STATIC_HANDLER_DEFINE(roba_runtime,"roba_runtime_v1",NULL,settings_set,NULL,NULL);
static int persist(unsigned slot,unsigned bank,const rs_record *r,void *ctx) {
    char key[32];snprintk(key,sizeof(key),"roba_runtime_v1/%u/%u",slot,bank);
    return settings_save_one(key,r,sizeof(*r));
}
static uint8_t response[8+RS_SIZE];
static size_t response_len;
static bool encode_response(pb_ostream_t *stream,const pb_field_t *field,void *const *arg) {
    return pb_encode_tag_for_field(stream,field)&&pb_encode_string(stream,response,response_len);
}
static bool handle_request(const zmk_custom_CallRequest *raw,pb_callback_t *out) {
    const uint8_t *q=raw->payload.bytes;size_t n=raw->payload.size;
    memset(response,0,sizeof(response));response[0]='R';response[1]='S';response[2]=1;response[3]=0x80;response_len=8;
    out->funcs.encode=encode_response;out->arg=NULL;
    if(n<8||q[0]!='R'||q[1]!='S'||q[2]!=1||q[3]>2||q[5]||q[6]||q[7]){response[4]=1;return true;}
    unsigned op=q[3],slot=q[4];response[3]=op|0x80;response[5]=slot;
    if(op==0) {
        if(n!=8||slot){response[4]=1;return true;}
        uint8_t info[]={RS_SLOTS,RS_TEXT,63,1,RS_SIZE,0,31,0};memcpy(response+8,info,sizeof(info));response_len+=sizeof(info);return true;
    }
    if(slot>=RS_SLOTS||n!=(op==1?8:12+RS_SIZE)){response[4]=1;return true;}
    k_mutex_lock(&store_mutex,K_FOREVER);
    if(op==2) {
        rs_record next;memcpy(&next,q+12,RS_SIZE);
        int rc=rs_commit(&store,slot,rs_u32(q+8),&next,persist,NULL);
        if(rc){response[4]=rc==-ESTALE?2:rc==-EINVAL?1:rc==-EBUSY||rc==-EEXIST?4:3;k_mutex_unlock(&store_mutex);return true;}
    }
    memcpy(response+8,&store.records[slot],RS_SIZE);response_len+=RS_SIZE;
    k_mutex_unlock(&store_mutex);return true;
}
static struct zmk_rpc_custom_subsystem_meta rpc_meta={.security=ZMK_STUDIO_RPC_HANDLER_SECURED};
ZMK_RPC_CUSTOM_SUBSYSTEM(roba__runtime_macro_v1,&rpc_meta,handle_request);
