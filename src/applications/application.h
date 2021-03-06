/*
  Copyright 2019 SoloKeys Developers

  Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
  http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
  http://opensource.org/licenses/MIT>, at your option. This file may not be
  copied, modified, or distributed except according to those terms.
 */

#ifndef SRC_APPLICATION_H_
#define SRC_APPLICATION_H_

#include <cstdint>
#include <string_view>

#include "opgputil.h"
#include "errors.h"
#include "filesystem.h"
#include "apduconst.h"

namespace Application {

class Application {
protected:
	bool selected;
	const bstr aid = "\x00"_bstr;

    // TODO: application config load/save

public:
	virtual ~Application();

	virtual Util::Error Init();

	virtual Util::Error Select(bstr &result);
	virtual Util::Error DeSelect();
	virtual bool Selected();

	virtual const bstr *GetAID();

	virtual Util::Error APDUExchange(APDUStruct &apdu, bstr &result);
};

} // namespace Application

#endif /* SRC_APPLICATION_H_ */
