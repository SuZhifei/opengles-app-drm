#ifndef __COMMON_H__
#define __COMMON_H__

#define debug	0

#define MSG(fmt, ...) \
		do { fprintf(stderr, fmt "\n", ##__VA_ARGS__); } while (0)
#define ERROR(fmt, ...) \
		do { fprintf(stderr, "ERROR:%s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); } while (0)
#define ERROR_MSG(msg) \
       do { perror(msg); } while (0)
/* Dynamic debug. */
#define DBG(fmt, ...) \
		do { if (debug) fprintf(stderr, fmt "\n", ##__VA_ARGS__); } while (0)

#ifndef true
#define true	1
#define false	0
#endif

#endif//__COMMON_H__
