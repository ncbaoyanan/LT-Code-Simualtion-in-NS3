/*
 * ltallinone.h
 *
 *  Created on: Nov 22, 2013
 *      Author: bao
 */

#ifndef LTALLINONE_H_
#define LTALLINONE_H_


#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef BOOL
#define BOOL bool
#endif


typedef unsigned long DWORD;

namespace ns3 {
/**LT code encoding function**/
/**buf: source data that needs to be encoded.**/
/**buf_send: (the encoded data that is going to be send)**/
/**dwFileSize: source data length (in Bytes)**/
/**LTSendSize: encoded data length (in Bytes). (Typically, LTSendSize = 1.2*dwFileSize, which means 1.2 times data is needed to decode the source file.)**/
void LT_Encode(char* buf_send,char* buf,DWORD dwFileSize,DWORD LTSendSize);



/**LT code decoding function**/
/**buf: decoded data that needs to be saved.**/
/**buf_Receive: (the received data that needs to be decoded)**/
/**m_dwFileSize: source data length (in Bytes)**/
/**m_LTSendSize: encoded data length (in Bytes)**/
BOOL LT_Decode(char* buf,char* buf_Receive,DWORD m_dwFileSize,DWORD m_LTSendSize);

}


#endif /* LTALLINONE_H_ */
