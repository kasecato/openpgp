/*
 Copyright 2019 SoloKeys Developers

 Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
 http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
 http://opensource.org/licenses/MIT>, at your option. This file may not be
 copied, modified, or distributed except according to those terms.
 */
 
#ifndef _OPENPGPLIB_H_
#define _OPENPGPLIB_H_

#include "stdint.h"
#include "stddef.h"

#ifdef __cplusplus
extern "C" {
#endif
    extern bool DoReset;
    
	void OpenpgpInit();
	void OpenpgpExchange(uint8_t *datain, size_t datainlen, uint8_t *dataout, uint32_t *outlen);

#ifdef __cplusplus
}
#endif

#endif
