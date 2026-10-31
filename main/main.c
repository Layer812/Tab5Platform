/* Tab5Platform reference app. The optional MAME sample lives in sample/mame.
 * Source-only release: MAME core is absent until an actual port is linked. */
#include "tab5_platform.h"
#include "t5_sample_mame.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#define MAX_ROMS 128
#define PAGE_ROWS 8
static char roms[MAX_ROMS][128];
static int rom_count=0,selected=0,page=0;
static int sd_ok=0;
static int sample_launched=0;
static uint64_t notice_until=0;
static char notice[160];
static const uint16_t BG=0x0841, PANEL=0x1082, HILITE=0x2945, WHITE=0xffff,GRAY=0xbdf7;
static int is_zip(const char *name){
    size_t n=strlen(name);
    return n>4 && strcasecmp(name+n-4,".zip")==0;
}
static int append_rom(const char *name,void *userdata){
    (void)userdata;
    if(!is_zip(name) || rom_count>=MAX_ROMS)return 0;
    snprintf(roms[rom_count],sizeof(roms[0]),"%s",name);
    ++rom_count;return 0;
}
static void sort_roms(void){
    for(int i=1;i<rom_count;i++){
        char item[128];memcpy(item,roms[i],128);
        int j=i-1;
        while(j>=0 && strcasecmp(roms[j],item)>0){
            memcpy(roms[j+1],roms[j],128);--j;
        }
        memcpy(roms[j+1],item,128);
    }
}
static void reload_roms(void){
    rom_count=0;selected=0;page=0;
    sd_ok=(t5p_storage_mount()==T5P_OK);
    if(sd_ok){
        int n=t5p_storage_enumerate("/sdcard/arcade/roms",append_rom,NULL);
        if(n<0){
            // Directory may not exist. No directories created without user intent.
            snprintf(notice,sizeof(notice),"Place ZIP files in /sdcard/arcade/roms/");
            notice_until=t5p_time_us()+10000000;
        }
        sort_roms();
    }
}
static void draw_ui(void){
    t5p_ui_begin();
    t5p_ui_clear(BG);
    t5p_ui_fill_rect(0,0,1280,98,PANEL);
    t5p_ui_text(42,16,"Tab5 Platform",WHITE,1);
    t5p_ui_text(880,28,"Platform R2",GRAY,0);
    t5p_ui_text(46,112,"Sample / MAME 2000    ROMs: /sdcard/arcade/roms",GRAY,0);
    t5p_ui_fill_rect(40,170,1200,445,PANEL);
    t5p_ui_text(66,142,t5_mame_core_built()?"MAME backend: linked (not validated)":"MAME backend: NOT LINKED",GRAY,0);
    if(!sd_ok) t5p_ui_text(66,194,"SD not mounted - insert a card",WHITE,0);
    else if(!rom_count)t5p_ui_text(66,194,"No ZIP files found",WHITE,0);
    else{
        for(int row=0;row<PAGE_ROWS;row++){
            int i=page*PAGE_ROWS+row;if(i>=rom_count)break;
            int y=176+row*54;
            if(i==selected)t5p_ui_fill_rect(52,y,1176,52,HILITE);
            char label[136];snprintf(label,sizeof(label),"%3d   %.105s",i+1,roms[i]);
            t5p_ui_text(70,y+5,label,WHITE,0);
        }
    }
    t5p_ui_fill_rect(40,634,275,66,PANEL);
    t5p_ui_fill_rect(335,634,280,66,PANEL);
    t5p_ui_fill_rect(635,634,280,66,PANEL);
    t5p_ui_fill_rect(935,634,305,66,PANEL);
    t5p_ui_text(65,646,"UP",WHITE,0);
    t5p_ui_text(360,646,"DOWN",WHITE,0);
    t5p_ui_text(660,646,"RELOAD",WHITE,0);
    t5p_ui_text(961,646,"SELECT",WHITE,0);
    if(t5p_time_us()<notice_until){
        t5p_ui_fill_rect(45,570,1190,45,0x2104);
        t5p_ui_text(65,575,notice,WHITE,0);
    }
    t5p_ui_end();
}
static void move_selection(int d){
    if(!rom_count)return;
    selected+=d;
    if(selected<0)selected=rom_count-1;
    if(selected>=rom_count)selected=0;
    page=selected/PAGE_ROWS;
}
static void select_rom(void){
    if(!rom_count)return;
    char path[320];
    if(snprintf(path,sizeof(path),"/sdcard/arcade/roms/%.127s",roms[selected])>=(int)sizeof(path))return;
    int rc=t5_mame_start(path);
    if(rc==T5P_OK){sample_launched=1;return;}
    snprintf(notice,sizeof(notice),"%.132s",t5_mame_status_text());
    notice_until=t5p_time_us()+6000000;
}
static int update_input(void){
    int dirty=0;
    t5p_input_event_t e;
    static int last_touch_pressed=0;
    while(t5p_input_poll(&e)){
        if(sample_launched){
            t5_mame_post_input(&e);
            if(e.type==T5P_INPUT_POINTER && e.value && e.x>1120 && e.y<100)
                t5_mame_stop();
            continue;
        }
        if(e.type==T5P_INPUT_KEY_DOWN){
            if(e.code==0x52){move_selection(-1);dirty=1;}
            else if(e.code==0x51){move_selection(1);dirty=1;}
            else if(e.code==0x28){select_rom();dirty=1;}
            else if(e.code==0x15){reload_roms();dirty=1;} // R
        }else if(e.type==T5P_INPUT_POINTER){
            if(e.value && !last_touch_pressed){
                if(e.y>=634){
                    if(e.x<315)move_selection(-1);
                    else if(e.x<615)move_selection(1);
                    else if(e.x<935)reload_roms();
                    else select_rom();
                    dirty=1;
                }else if(e.y>=176 && e.y<608){
                    int idx=page*PAGE_ROWS+(e.y-176)/54;
                    if(idx<rom_count){selected=idx;dirty=1;}
                }
            }
            last_touch_pressed=e.value?1:0;
        }
    }
    return dirty;
}
void app_main(void){
    ESP_LOGI("Tab5Platform","R2 independent platform / optional MAME sample frontend");
    if(t5p_init()!=T5P_OK){
        ESP_LOGE("Tab5Platform","Platform initialization failed; no flash/partition writes performed");
        return;
    }
    reload_roms();draw_ui();
    int redraw_notice=0;
    while(1){
        int dirty=update_input();
        if(sample_launched){
            t5_mame_state_t state=t5_mame_state();
            if(state==T5_MAME_RUNNING || state==T5_MAME_BOOTING){
                vTaskDelay(pdMS_TO_TICKS(16));
                continue;
            }
            sample_launched=0;
            snprintf(notice,sizeof(notice),"MAME sample returned: %s",t5_mame_status_text());
            notice_until=t5p_time_us()+5000000;
            dirty=1;
        }
        int has_notice=t5p_time_us()<notice_until;
        if(has_notice!=redraw_notice){dirty=1;redraw_notice=has_notice;}
        if(dirty)draw_ui();
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
