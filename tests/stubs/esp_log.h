#pragma once
#include <stdio.h>
#define ESP_LOGW(tag,fmt,...) ((void)fprintf(stderr,"[WARN %s] " fmt "\n",tag,##__VA_ARGS__))
