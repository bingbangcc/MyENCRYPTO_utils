#include <string>
#include <iostream>
#include <cstdlib>
#include <vector>

#include "timer.h"
#include "constants.h"
#include "socket.h"
#include "typedefs.h"


proto_timings m_tTimes[P_LAST - P_FIRST + 1];
proto_mem m_tMem[P_LAST - P_FIRST + 1];
proto_comm m_tSend[P_LAST - P_FIRST + 1];
proto_comm m_tRecv[P_LAST - P_FIRST + 1];

double getMillies(timespec timestart, timespec timeend) {
	long time1 = (timestart.tv_sec * 1000000) + (timestart.tv_nsec / 1000);
	long time2 = (timeend.tv_sec * 1000000) + (timeend.tv_nsec / 1000);

	return (double) (time2 - time1) / 1000;
}

uint64_t getBytes(uint64_t memstart, uint64_t memeend) {
    return (uint64_t) (memeend - memstart);
}


void StartWatch(const std::string& msg, PHASE phase) {
	if (phase < P_FIRST || phase > P_LAST) {
		std::cerr << "Phase not recognized: " << phase << std::endl;
		return;
	}

    // get begin time
	clock_gettime(CLOCK_MONOTONIC, &(m_tTimes[phase].tbegin));

    // get current memory
    get_memory_usage(mem_type::CURRENT_MEM, &(m_tMem[phase].mbegin));


#ifndef BATCH
	std::cout << msg << std::endl;
#else
	(void)msg;  // silence -Wunused-parameter warning
#endif
}


void StopWatch(const std::string& msg, PHASE phase) {
	if (phase < P_FIRST || phase > P_LAST) {
		std::cerr << "Phase not recognized: " << phase << std::endl;
		return;
	}

    // get end time
	clock_gettime(CLOCK_MONOTONIC, &(m_tTimes[phase].tend));
	m_tTimes[phase].timing = getMillies(m_tTimes[phase].tbegin, m_tTimes[phase].tend);

    // get peek memory
    get_memory_usage(mem_type::PEEK_MEM, &(m_tMem[phase].mpeek));
	// get_memory_usage(mem_type::CURRENT_MEM, &(m_tMem[phase].mend));
    m_tMem[phase].addmem = getBytes(m_tMem[phase].mbegin, m_tMem[phase].mpeek);


#ifndef BATCH
	std::cout << msg << m_tTimes[phase].timing << " ms " << std::endl;
#else
	(void)msg;  // silence -Wunused-parameter warning
#endif
}

void StartRecording(const std::string& msg, PHASE phase,
		const std::vector<std::unique_ptr<CSocket>>& sock) {
	StartWatch(msg, phase);

	m_tSend[phase].cbegin = 0;
	m_tRecv[phase].cbegin = 0;
	for(uint32_t i = 0; i < sock.size(); i++) {
		m_tSend[phase].cbegin += sock[i]->getSndCnt();
		m_tRecv[phase].cbegin += sock[i]->getRcvCnt();
	}
}

void StopRecording(const std::string& msg, PHASE phase,
		const std::vector<std::unique_ptr<CSocket>>& sock) {
	StopWatch(msg, phase);

	m_tSend[phase].cend = 0;
	m_tRecv[phase].cend = 0;
	for(uint32_t i = 0; i < sock.size(); i++) {
		m_tSend[phase].cend += sock[i]->getSndCnt();
		m_tRecv[phase].cend += sock[i]->getRcvCnt();
	}

	m_tSend[phase].totalcomm = m_tSend[phase].cend - m_tSend[phase].cbegin;
	m_tRecv[phase].totalcomm = m_tRecv[phase].cend - m_tRecv[phase].cbegin;
}


void PrintTimings() {
	// std::string unit = " ms";
	// std::cout << std::endl;
	// std::cout << "Timings: " << std::endl;
	
	// std::cout << "Init =\t\t" << m_tTimes[P_INIT].timing << unit << std::endl;
	// std::cout << "CircuitGen =\t" << m_tTimes[P_CIRCUIT].timing << unit << std::endl;
	// std::cout << "Network =\t" << m_tTimes[P_NETWORK].timing << unit << std::endl;
	// std::cout << "BaseOTs =\t" << m_tTimes[P_BASE_OT].timing << unit << std::endl;
	// std::cout << "OTExtension =\t" << m_tTimes[P_OT_EXT].timing << unit << std::endl;
	// std::cout << "Garbling =\t" << m_tTimes[P_GARBLE].timing << unit << std::endl;
	// std::cout << "Setup =\t\t" << m_tTimes[P_SETUP].timing << unit << std::endl;
	// std::cout << "Online =\t" << m_tTimes[P_ONLINE].timing << unit << std::endl;
	// std::cout << "Total =\t\t" << m_tTimes[P_TOTAL].timing << unit << std::endl;
	std::cout << m_tTimes[P_ONLINE].timing << std::endl;
}

void PrintCommunication() {
	std::string unit = " bytes";
	std::cout << "Communication: " << std::endl;
	
	std::cout << "Init Sent / Rcv\t\t" << m_tSend[P_INIT].totalcomm << " " << unit << " / " << m_tRecv[P_INIT].totalcomm << unit << std::endl;
	std::cout << "CircuitGen Sent / Rcv\t" << m_tSend[P_CIRCUIT].totalcomm << " " << unit << " / " << m_tRecv[P_CIRCUIT].totalcomm << unit << std::endl;
	std::cout << "Network Sent / Rcv\t" << m_tSend[P_NETWORK].totalcomm << " " << unit << " / " << m_tRecv[P_NETWORK].totalcomm << unit << std::endl;
	std::cout << "BaseOTs Sent / Rcv\t" << m_tSend[P_BASE_OT].totalcomm << " " << unit << " / " << m_tRecv[P_BASE_OT].totalcomm << unit << std::endl;
	std::cout << "OTExtension Sent / Rcv\t" << m_tSend[P_OT_EXT].totalcomm << " " << unit << " / " << m_tRecv[P_OT_EXT].totalcomm << unit << std::endl;
	std::cout << "Garbling Sent / Rcv\t" << m_tSend[P_GARBLE].totalcomm << " " << unit << " / " << m_tRecv[P_GARBLE].totalcomm << unit << std::endl;
	std::cout << "Setup Sent / Rcv\t" << m_tSend[P_SETUP].totalcomm << " " << unit << " / " << m_tRecv[P_SETUP].totalcomm << unit << std::endl;
	std::cout << "Online Sent / Rcv\t" << m_tSend[P_ONLINE].totalcomm << " " << unit << " / " << m_tRecv[P_ONLINE].totalcomm << unit << std::endl;
	std::cout << "Total Sent / Rcv\t" << m_tSend[P_TOTAL].totalcomm << " " << unit << " / " << m_tRecv[P_TOTAL].totalcomm << unit << std::endl;
}

void PrintMemory() {
    std::string unit = " KB";
	std::cout << std::endl;
    std::cout << "Memory: " << std::endl;

	std::cout << "Init Begin / Peek\t" << m_tMem[P_INIT].mbegin << unit << " / " << m_tMem[P_INIT].mpeek << unit << std::endl;
	std::cout << "CircuitGen Begin / Peek\t" << m_tMem[P_CIRCUIT].mbegin << unit << " / " << m_tMem[P_CIRCUIT].mpeek << unit << std::endl;
	std::cout << "Network Begin / Peek\t" << m_tMem[P_NETWORK].mbegin << unit << " / " << m_tMem[P_NETWORK].mpeek << unit << std::endl;
	std::cout << "BaseOTs Begin / Peek\t" << m_tMem[P_BASE_OT].mbegin << unit << " / " << m_tMem[P_BASE_OT].mpeek << unit << std::endl;
	std::cout << "OTExtension Begin / Peek\t" << m_tMem[P_OT_EXT].mbegin << unit << " / " << m_tMem[P_OT_EXT].mpeek << unit << std::endl;
	std::cout << "Garbling Begin / Peek\t" << m_tMem[P_GARBLE].mbegin << unit << " / " << m_tMem[P_GARBLE].mpeek << unit << std::endl;
	std::cout << "Setup Begin / Peek\t" << m_tMem[P_SETUP].mbegin << unit << " / " << m_tMem[P_SETUP].mpeek << unit << std::endl;
	std::cout << "Online Begin / Peek\t" << m_tMem[P_ONLINE].mbegin << unit << " / " << m_tMem[P_ONLINE].mpeek << unit << std::endl;
	std::cout << "Total Begin / Peek\t" << m_tMem[P_TOTAL].mbegin << unit << " / " << m_tMem[P_TOTAL].mpeek << unit << std::endl;
	std::cout << std::endl;
    // std::cout << "Setup Begin=\t" << m_tMem[P_SETUP].mbegin << unit << std::endl;
	// std::cout << "Setup End=\t" << m_tMem[P_SETUP].mpeek << unit << std::endl;
	// std::cout << "Setup Difference=\t\t" << m_tMem[P_SETUP].peekmem << unit << std::endl;
    // std::cout << "Online Begin=\t" << m_tMem[P_ONLINE].mbegin << unit << std::endl;
	// std::cout << "Online End=\t" << m_tMem[P_ONLINE].mpeek << unit << std::endl;
	// std::cout << "Online Difference=\t" << m_tMem[P_ONLINE].peekmem << unit << std::endl;
}