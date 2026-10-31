/* Optional MAME 2000 (0.37b5) frontend. NO EMULATOR ENGINE IS INCLUDED.
 * This is a real libretro host adapter, not a substitute for MAME.
 * Boot attempts report UNAVAILABLE unless an actual port is linked.
 */
#include "t5_sample_mame.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

static volatile t5_mame_state_t s_state=T5_MAME_IDLE;
static volatile int s_stop=0;
#ifdef T5_SAMPLE_MAME_WITH_LIBRETRO_CORE
static char s_rom[512];
#endif
static uint16_t s_buttons=0;
static portMUX_TYPE s_input_mux=portMUX_INITIALIZER_UNLOCKED;
static const char *TAG="MameSample";

int t5_mame_core_built(void) {
#ifdef T5_SAMPLE_MAME_WITH_LIBRETRO_CORE
    return 1;
#else
    return 0;
#endif
}
t5_mame_state_t t5_mame_state(void){return s_state;}
const char *t5_mame_status_text(void){
    switch(s_state){
      case T5_MAME_UNAVAILABLE:return "MAME 2000 engine not linked (host adapter only)";
      case T5_MAME_BAD_ROM:return "ROM file missing or inaccessible";
      case T5_MAME_BOOTING:return "Starting MAME 2000 core";
      case T5_MAME_RUNNING:return "MAME 2000 core running (ESC to quit)";
      case T5_MAME_FAILED:return "MAME core start failed / unsupported set";
      default:return "MAME sample ready";
    }
}
/* Sample maps USB Boot HID keyboard usages to libretro joypad bits.
 * HID key: arrows 0x4f..0x52, Return 0x28, Z 0x1d, X 0x1b,
 * Shift 0xe1/0xe5. Additional arcade control schemes belong in driver maps.
 */
static uint16_t keymask(uint32_t code){
    switch(code){
       case 0x52:return 1u<<4; /* Up */
       case 0x51:return 1u<<5; /* Down */
       case 0x50:return 1u<<6; /* Left */
       case 0x4f:return 1u<<7; /* Right */
       case 0x28:return 1u<<3; /* Start */
       case 0xe1:case 0xe5:return 1u<<2; /* Select */
       case 0x1d:return 1u<<8; /* A */
       case 0x1b:return 1u<<0; /* B */
       default:return 0;
    }
}
void t5_mame_post_input(const t5p_input_event_t *e){
   if(!e)return;
   if(e->type==T5P_INPUT_KEY_DOWN && e->code==0x29){s_stop=1;return;}
   if(e->type!=T5P_INPUT_KEY_DOWN && e->type!=T5P_INPUT_KEY_UP)return;
   uint16_t bit=keymask(e->code);
   portENTER_CRITICAL(&s_input_mux);
   if(e->type==T5P_INPUT_KEY_DOWN)s_buttons|=bit;
   else s_buttons&=(uint16_t)~bit;
   portEXIT_CRITICAL(&s_input_mux);
}
void t5_mame_stop(void){s_stop=1;}

#ifdef T5_SAMPLE_MAME_WITH_LIBRETRO_CORE
#include "libretro_minimal.h"
static unsigned s_pixel_format=RETRO_PIXEL_FORMAT_0RGB1555;
static double s_src_rate=44100.0;
static double s_src_phase=0.0;
static unsigned s_rotation=0;
static uint64_t s_frames=0;
static int s_geometry_rejected=0;
static bool core_env(unsigned cmd,void *data){
    switch(cmd){
      case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:{
          if(!data)return false;
          unsigned f=*(const unsigned*)data;
          if(f!=RETRO_PIXEL_FORMAT_RGB565 && f!=RETRO_PIXEL_FORMAT_0RGB1555)return false;
          s_pixel_format=f;return true;
      }
      case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
      case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
          if(!data)return false;
          *(const char**)data="/sdcard/arcade/system";
          return true;
      case RETRO_ENVIRONMENT_GET_CAN_DUPE:
          if(!data)return false;
          *(bool*)data=false;return true;
      case RETRO_ENVIRONMENT_SET_ROTATION:
          if(!data)return false;
          s_rotation=*(const unsigned*)data&3u;return true;
      case RETRO_ENVIRONMENT_SET_VARIABLES: return true;
      case RETRO_ENVIRONMENT_GET_VARIABLE:
          if(!data)return false;
          ((struct retro_variable*)data)->value=NULL;return false;
      case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
          if(!data)return false;
          *(bool*)data=false;return true;
      case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
      case RETRO_ENVIRONMENT_SET_GEOMETRY:return true;
      default:return false;
    }
}
static void core_video(const void *pixels,unsigned w,unsigned h,size_t pitch){
    if(!pixels || !w || !h)return;
    t5p_video_slot_t slot={0};
    if(t5p_video_acquire(w,h,&slot)!=T5P_OK){
        if(w>384 || h>384)s_geometry_rejected=1;
        return;
    }
    for(unsigned y=0;y<h;y++){
        const uint16_t *src=(const uint16_t*)((const uint8_t*)pixels + y*pitch);
        uint16_t *dst=slot.pixels+y*slot.stride_pixels;
        if(s_pixel_format==RETRO_PIXEL_FORMAT_RGB565){
            memcpy(dst,src,w*sizeof(uint16_t));
        } else {
            for(unsigned x=0;x<w;x++){
                uint16_t v=src[x];
                dst[x]=(uint16_t)(((v&0x7c00u)<<1)|((v&0x03e0u)<<1)|((v&0x001fu)));
            }
        }
    }
    if(t5p_video_present(&slot,(t5p_rotation_t)(s_rotation&3u))!=T5P_OK)
        t5p_video_cancel(&slot);
    ++s_frames;
}
static size_t core_audio_batch(const int16_t *samples,size_t n){
    if(!samples)return n;
    if(s_src_rate<8000.0 || s_src_rate>192000.0) return n;
    /* Simple bounded nearest-neighbour resampling. Production port should use
     * a tested streaming resampler with persistent edge state. */
    int16_t out[256*2];size_t num=0;
    const double step=44100.0/s_src_rate;
    for(size_t i=0;i<n;i++){
        s_src_phase+=step;
        while(s_src_phase>=1.0){
            out[num*2]=samples[i*2];out[num*2+1]=samples[i*2+1];
            ++num;s_src_phase-=1.0;
            if(num==256){(void)t5p_audio_submit(out,num);num=0;}
        }
    }
    if(num)(void)t5p_audio_submit(out,num);
    return n;
}
static void core_audio(int16_t l,int16_t r){int16_t p[2]={l,r};(void)core_audio_batch(p,1);}
static void core_poll(void){}
static int16_t core_input(unsigned port,unsigned dev,unsigned index,unsigned id){
    (void)index;
    if(port||dev!=RETRO_DEVICE_JOYPAD||id>=16)return 0;
    portENTER_CRITICAL(&s_input_mux);
    uint16_t v=s_buttons;
    portEXIT_CRITICAL(&s_input_mux);
    return (v&(1u<<id)) ? 1:0;
}
static void core_task(void *unused){
    (void)unused;
    bool inited=false,loaded=false;
    struct retro_system_info info={0};
    struct retro_system_av_info av={0};
    retro_set_environment(core_env);
    retro_set_video_refresh(core_video);
    retro_set_audio_sample(core_audio);
    retro_set_audio_sample_batch(core_audio_batch);
    retro_set_input_poll(core_poll);
    retro_set_input_state(core_input);
    retro_init();inited=true;
    retro_get_system_info(&info);
    struct retro_game_info game={s_rom,NULL,0,NULL};
    /* MAME 2000 normally requires the ROM path and reads zip itself. */
    loaded=retro_load_game(&game);
    if(!loaded){s_state=T5_MAME_FAILED;goto cleanup;}
    retro_get_system_av_info(&av);
    s_src_rate=av.timing.sample_rate>0.0?av.timing.sample_rate:44100.0;
    s_state=T5_MAME_RUNNING;
    ESP_LOGI(TAG,"core=%s version=%s fps=%.3f audio=%.1f",
            info.library_name?info.library_name:"unknown",
            info.library_version?info.library_version:"unknown",av.timing.fps,s_src_rate);
    const uint32_t frame_period=(av.timing.fps>=10.0 && av.timing.fps<=120.0)?
       (uint32_t)(1000000.0/av.timing.fps):16667u;
    uint64_t deadline=t5p_time_us();
    while(!s_stop){
        retro_run();
        deadline+=frame_period;
        uint64_t now=t5p_time_us();
        if(deadline>now){
            uint64_t remaining=deadline-now;
            if(remaining>=2000)vTaskDelay(pdMS_TO_TICKS(remaining/1000));
            /* no host-spin fine wait; avoid starving the audio/display core */
        }else if(now-deadline>frame_period*3u)deadline=now;
    }
cleanup:
    if(loaded)retro_unload_game();
    if(inited)retro_deinit();
    if(s_geometry_rejected)ESP_LOGW(TAG,"Frame geometry exceeded R2 384x384 bound");
    if(s_state!=T5_MAME_FAILED)s_state=T5_MAME_IDLE;
    vTaskDelete(NULL);
}
#endif

int t5_mame_start(const char *path){
    if(!path || strlen(path)>=sizeof(s_rom) || strncmp(path,"/sdcard/arcade/roms/",20)!=0)
        return T5P_ERR_INVALID;
    if(s_state==T5_MAME_RUNNING || s_state==T5_MAME_BOOTING)return T5P_ERR_BUSY;
    struct stat st;
    if(stat(path,&st)!=0 || !S_ISREG(st.st_mode)){
        s_state=T5_MAME_BAD_ROM;return T5P_ERR_IO;
    }
#ifndef T5_SAMPLE_MAME_WITH_LIBRETRO_CORE
    s_state=T5_MAME_UNAVAILABLE;
    ESP_LOGW(TAG,"sample/mame has no linked MAME 2000 emulator engine");
    return T5P_ERR_NOT_READY;
#else
    strncpy(s_rom,path,sizeof(s_rom)-1);s_rom[sizeof(s_rom)-1]=0;
    s_stop=0;s_geometry_rejected=0;s_pixel_format=RETRO_PIXEL_FORMAT_0RGB1555;
    s_src_phase=0.0;s_frames=0;
    portENTER_CRITICAL(&s_input_mux);s_buttons=0;portEXIT_CRITICAL(&s_input_mux);
    s_state=T5_MAME_BOOTING;
    BaseType_t result=xTaskCreatePinnedToCore(core_task,"mame2000",16384,NULL,4,NULL,1);
    if(result!=pdPASS){s_state=T5_MAME_FAILED;return T5P_ERR_NO_MEMORY;}
    return T5P_OK;
#endif
}
