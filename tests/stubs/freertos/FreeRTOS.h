#pragma once
/* Minimal compile-only host stubs for unlinked MAME sample adapter tests. */
typedef unsigned portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0u
#define portENTER_CRITICAL(mux) ((void)(mux))
#define portEXIT_CRITICAL(mux) ((void)(mux))
