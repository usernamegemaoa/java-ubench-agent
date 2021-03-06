/*
 * Copyright 2014 Charles University in Prague
 * Copyright 2014 Vojtech Horky
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UBENCH_H_GUARD
#define UBENCH_H_GUARD

#pragma warning(push, 0)
#include <jvmti.h>
#include <jni.h>
#include <jvmticmlr.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#pragma warning(pop)

#include "myatomic.h"

#ifdef __GNUC__
#define UNUSED_PARAMETER(name) name __attribute__((unused))
#else
#define UNUSED_PARAMETER(name) name
#endif

/*
 * Prevent condition expression is constant warning in wrappers around
 * multi-statement macros (the do { ... } while (0) construct).
 */
#ifdef _MSC_VER
#define ONCE \
	__pragma(warning(push)) \
	__pragma(warning(disable:4127)) \
    while (0) \
    __pragma(warning(pop))
#else
#define ONCE while (0)
#endif

#ifdef JITCOUNTER_DEBUG
#define DEBUG_PRINTF(fmt, ...) printf("[ubench-agent]: " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...) (void)0
#endif

#ifdef HAS_GETRUSAGE
#include <sys/resource.h>
#endif

#ifdef HAS_TIMESPEC
typedef struct timespec timestamp_t;
#elif defined(HAS_QUERY_PERFORMANCE_COUNTER)
#pragma warning(push, 0)
#include <windows.h>
#pragma warning(pop)
typedef LARGE_INTEGER timestamp_t;
#else
typedef int timestamp_t;
#endif

#define UBENCH_MAX_PAPI_EVENTS 20

/*
 * Backend bit masks.
 *
 * Backend refers to a function that collects the counters (be it number
 * of JIT compilations or number of page faults or ...).
 */
#define UBENCH_EVENT_BACKEND_LINUX 1
#define UBENCH_EVENT_BACKEND_RESOURCE_USAGE 2
#define UBENCH_EVENT_BACKEND_PAPI 4
#define UBENCH_EVENT_BACKEND_SYS_WALLCLOCK 8
#define UBENCH_EVENT_BACKEND_JVM_COMPILATIONS 16

typedef struct {
	timestamp_t timestamp;
#ifdef HAS_GETRUSAGE
	struct rusage resource_usage;
#endif
	long compilations;
	int garbage_collections;
#ifdef HAS_PAPI
	long long papi_events[UBENCH_MAX_PAPI_EVENTS];
	int papi_rc1;
	int papi_rc2;
#endif
} ubench_events_snapshot_t;

typedef struct {
	ubench_events_snapshot_t start;
	ubench_events_snapshot_t end;
} benchmark_run_t;

typedef struct ubench_event_info ubench_event_info_t;
typedef long long (*event_getter_func_t)(const benchmark_run_t *, const ubench_event_info_t *);

struct ubench_event_info {
	unsigned int backend;
	int id;
	size_t papi_index;
	event_getter_func_t op_get;
	char *name;
};

typedef struct {
	unsigned int used_backends;

	ubench_event_info_t *used_events;
	size_t used_events_count;

#ifdef HAS_PAPI
	int used_papi_events[UBENCH_MAX_PAPI_EVENTS];
	size_t used_papi_events_count;
	int papi_eventset;
#endif

	benchmark_run_t *data;
	size_t data_size;
	size_t data_index;
} benchmark_configuration_t;


extern jint ubench_counters_init(jvmtiEnv *);
extern void ubench_register_this_thread(jthread, JNIEnv*);
extern void ubench_unregister_this_thread(jthread, JNIEnv*);
extern jint ubench_benchmark_init(void);
extern int ubench_event_init(void);
extern int ubench_event_resolve(const char *, ubench_event_info_t *);

extern void ubench_measure_start(const benchmark_configuration_t const*, ubench_events_snapshot_t *);
extern void ubench_measure_stop(const benchmark_configuration_t const*, ubench_events_snapshot_t *);

extern ubench_atomic_t counter_compilation;
extern ubench_atomic_t counter_compilation_total;
extern ubench_atomic_t counter_gc_total;

static inline char *ubench_str_dup(const char *str) {
	char *result = malloc(sizeof(char) * strlen(str) + 1);
	if (result == NULL) {
		return NULL;
	}

	/*
	 * We know that the buffer is big enough and thus we can use strcpy()
	 * without worrying that we would run past the buffer.
	 */
#pragma warning(suppress:4996)
	strcpy(result, str);

	return result;
}

#endif
