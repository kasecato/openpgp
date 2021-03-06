/*
 Copyright 2019 SoloKeys Developers

 Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
 http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
 http://opensource.org/licenses/MIT>, at your option. This file may not be
 copied, modified, or distributed except according to those terms.
 */

#ifndef SRC_APDUEXECUTOR_H_
#define SRC_APDUEXECUTOR_H_

#include <cstdint>
#include <cstdlib>
#include "opgputil.h"
#include "errors.h"
#include "applications/applicationstorage.h"
#include "applications/apduconst.h"

namespace Application {

class APDUExecutor {
private:
	void SetResultError(bstr &result, Util::Error error);
public:
    APDUExecutor();
    
	Util::Error Execute(bstr apdu, bstr &result);
};

} /* namespace OpenPGP */

#endif /* SRC_APDUEXECUTOR_H_ */
