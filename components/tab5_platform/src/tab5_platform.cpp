/* ArcadeTab5 Platform R1 — standalone Tab5 hardware host.
 * Does not include, link or emulate PX68K, MAME, X68000 or any game ROM.
 */
#include "tab5_platform.h"
#include <M5Unified.h>
#include <M5GFX.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_vfs_fat.h>
#include <driver/sdmmc_host.h>
#include <driver/gpio.h>
#include <sdmmc_cmd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

static const char *TAG="ArcadePlatform";
static constexpr int kFrameCount=2;
static constexpr uint32_t kMaxFrameW=384, kMaxFrameH=384;
static constexpr int kStripeH=8;
static constexpr size_t kAudioRingFrames=4096;
static constexpr size_t kAudioChunkFrames=256;
static constexpr int kSpeakerSlots=3;
static constexpr uint8_t kSpeakerChannel=0;

struct Frame {
    uint16_t *data=nullptr;
    uint32_t w=0,h=0;
    t5p_rotation_t rotation=T5P_ROTATION_0;
    uint8_t state=0; // 0 free, 1 producer, 2 queued, 3 displaying
};
static Frame s_frames[kFrameCount];
static QueueHandle_t s_frame_queue=nullptr, s_input_queue=nullptr;
static SemaphoreHandle_t s_frame_mutex=nullptr, s_display_mutex=nullptr, s_audio_mutex=nullptr;
static TaskHandle_t s_render_task=nullptr, s_audio_task=nullptr;
static uint16_t *s_stripe=nullptr;
static int16_t *s_pcm=nullptr;
static size_t s_pcm_head=0,s_pcm_count=0;
static int16_t *s_play[kSpeakerSlots]={};
static volatile uint32_t s_play_busy[kSpeakerSlots]={};
static int s_play_cursor=0;
static bool s_ready=false, s_storage_ready=false, s_audio_ready=false;
static sdmmc_card_t *s_card=nullptr;

extern "C" int t5p_internal_input_push(const t5p_input_event_t *ev) {
    if (!ev || !s_input_queue) return 0;
    // Tiny non-blocking bounded event queue shared by touch and USB host.
    return xQueueSend(s_input_queue,ev,0)==pdTRUE;
}
extern "C" uint64_t t5p_time_us(void) { return (uint64_t)esp_timer_get_time(); }
extern "C" int t5p_input_poll(t5p_input_event_t *ev) {
    return ev && s_input_queue && xQueueReceive(s_input_queue,ev,0)==pdTRUE;
}

static void poll_touch(void) {
    static bool last=false;
    M5.update();
    const auto touch=M5.Touch.getDetail();
    const bool pressed=touch.isPressed();
    {
        static int xprev=-1,yprev=-1;
        if (pressed != last || (pressed && (touch.x!=xprev || touch.y!=yprev))) {
            t5p_input_event_t ev={};
            ev.type=T5P_INPUT_POINTER;ev.device_id=1;ev.code=0;
            ev.value=pressed?1:0;ev.x=touch.x;ev.y=touch.y;
            t5p_internal_input_push(&ev);
        }
        xprev=touch.x;yprev=touch.y;
    }
    last=pressed;
}
static void render_slot(int idx) {
    const Frame &f=s_frames[idx];
    const uint32_t rotatedW=(f.rotation&1)?f.h:f.w;
    const uint32_t rotatedH=(f.rotation&1)?f.w:f.h;
    uint32_t dw=0,dh=0;
    t5p_fit_rect(f.w,f.h,f.rotation,1200,680,&dw,&dh);
    if (!dw || !dh || !s_stripe) return;
    const int x0=(1280-(int)dw)/2, y0=(720-(int)dh)/2;
    xSemaphoreTake(s_display_mutex,portMAX_DELAY);
    static uint32_t prevw=0,prevh=0;
    if (prevw!=dw || prevh!=dh) { M5.Display.fillScreen(0);prevw=dw;prevh=dh; }
    for (uint32_t y=0;y<dh;y+=kStripeH) {
        uint32_t count=dh-y;
        if (count>kStripeH) count=kStripeH;
        for (uint32_t row=0;row<count;row++) {
            const uint32_t ry=(uint32_t)(((uint64_t)(y+row)*rotatedH)/dh);
            for (uint32_t x=0;x<dw;x++) {
                const uint32_t rx=(uint32_t)(((uint64_t)x*rotatedW)/dw);
                uint32_t sx=rx,sy=ry;
                switch (f.rotation) {
                    case T5P_ROTATION_90: sx=ry;sy=f.h-1-rx;break;
                    case T5P_ROTATION_180:sx=f.w-1-rx;sy=f.h-1-ry;break;
                    case T5P_ROTATION_270:sx=f.w-1-ry;sy=rx;break;
                    default:break;
                }
                s_stripe[row*dw+x]=f.data[sy*f.w+sx];
            }
        }
        M5.Display.pushImage(x0,y0+(int)y,(int)dw,(int)count,s_stripe);
    }
    xSemaphoreGive(s_display_mutex);
}
static void render_worker(void *) {
    TickType_t lastWake=xTaskGetTickCount();
    while (true) {
        int idx=-1;
        if (xQueueReceive(s_frame_queue,&idx,0)==pdTRUE && idx>=0 && idx<kFrameCount) {
            xSemaphoreTake(s_frame_mutex,portMAX_DELAY);
            if (s_frames[idx].state==2) s_frames[idx].state=3;
            else idx=-1;
            xSemaphoreGive(s_frame_mutex);
            if (idx>=0) {
                render_slot(idx);
                xSemaphoreTake(s_frame_mutex,portMAX_DELAY);
                s_frames[idx].state=0;
                xSemaphoreGive(s_frame_mutex);
            }
        }
        poll_touch();
        vTaskDelayUntil(&lastWake,pdMS_TO_TICKS(8));
    }
}
extern "C" int t5p_video_acquire(uint32_t w,uint32_t h,t5p_video_slot_t *out) {
    if (!s_ready || !out) return T5P_ERR_NOT_READY;
    if (!w || !h || w>kMaxFrameW || h>kMaxFrameH) return T5P_ERR_INVALID;
    if (xSemaphoreTake(s_frame_mutex,0)!=pdTRUE) return T5P_ERR_BUSY;
    int ret=T5P_ERR_BUSY;
    for (int i=0;i<kFrameCount;i++) if (s_frames[i].state==0) {
        s_frames[i].state=1;s_frames[i].w=w;s_frames[i].h=h;
        out->pixels=s_frames[i].data;out->width=w;out->height=h;
        out->stride_pixels=w;out->slot_token=(uint32_t)i+1;
        ret=T5P_OK;break;
    }
    xSemaphoreGive(s_frame_mutex);
    return ret;
}
extern "C" int t5p_video_present(t5p_video_slot_t *slot,t5p_rotation_t rotation) {
    if (!slot || !slot->slot_token || slot->slot_token>kFrameCount || rotation>T5P_ROTATION_270)
        return T5P_ERR_INVALID;
    const int idx=(int)slot->slot_token-1;
    if (xSemaphoreTake(s_frame_mutex,0)!=pdTRUE) return T5P_ERR_BUSY;
    if (s_frames[idx].state!=1 || s_frames[idx].data!=slot->pixels ||
        s_frames[idx].w!=slot->width || s_frames[idx].h!=slot->height) {
        xSemaphoreGive(s_frame_mutex);return T5P_ERR_INVALID;
    }
    s_frames[idx].rotation=rotation;s_frames[idx].state=2;
    if (xQueueSend(s_frame_queue,&idx,0)!=pdTRUE) {
        s_frames[idx].state=1;xSemaphoreGive(s_frame_mutex);return T5P_ERR_BUSY;
    }
    slot->pixels=nullptr;slot->slot_token=0;
    xSemaphoreGive(s_frame_mutex);
    return T5P_OK;
}
extern "C" void t5p_video_cancel(t5p_video_slot_t *slot) {
    if (!slot || !slot->slot_token || slot->slot_token>kFrameCount || !s_frame_mutex) return;
    if (xSemaphoreTake(s_frame_mutex,0)!=pdTRUE) return;
    const int i=(int)slot->slot_token-1;
    if (s_frames[i].state==1 && s_frames[i].data==slot->pixels) {
        s_frames[i].state=0;slot->pixels=nullptr;slot->slot_token=0;
    }
    xSemaphoreGive(s_frame_mutex);
}
extern "C" size_t t5p_audio_submit(const int16_t *samples,size_t frames) {
    if (!s_audio_ready || !samples || !s_pcm || !frames) return 0;
    if (xSemaphoreTake(s_audio_mutex,0)!=pdTRUE) return 0;
    size_t n=frames,free=kAudioRingFrames-s_pcm_count;
    if (n>free) n=free;
    const size_t tail=(s_pcm_head+s_pcm_count)%kAudioRingFrames;
    const size_t first=(n>kAudioRingFrames-tail)?kAudioRingFrames-tail:n;
    memcpy(s_pcm+2*tail,samples,first*4);
    if (n>first)memcpy(s_pcm,samples+2*first,(n-first)*4);
    s_pcm_count+=n;
    xSemaphoreGive(s_audio_mutex);
    if (n && s_audio_task) xTaskNotifyGive(s_audio_task);
    return n;
}
extern "C" size_t t5p_audio_queued_frames(void) {
    if (!s_audio_mutex || xSemaphoreTake(s_audio_mutex,0)!=pdTRUE) return 0;
    size_t n=s_pcm_count;xSemaphoreGive(s_audio_mutex);return n;
}
static void speaker_released(void *,const void *ptr,uint8_t channel) {
    if (channel!=kSpeakerChannel) return;
    for(int i=0;i<kSpeakerSlots;i++) if(ptr==s_play[i])
        __atomic_store_n(&s_play_busy[i],0u,__ATOMIC_RELEASE);
    if(s_audio_task)xTaskNotifyGive(s_audio_task);
}
static void audio_worker(void *) {
    while (true) {
        int id=-1;
        for(int k=0;k<kSpeakerSlots;k++) {
            int candidate=(s_play_cursor+k)%kSpeakerSlots;
            if(!__atomic_load_n(&s_play_busy[candidate],__ATOMIC_ACQUIRE)) {id=candidate;break;}
        }
        if(id<0 || xSemaphoreTake(s_audio_mutex,pdMS_TO_TICKS(1))!=pdTRUE) {
            ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(5));continue;
        }
        if(s_pcm_count<kAudioChunkFrames) {
            xSemaphoreGive(s_audio_mutex);
            ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(5));continue;
        }
        const size_t first=(kAudioChunkFrames>kAudioRingFrames-s_pcm_head)?
                            kAudioRingFrames-s_pcm_head:kAudioChunkFrames;
        memcpy(s_play[id],s_pcm+s_pcm_head*2,first*4);
        if(first<kAudioChunkFrames)memcpy(s_play[id]+2*first,s_pcm,(kAudioChunkFrames-first)*4);
        xSemaphoreGive(s_audio_mutex);
        __atomic_store_n(&s_play_busy[id],1u,__ATOMIC_RELEASE);
        if(M5.Speaker.playRaw(s_play[id],kAudioChunkFrames*2,T5P_AUDIO_RATE_HZ,true,1,kSpeakerChannel,false)) {
            xSemaphoreTake(s_audio_mutex,portMAX_DELAY);
            s_pcm_head=(s_pcm_head+kAudioChunkFrames)%kAudioRingFrames;
            s_pcm_count-=kAudioChunkFrames;
            xSemaphoreGive(s_audio_mutex);
            s_play_cursor=(id+1)%kSpeakerSlots;
        } else {
            __atomic_store_n(&s_play_busy[id],0u,__ATOMIC_RELEASE);
            ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(5));
        }
    }
}
static bool init_audio(void) {
    if(!M5.Speaker.isEnabled()) return false;
    s_audio_mutex=xSemaphoreCreateMutex();
    s_pcm=(int16_t *)heap_caps_malloc(kAudioRingFrames*4,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
    if(!s_pcm)s_pcm=(int16_t *)heap_caps_malloc(kAudioRingFrames*4,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!s_audio_mutex || !s_pcm)return false;
    for(int i=0;i<kSpeakerSlots;i++) {
        s_play[i]=(int16_t *)heap_caps_malloc(kAudioChunkFrames*4,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
        if(!s_play[i])return false;
    }
    // Restore the tested R6 Tab5 ES8388 / amplifier restart sequence.
    M5.Speaker.stop();M5.Speaker.end();vTaskDelay(pdMS_TO_TICKS(50));
    auto cfg=M5.Speaker.config();cfg.sample_rate=T5P_AUDIO_RATE_HZ;
    cfg.stereo=true;cfg.dma_buf_len=256;cfg.dma_buf_count=8;
    cfg.task_priority=4;cfg.task_pinned_core=0;
    M5.Speaker.config(cfg);
    if(!M5.Speaker.begin())return false;
    M5.Speaker.setBufferReleaseCallback(nullptr,speaker_released);
    vTaskDelay(pdMS_TO_TICKS(20));
    M5.Speaker.setAllChannelVolume(255);
    M5.Speaker.setChannelVolume(kSpeakerChannel,255);
    M5.Speaker.setVolume(33);
    s_audio_ready=true;
    if(xTaskCreatePinnedToCore(audio_worker,"tab5_pcm",4096,nullptr,4,&s_audio_task,0)!=pdPASS) {
        s_audio_ready=false;return false;
    }
    return true;
}
extern "C" int t5p_storage_mount(void) {
    if(s_storage_ready) return T5P_OK;
    sdmmc_host_t host=SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot=SDMMC_SLOT_CONFIG_DEFAULT();
    host.max_freq_khz=20000;host.flags|=SDMMC_HOST_FLAG_4BIT;
    slot.clk=GPIO_NUM_43;slot.cmd=GPIO_NUM_44;
    slot.d0=GPIO_NUM_39;slot.d1=GPIO_NUM_40;slot.d2=GPIO_NUM_41;slot.d3=GPIO_NUM_42;
    slot.width=4;
#ifdef SDMMC_SLOT_FLAG_INTERNAL_PULLUP
    slot.flags|=SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
#endif
    esp_vfs_fat_sdmmc_mount_config_t cfg={};
    cfg.format_if_mount_failed=false;cfg.max_files=16;cfg.allocation_unit_size=16*1024;
    esp_err_t err=esp_vfs_fat_sdmmc_mount(T5P_SD_ROOT,&host,&slot,&cfg,&s_card);
    if(err!=ESP_OK) {ESP_LOGW(TAG,"SD not mounted: %s",esp_err_to_name(err));return T5P_ERR_IO;}
    s_storage_ready=true;return T5P_OK;
}
extern "C" int t5p_storage_enumerate(const char *directory,t5p_enum_file_fn callback,void *user) {
    if(!s_storage_ready)return T5P_ERR_NOT_READY;
    if(!directory || !callback || strncmp(directory,T5P_SD_ROOT"/",strlen(T5P_SD_ROOT)+1)!=0 ||
       strstr(directory,".."))return T5P_ERR_INVALID;
    DIR *dir=opendir(directory);
    if(!dir)return T5P_ERR_IO;
    int count=0;struct dirent *e;
    while((e=readdir(dir))) {
        if(e->d_name[0]=='.')continue;
        char path[512];
        if(snprintf(path,sizeof(path),"%s/%s",directory,e->d_name)>=(int)sizeof(path))continue;
        struct stat st;
        if(stat(path,&st)!=0 || !S_ISREG(st.st_mode))continue;
        ++count;
        if(callback(e->d_name,user)!=0)break;
    }
    closedir(dir);return count;
}
extern "C" void t5p_ui_begin(void){if(s_display_mutex)xSemaphoreTake(s_display_mutex,portMAX_DELAY);}
extern "C" void t5p_ui_end(void){if(s_display_mutex)xSemaphoreGive(s_display_mutex);}
extern "C" void t5p_ui_clear(uint16_t c){M5.Display.fillScreen(c);}
extern "C" void t5p_ui_fill_rect(int x,int y,int w,int h,uint16_t c){M5.Display.fillRect(x,y,w,h,c);}
extern "C" void t5p_ui_text(int x,int y,const char *text,uint16_t color,int large){
    M5.Display.setFont(large?&lgfx::fonts::FreeSansBold24pt7b:&lgfx::fonts::FreeSansBold18pt7b);
    M5.Display.setTextColor(color);
    M5.Display.setTextDatum(lgfx::textdatum_t::top_left);
    if(text)M5.Display.drawString(text,x,y);
}
extern "C" int t5p_init(void){
    if(s_ready)return T5P_OK;
    auto cfg=M5.config();M5.begin(cfg);
    if(M5.Display.width()<M5.Display.height())M5.Display.setRotation(1);
    if(M5.Display.width()!=1280 || M5.Display.height()!=720){
        ESP_LOGE(TAG,"Unexpected LCD geometry: %dx%d",M5.Display.width(),M5.Display.height());
        return T5P_ERR_IO;
    }
    M5.Display.setSwapBytes(true);M5.Display.setBrightness(255);
    s_display_mutex=xSemaphoreCreateMutex();s_frame_mutex=xSemaphoreCreateMutex();
    s_frame_queue=xQueueCreate(2,sizeof(int));
    s_input_queue=xQueueCreate(64,sizeof(t5p_input_event_t));
    if(!s_display_mutex || !s_frame_mutex || !s_frame_queue || !s_input_queue)return T5P_ERR_NO_MEMORY;
    for(int i=0;i<kFrameCount;i++){
        s_frames[i].data=(uint16_t *)heap_caps_malloc(kMaxFrameW*kMaxFrameH*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
        if(!s_frames[i].data)return T5P_ERR_NO_MEMORY;
    }
    s_stripe=(uint16_t *)heap_caps_malloc(1200*kStripeH*2,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!s_stripe)return T5P_ERR_NO_MEMORY;
    M5.Display.fillScreen(0);
    if(!init_audio())ESP_LOGW(TAG,"Audio unavailable; launcher remains available");
    (void)t5p_storage_mount();  // Missing SD must not stop the UI.
    if(t5p_input_usb_start()!=T5P_OK)ESP_LOGW(TAG,"USB HID unavailable; touch remains available");
    s_ready=true;
    if(xTaskCreatePinnedToCore(render_worker,"tab5_lcd",6144,nullptr,2,&s_render_task,0)!=pdPASS){
        s_ready=false;return T5P_ERR_NO_MEMORY;
    }
    ESP_LOGI(TAG,"Tab5Platform R2 ready; USB/SD/audio availability logged above");
    return T5P_OK;
}
extern "C" void t5p_shutdown(void){
    // R1 product lifetime is from boot to reset. No partial teardown is exposed.
}
