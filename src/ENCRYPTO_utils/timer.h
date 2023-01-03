#pragma once
#include <sys/time.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>

class CSocket;

// Note do not change P_FIRST and P_LAST and keep them pointing to the first and last element in the enum
enum PHASE {
	P_TOTAL, P_INIT, P_CIRCUIT, P_NETWORK, P_BASE_OT, P_SETUP, P_OT_EXT, P_GARBLE, P_ONLINE, P_FIRST = P_TOTAL, P_LAST = P_ONLINE
};

// Structure for measuring runtime
struct proto_timings {
	double timing;
	timespec tbegin;
	timespec tend;
};

// Structure for counting communication
struct proto_comm {
	uint64_t totalcomm;
	uint64_t cbegin;
	uint64_t cend;
};

// Structure for counting memory
struct proto_mem {
    uint64_t peekmem;
    uint64_t mbegin;
    uint64_t mend;
};

extern proto_timings m_tTimes[P_LAST - P_FIRST + 1];
extern proto_mem m_tMem[P_LAST - P_FIRST + 1];
extern proto_comm m_tSend[P_LAST - P_FIRST + 1];
extern proto_comm m_tRecv[P_LAST - P_FIRST + 1];

/**
 * Return time difference in milliseconds
 */
double getMillies(timespec timestart, timespec timeend);


/**
 * Return memory difference in Bytes
 */
uint64_t getBytes(uint64_t memstart, uint64_t memeend);


/**
 * Start measuring runtime for a given phase
 * @param msg - a message for debugging
 * @param phase - the phase to measure
 */
void StartWatch(const std::string& msg, PHASE phase);

/**
 * Stop measuring runtime
 * Called after StartWatch() with identical phase parameter
 * @param msg - a message for debugging
 * @param phase - the phase to measure
 */
void StopWatch(const std::string& msg, PHASE phase);

/**
 * Start measuring both runtime and communication
 * @param msg - a message for debugging
 * @param phase - the phase to measure
 * @param sock - a vector of sockets
 */
void StartRecording(const std::string& msg, PHASE phase,
		const std::vector<std::unique_ptr<CSocket>>& sock);

/**
 * Stop measuring both runtime and communication
 * Called after StartRecording() with identical phase parameter
 * @param msg - a message for debugging
 * @param phase - the phase to measure
 * @param sock - a vector of sockets
 */
void StopRecording(const std::string& msg, PHASE phase,
		const std::vector<std::unique_ptr<CSocket>>& sock);

void PrintTimings();

void PrintCommunication();

void PrintMemory();

inline double GetTimeForPhase(PHASE phase) {
	return m_tTimes[phase].timing;
}

inline uint64_t GetSentDataForPhase(PHASE phase) {
	return m_tSend[phase].totalcomm;
}

inline uint64_t GetReceivedDataForPhase(PHASE phase) {
	return m_tRecv[phase].totalcomm;
}

inline uint64_t GetMemoryForPhase(PHASE phase) {
    return m_tMem[phase].peekmem;
}

// the info line num in /proc/{pid}/status file
#define VMRSS_LINE 22
#define VMHWM_LINE 21

enum mem_type{ CURRENT_MEM, PEEK_MEM };

// get specific process physical memeory occupation size by pid (MB)
inline void get_memory_usage(mem_type memType, uint64_t* result)
{
    int pid = getpid();

    char file_name[64] = { 0 };
    FILE* fd;
    char line_buff[512] = { 0 };
    sprintf(file_name, "/proc/%d/status", pid);

    fd = fopen(file_name, "r");
    if (nullptr == fd)
        return;

    char name[64];
    uint64_t vmrss = 0;

    int line_pos = -1;
    if (memType == mem_type::CURRENT_MEM) {
        line_pos = VMRSS_LINE;
    }
    else {
        line_pos = VMHWM_LINE;
    }

    for (int i = 0; i < line_pos - 1; i++)
        fgets(line_buff, sizeof(line_buff), fd);


    fgets(line_buff, sizeof(line_buff), fd);
    sscanf(line_buff, "%s %lu", name, &vmrss);
    fclose(fd);

    // VmRSS in KB
//    return vmrss;
    *result = vmrss;
}