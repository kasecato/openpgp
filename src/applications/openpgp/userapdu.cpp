/*
  Copyright 2019 SoloKeys Developers

  Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
  http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
  http://opensource.org/licenses/MIT>, at your option. This file may not be
  copied, modified, or distributed except according to those terms.
 */

#include <applications/openpgp/security.h>
#include "opgpdevice.h"
#include "userapdu.h"
#include "applications/apduconst.h"
#include "solofactory.h"
#include "applications/openpgp/openpgpfactory.h"
#include "applications/openpgpapplication.h"
#include "openpgpconst.h"
#include "openpgpstruct.h"
#include "filesystem.h"

namespace OpenPGP {

Util::Error APDUVerify::Check(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2) {
    if (ins != Application::APDUcommands::Verify)
		return Util::Error::WrongCommand;

	if (cla != 0x00 && cla != 0x0c)
		return Util::Error::WrongAPDUCLA;

	if ((p1 != 0x00 && p1 != 0xff) ||
		(p2 != 0x81 && p2 != 0x82 && p2 != 0x83))
		return Util::Error::WrongAPDUP1P2;

	return Util::Error::NoError;
}

Util::Error APDUVerify::Process(uint8_t cla, uint8_t ins, uint8_t p1,
		uint8_t p2, bstr data, uint8_t le, bstr &dataOut) {

	auto err = Check(cla, ins, p1, p2);
	if (err != Util::Error::NoError)
		return err;

	if (p1 == 0xff && data.length() > 0)
		return Util::Error::WrongAPDULength;

	Factory::SoloFactory &solo = Factory::SoloFactory::GetSoloFactory();
	OpenPGP::OpenPGPFactory &opgp_factory = solo.GetOpenPGPFactory();
	OpenPGP::Security &security = opgp_factory.GetSecurity();

	Password passwd_id = Password::PSOCDS; // p2 == 0x81
	if (p2 == 0x82)
		passwd_id = Password::PW1;
	if (p2 == 0x83)
		passwd_id = Password::PW3;

	// clear authentication status
	if (p1 == 0xff){
		security.ClearAuth(passwd_id);
		return Util::Error::NoError;
	}

	// check status. OpenPGP v3.3.1 page 44. if input data length == 0, return authentication status
	if (data.length() == 0) {
		if (security.GetAuth(passwd_id)) {
			return Util::Error::NoError;
		} else {
			dataOut.appendAPDUres(0x6300 + security.PasswdTryRemains(passwd_id));
			return Util::Error::ErrorPutInData;
		}
	}

	// verify password (strict check)
	return security.VerifyPasswd(passwd_id, data, false, nullptr);
}

std::string_view APDUVerify::GetName() {
	using namespace std::literals;
	return "Verify"sv;
}

Util::Error APDUChangeReferenceData::Check(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2) {
    if (ins != Application::APDUcommands::ChangeReferenceData)
		return Util::Error::WrongCommand;

	if (cla != 0x00 && cla != 0x0c)
		return Util::Error::WrongAPDUCLA;

	if ((p1 != 0x00) ||
		(p2 != 0x81 && p2 != 0x83))
		return Util::Error::WrongAPDUP1P2;

	return Util::Error::NoError;
}

Util::Error APDUChangeReferenceData::Process(uint8_t cla, uint8_t ins,
		uint8_t p1, uint8_t p2, bstr data, uint8_t le, bstr &dataOut) {

	Factory::SoloFactory &solo = Factory::SoloFactory::GetSoloFactory();
	OpenPGP::OpenPGPFactory &opgp_factory = solo.GetOpenPGPFactory();
	OpenPGP::Security &security = opgp_factory.GetSecurity();

	auto err_check = Check(cla, ins, p1, p2);
	if (err_check != Util::Error::NoError)
		return err_check;

	Password passwd_id = Password::PW1;
	if (p2 == 0x83)
		passwd_id = Password::PW3;

	size_t passwd_length = 0;
	auto err = security.VerifyPasswd(passwd_id, data, true, &passwd_length);
	if (err != Util::Error::NoError)
		return err;

	// set new password
	err = security.SetPasswd(passwd_id, data.substr(passwd_length, data.length() - passwd_length));
	if (err != Util::Error::NoError)
		return err;

	return Util::Error::NoError;
}

std::string_view APDUChangeReferenceData::GetName() {
	using namespace std::literals;
	return "ChangeReferenceData"sv;
}

Util::Error APDUResetRetryCounter::Check(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2) {
    if (ins != Application::APDUcommands::ResetRetryCounter)
		return Util::Error::WrongCommand;

	if (cla != 0x00 && cla != 0x0c)
		return Util::Error::WrongAPDUCLA;

	if ((p1 != 0x00 && p1 != 0x02) ||
		(p2 != 0x81))
		return Util::Error::WrongAPDUP1P2;

	return Util::Error::NoError;
}

Util::Error APDUResetRetryCounter::Process(uint8_t cla, uint8_t ins,
		uint8_t p1, uint8_t p2, bstr data, uint8_t le, bstr &dataOut) {

	Factory::SoloFactory &solo = Factory::SoloFactory::GetSoloFactory();
	OpenPGP::OpenPGPFactory &opgp_factory = solo.GetOpenPGPFactory();
	OpenPGP::Security &security = opgp_factory.GetSecurity();

	auto err = Check(cla, ins, p1, p2);
	if (err != Util::Error::NoError)
		return err;

	bstr passwd;

	// 0x02 - after correct verification of PW3
	// 0x00 - resetting code (RC) in data
	if (p1 == 0x02) {
		if (!security.GetAuth(Password::PW3))
			return Util::Error::AccessDenied;

		passwd = data;
	} else {
		size_t rc_length = 0;
		auto err = security.VerifyPasswd(Password::RC, data, true, &rc_length);
		if (err != Util::Error::NoError)
			return err;

		passwd = data.substr(rc_length, data.length() - rc_length);
	}

	err = security.SetPasswd(Password::PW1, passwd);
	if (err != Util::Error::NoError)
		return err;

	// not in standard.
	security.ClearAuth(Password::PW1);

	// GNUK clear PW3 password if pw3 is empty
	if (security.PWIsEmpty(Password::PW3))
		security.ClearAuth(Password::PW3);

	return Util::Error::NoError;
}

std::string_view APDUResetRetryCounter::GetName() {
	using namespace std::literals;
	return "ResetRetryCounter"sv;
}

// Open PGP application v 3.3.1 page 49
Util::Error APDUGetData::Check(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2) {
    if (ins != Application::APDUcommands::GetData && ins != Application::APDUcommands::GetData2)
		return Util::Error::WrongCommand;

	if (cla != 0x00 && cla != 0x0c)
		return Util::Error::WrongAPDUCLA;

	return Util::Error::NoError;
}

Util::Error APDUGetData::Process(uint8_t cla, uint8_t ins, uint8_t p1,
		uint8_t p2, bstr data, uint8_t le, bstr &dataOut) {

	Factory::SoloFactory &solo = Factory::SoloFactory::GetSoloFactory();
	File::FileSystem &filesystem = solo.GetFileSystem();

	auto err_check = Check(cla, ins, p1, p2);
	if (err_check != Util::Error::NoError)
		return err_check;

	uint16_t object_id = (p1 << 8) + p2;
	printf_device("read object id = 0x%04x\n", object_id);

	filesystem.ReadFile(File::AppID::OpenPGP, object_id, File::File, dataOut);

	return Util::Error::NoError;
}

std::string_view APDUGetData::GetName() {
	using namespace std::literals;
	return "GetData"sv;
}

Util::Error APDUPutData::Check(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2) {
    if (ins != Application::APDUcommands::PutData && ins != Application::APDUcommands::PutData2)
		return Util::Error::WrongCommand;

    if (ins == Application::APDUcommands::PutData2 && (p1 != 0x3f || p2 != 0xff))
		return Util::Error::WrongCommand;

	if (cla != 0x00 && cla != 0x0c && cla != 0x10)
		return Util::Error::WrongAPDUCLA;

	return Util::Error::NoError;
}

Util::Error APDUPutData::Process(uint8_t cla, uint8_t ins, uint8_t p1,
		uint8_t p2, bstr data, uint8_t le, bstr &dataOut) {

	dataOut.clear();

	Factory::SoloFactory &solo = Factory::SoloFactory::GetSoloFactory();
	File::FileSystem &filesystem = solo.GetFileSystem();
	Crypto::KeyStorage &key_storage = solo.GetKeyStorage();
	OpenPGP::OpenPGPFactory &opgp_factory = solo.GetOpenPGPFactory();
	OpenPGP::Security &security = opgp_factory.GetSecurity();

	auto err_check = Check(cla, ins, p1, p2);
	if (err_check != Util::Error::NoError)
		return err_check;

    if (ins == Application::APDUcommands::PutData) {
		uint16_t object_id = (p1 << 8) + p2;
		printf_device("write object id = 0x%04x\n", object_id);

		if (OpenPGP::PGPConst::ReadWriteOnlyAllowedFiles) {
			err_check = security.DataObjectInAllowedList(object_id);
			if (err_check != Util::Error::NoError)
				return err_check;
		}

		// check cardholder certificate max length
		if (data.length() > PGPConst::MaxCardholderCertificateLen && object_id == 0x7f21)
			return Util::Error::WrongAPDUDataLength;

		// check max length
		if (data.length() > PGPConst::MaxSpecialDOLen &&
			(object_id == 0x0101 || object_id == 0x0102 || object_id == 0x0103 || object_id == 0x0104 ||
			 object_id == 0x5e ||
			 object_id == 0xf50 ||
			 object_id == 0xf9)
		   )
			return Util::Error::WrongAPDUDataLength;

		// check if we set correct algorithm attributes
		if (object_id == 0xc1 || object_id == 0xc2 || object_id == 0xc3) {
			if (data.length() > PGPConst::MaxSpecialDOLen)
				return Util::Error::WrongAPDUDataLength;

			AlgoritmAttr aa;
			auto err = aa.DecodeData(data, object_id);
			if (err != Util::Error::NoError)
				return err;

		}

		// check AES key correct length
		if (object_id == 0xd5 &&
			data.length() != 16 && data.length() != 24 && data.length() != 32)
			return Util::Error::WrongAPDUDataLength;

		auto area = security.DataObjectInSecureArea(object_id) ? File::Secure : File::File;
		auto err = filesystem.WriteFile(File::AppID::OpenPGP, object_id, area, data);
		if (err != Util::Error::NoError)
			return err;

		// refresh objects and some logic after saving data to filesystem
		err = security.AfterSaveFileLogic(object_id);
		if (err != Util::Error::NoError)
			return err;
	} else {
		printf_device("write KeyExtHeader\n");
		key_storage.SetKeyExtHeader(File::AppID::OpenPGP, data);
	}

	return Util::Error::NoError;
}

std::string_view APDUPutData::GetName() {
	using namespace std::literals;
	return "PutData"sv;
}

} // namespace OpenPGP
