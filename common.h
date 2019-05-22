/*
 * common.h
 *
 *  Created on: 2019. 5. 15.
 *      Author: kwanghun_choi@tmax.co.kr
 */

#ifndef COMMON_H_
#define COMMON_H_

#define DEBUG

#ifdef DEBUG
#define LOG(msg) std::cout << msg << "\n"
#define LOG_ERR(msg) std::cerr << msg << "\n";
#else
#define LOG(msg)
#define LOG_ERR(msg)
#endif

#endif /* COMMON_H_ */
