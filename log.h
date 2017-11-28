#ifndef _LOG_H_
#define _LOG_H_

//#define DEBUG

// Log always even if DEBUG is not defined
#define LI(x, ...) printf("INFO: " x "\n", ##__VA_ARGS__)
#define LE(x, ...) printf("ERROR: " x "\n", ##__VA_ARGS__)

#ifdef DEBUG

#define LOG(msg, ...) printf(msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...) LOG("INFO: " msg "\n", ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) LOG("ERROR: " msg "\n", ##__VA_ARGS__)

#else

#define LOG(msg, ...)
#define LOG_INFO(msg, ...) 
#define LOG_ERROR(msg, ...) 

#endif

#endif /* ifndef _LOG_H_ */
