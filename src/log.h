#pragma once

#ifdef LOG_INSTRUCTIONS
#define LOG(fmt, ...)    printf(fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)    
#endif