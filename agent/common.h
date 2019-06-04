/*
 * common.h
 *
 *  Created on: 2019. 5. 15.
 *      Author: kwanghun_choi@tmax.co.kr
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <iostream>

//#define DEBUG

#ifdef DEBUG
#define LOG(msg) std::cout << msg << "\n";
#define LOG2(msg1, msg2) std::cout << msg1 << msg2 << "\n";
#define LOG_ERR(msg) std::cerr << msg << "\n";
#define LOG_ERR2(msg1, msg2) std::cerr << msg1 << msg2 << "\n";
#else
#define LOG(msg)
#define LOG2(msg1, msg2)
#define LOG_ERR(msg)
#define LOG_ERR2(msg1, msg2)
#endif

#endif /* COMMON_H_ */
