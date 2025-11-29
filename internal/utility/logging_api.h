#ifndef __INTERNAL_UTILITY_LOGGING_API_H__
#define __INTERNAL_UTILITY_LOGGING_API_H__

#define logg(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, __VA_ARGS__);
#define logg_s(s)      printf("[%s:%d] " s "\n", __FILE__, __LINE__);

#endif
