/* Generic USB boot keyboard/mouse host for ArcadeTab5. HID usage codes are
 * published, not translated to X68000 or MAME key IDs. Generic gamepad report
 * descriptors require per-device parsing and are deliberately not guessed. */
#include "tab5_platform.h"
#include <stdint.h>
#include <string.h>
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "usb/usb_host.h"
#include "usb/hid_host.h"
static const char *TAG="ArcadeUSB";
extern "C" int t5p_internal_input_push(const t5p_input_event_t *ev);
static QueueHandle_t s_events=nullptr;
static volatile esp_err_t s_install_state=ESP_ERR_INVALID_STATE;
static hid_host_device_handle_t s_keyboard=nullptr,s_mouse=nullptr;
static uint8_t s_previous[6]={};
static uint8_t s_modifiers=0;
static void post(t5p_input_type_t type,uint32_t code,int value,int x=0,int y=0) {
    t5p_input_event_t ev={};
    ev.type=type;ev.device_id=2;ev.code=code;ev.value=value;ev.x=x;ev.y=y;
    (void)t5p_internal_input_push(&ev);
}
static bool contains(const uint8_t arr[6],uint8_t key) {
    for(int i=0;i<6;i++)if(arr[i]==key)return true;
    return false;
}
static void keyboard_report(const uint8_t *raw,size_t len){
    if(len<8)return;
    const uint8_t *keys=raw+2;
    for(int i=0;i<6;i++)if(s_previous[i] && !contains(keys,s_previous[i]))
        post(T5P_INPUT_KEY_UP,s_previous[i],0);
    for(int i=0;i<6;i++)if(keys[i] && keys[i]!=1 && !contains(s_previous,keys[i]))
        post(T5P_INPUT_KEY_DOWN,keys[i],1);
    for(int i=0;i<8;i++) {
        uint8_t bit=(uint8_t)(1u<<i);
        if((s_modifiers^raw[0])&bit) {
            const bool down=(raw[0]&bit)!=0;
            post(down?T5P_INPUT_KEY_DOWN:T5P_INPUT_KEY_UP,0xe0u+(uint32_t)i,down);
        }
    }
    s_modifiers=raw[0];memcpy(s_previous,keys,6);
}
static void hid_interface_event(hid_host_device_handle_t dev,
                                hid_host_interface_event_t event,void *) {
    if(event==HID_HOST_INTERFACE_EVENT_INPUT_REPORT){
        uint8_t data[64]={};size_t n=0;
        if(hid_host_device_get_raw_input_report_data(dev,data,sizeof(data),&n)!=ESP_OK)return;
        if(dev==s_keyboard) keyboard_report(data,n);
        else if(dev==s_mouse && n>=3) {
            post(T5P_INPUT_AXIS,0,(int8_t)data[1]);
            post(T5P_INPUT_AXIS,1,(int8_t)data[2]);
            post(T5P_INPUT_BUTTON_DOWN,0,data[0]&1);
        }
    } else if(event==HID_HOST_INTERFACE_EVENT_DISCONNECTED){
        if(dev==s_keyboard){
            for(int i=0;i<6;i++)if(s_previous[i])post(T5P_INPUT_KEY_UP,s_previous[i],0);
            memset(s_previous,0,sizeof(s_previous));s_modifiers=0;s_keyboard=nullptr;
        } else if(dev==s_mouse)s_mouse=nullptr;
        (void)hid_host_device_close(dev);
    }
}
static void hid_connected(hid_host_device_handle_t dev,
                          hid_host_driver_event_t event,void *) {
    if(event==HID_HOST_DRIVER_EVENT_CONNECTED && s_events)
        (void)xQueueSend(s_events,&dev,0);
}
static void usb_control_task(void *){
    hid_host_device_handle_t dev;
    while(true){
        if(xQueueReceive(s_events,&dev,portMAX_DELAY)!=pdTRUE)continue;
        hid_host_dev_params_t p={};
        if(hid_host_device_get_params(dev,&p)!=ESP_OK)continue;
        if((p.proto!=HID_PROTOCOL_KEYBOARD && p.proto!=HID_PROTOCOL_MOUSE) ||
           p.sub_class!=HID_SUBCLASS_BOOT_INTERFACE)continue;
        if(p.proto==HID_PROTOCOL_KEYBOARD && s_keyboard)continue;
        if(p.proto==HID_PROTOCOL_MOUSE && s_mouse)continue;
        hid_host_device_config_t cfg={};
        cfg.callback=hid_interface_event;
        if(hid_host_device_open(dev,&cfg)!=ESP_OK)continue;
        if(hid_class_request_set_protocol(dev,HID_REPORT_PROTOCOL_BOOT)!=ESP_OK){
            (void)hid_host_device_close(dev);continue;
        }
        (void)hid_class_request_set_idle(dev,0,0);
        if(hid_host_device_start(dev)!=ESP_OK){
            (void)hid_host_device_close(dev);continue;
        }
        if(p.proto==HID_PROTOCOL_KEYBOARD)s_keyboard=dev;
        else s_mouse=dev;
        ESP_LOGI(TAG,"USB boot HID connected: %s",p.proto==HID_PROTOCOL_KEYBOARD?"keyboard":"mouse");
    }
}
static void usb_library_task(void *starter){
    usb_host_config_t cfg={};
    cfg.skip_phy_setup=false;
    cfg.intr_flags=ESP_INTR_FLAG_LEVEL1;
    s_install_state=usb_host_install(&cfg);
    xTaskNotifyGive((TaskHandle_t)starter);
    if(s_install_state!=ESP_OK){vTaskDelete(nullptr);return;}
    while(true){
        uint32_t flags=0;
        if(usb_host_lib_handle_events(portMAX_DELAY,&flags)!=ESP_OK){
            vTaskDelay(pdMS_TO_TICKS(20));continue;
        }
        if(flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS)
            (void)usb_host_device_free_all();
    }
}
extern "C" int t5p_input_usb_start(void){
    if(s_events)return T5P_OK;
    s_events=xQueueCreate(12,sizeof(hid_host_device_handle_t));
    if(!s_events)return T5P_ERR_NO_MEMORY;
    TaskHandle_t task=nullptr;
    if(xTaskCreatePinnedToCore(usb_library_task,"arcade_usb_lib",4096,
                               xTaskGetCurrentTaskHandle(),3,&task,0)!=pdPASS)
        return T5P_ERR_NO_MEMORY;
    if(ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(3000))==0 || s_install_state!=ESP_OK){
        ESP_LOGW(TAG,"USB Host start failed");return T5P_ERR_IO;
    }
    if(xTaskCreatePinnedToCore(usb_control_task,"arcade_usb_ctl",4096,
                               nullptr,3,nullptr,0)!=pdPASS)return T5P_ERR_NO_MEMORY;
    hid_host_driver_config_t cfg={};
    cfg.create_background_task=true;
    cfg.task_priority=3;cfg.stack_size=4096;cfg.core_id=0;
    cfg.callback=hid_connected;
    if(hid_host_install(&cfg)!=ESP_OK)return T5P_ERR_IO;
    ESP_LOGI(TAG,"USB boot keyboard/mouse active; generic gamepad pending driver mapping");
    return T5P_OK;
}
