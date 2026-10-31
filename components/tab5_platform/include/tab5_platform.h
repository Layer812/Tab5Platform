/* ArcadeTab5 platform API R1. New code, no PX68K/MAME dependency.
 * Original API declarations: Copyright (c) 2026 Layer812/ArcadeTab5 contributors.
 * Project's original-code license must be selected before public redistribution.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define T5P_AUDIO_RATE_HZ 44100u
#define T5P_LCD_LOGICAL_WIDTH 1280u
#define T5P_LCD_LOGICAL_HEIGHT 720u
#define T5P_SD_ROOT "/sdcard"

typedef enum { T5P_OK=0, T5P_ERR_NOT_READY=-1, T5P_ERR_INVALID=-2,
               T5P_ERR_BUSY=-3, T5P_ERR_NO_MEMORY=-4, T5P_ERR_IO=-5 } t5p_result_t;
typedef enum { T5P_ROTATION_0=0, T5P_ROTATION_90=1,
               T5P_ROTATION_180=2,T5P_ROTATION_270=3 } t5p_rotation_t;
typedef struct { uint16_t *pixels; uint32_t width, height, stride_pixels, slot_token; } t5p_video_slot_t;
typedef enum { T5P_INPUT_KEY_DOWN, T5P_INPUT_KEY_UP, T5P_INPUT_BUTTON_DOWN,
               T5P_INPUT_BUTTON_UP, T5P_INPUT_AXIS, T5P_INPUT_POINTER } t5p_input_type_t;
typedef struct { t5p_input_type_t type; uint32_t device_id, code;
                 int32_t value, x, y; } t5p_input_event_t;
typedef int (*t5p_enum_file_fn)(const char *name, void *userdata);

/* Must be initialized once from CPU0 app_main. CPU0 owns display/touch/audio/USB. */
int t5p_init(void);
void t5p_shutdown(void);     /* currently intended only for terminal teardown */
uint64_t t5p_time_us(void);
/* Producer: CPU1 recommended; 2 bounded immutable slots, 384x384 maximum R1.
 * acquire/present never wait. Returns T5P_OK (0) on success, negative on rejection.
 * On successful present the caller MUST NOT access slot->pixels. */
int t5p_video_acquire(uint32_t width, uint32_t height, t5p_video_slot_t *out);
int t5p_video_present(t5p_video_slot_t *slot, t5p_rotation_t rotation);
void t5p_video_cancel(t5p_video_slot_t *slot);
/* PCM: signed 16-bit stereo interleaved 44.1 kHz; accepted frames may be partial.
 * May be called from CPU1. Does not block a guest timeline. */
size_t t5p_audio_submit(const int16_t *interleaved_stereo, size_t frames);
size_t t5p_audio_queued_frames(void);
int t5p_input_poll(t5p_input_event_t *out); /* pop one event, 1/0 */
/* SDMMC: GPIO43 CLK,44 CMD,39..42 data. Mount never formats card. */
int t5p_storage_mount(void);
int t5p_storage_enumerate(const char *directory, t5p_enum_file_fn fn, void *userdata);
/* Synchronous UI drawing from CPU0, explicit display mutex, no X68000 terms.
 * These methods are for launcher; avoid use within tight emulation loop. */
void t5p_ui_begin(void);
void t5p_ui_end(void);
void t5p_ui_clear(uint16_t color);
void t5p_ui_fill_rect(int x, int y, int w, int h, uint16_t color);
void t5p_ui_text(int x, int y, const char *ascii, uint16_t color, int large);
int t5p_input_usb_start(void); /* USB boot keyboards/mice; gamepads pending HID descriptor handling */
/* Source/input pure geometry helper is also buildable with host gcc. */
void t5p_fit_rect(uint32_t src_w, uint32_t src_h, int rotation,
                  uint32_t box_w, uint32_t box_h,
                  uint32_t *out_w, uint32_t *out_h);
#ifdef __cplusplus
}
#endif
