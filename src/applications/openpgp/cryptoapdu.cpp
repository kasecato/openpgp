/*
  Copyright 2019 SoloKeys Developers

  Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
  http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
  http://opensource.org/licenses/MIT>, at your option. This file may not be
  copied, modified, or distributed except according to those terms.
 */

#include <applications/openpgp/security.h>
#include "cryptoapdu.h"
#include "tlv.h"
#include "opgpdevice.h"
#include "applications/apduconst.h"
#include "solofactory.h"
#include "applications/openpgp/openpgpfactory.h"
#include "applications/openpgpapplication.h"
#include "applications/openpgp/openpgpconst.h"
#include "applications/openpgp/openpgpstruct.h"

namespace OpenPGP {

Util::Error APDUGetChallenge::Check(uint8_t cla, uint8_t ins,
		uint8_t p1, uint8_t p2) {
	if (ins != Application::APDUcommands::GetChallenge)
		return Util::Error::WrongCommand;

	if (cla != 0x00)
		return Util::Error::WrongAPDUCLA;

	if (p1 != 0x00 && p2 != 0x00)   // encipher
		return Util::Error::WrongAPDUP1P2;

	return Util::Error::NoError;
}

Util::Error APDUGetChallenge::Process(uint8_t cla, uint8_t ins,
		uint8_t p1, uint8_t p2, bstr data, uint8_t le, bstr& dataOut) {

	dataOut.clear();

	auto err_check = Check(cla, ins, p1, p2);
	if (err_check != Util::Error::NoError)
		return err_check;

	if (data.length() > 0)
		return Util::Error::WrongAPDUDataLength;


	Factory::SoloFactory &solo = Factory::SoloFactory::GetSoloFactory();
	Crypto::CryptoLib &crypto = solo.GetCryptoLib();

	if (le == 0)
		le = 0xff;

	return crypto.GenerateRandom(le, dataOut);
}

std::string_view APDUGetChallenge::GetName() {
	using namespace std::literals;
	return "GetChallenge"sv;
}

Util::Error APDUInternalAuthenticate::Check(uint8_t cla, uint8_t ins,
		uint8_t p1, uint8_t p2) {

	if (ins != Application::APDUcommands::InternalAuthenticate)
		return Util::Error::WrongCommand;

	if (cla != 0x00)
		return Util::Error::WrongAPDUCLA;

	if ((p1 != 0x00 || p2 != 0x00))
		return Util::Error::WrongAPDUP1P2;

	return Util::Error::NoError;
}

// OpenPGP 3.3.1 page 61
Util::Error APDUInternalAuthenticate::Process(uint8_t cla, uint8_t ins,
		uint8_t p1, uint8_t p2, bstr data, uint8_t le, bstr& dataOut) {

	auto err_check = Check(cla, ins, p1, p2);
	if (err_check != Util::Error::NoError)
		return err_check;

	Factory::SoloFactory &solo = Factory::SoloFactory::GetSoloFactory();
	File::FileSystem &filesystem = solo.GetFileSystem();
	Crypto::CryptoEngine &crypto_e = solo.GetCryptoEngine();
	OpenPGP::OpenPGPFactory &opgp_factory = solo.GetOpenPGPFactory();
	OpenPGP::Security &security = opgp_factory.GetSecurity();

	if (!security.GetAuth(OpenPGP::Password::PW1))
		return Util::Error::AccessDenied;

	OpenPGP::AlgoritmAttr alg;
	auto err = alg.Load(filesystem, 0xc3); // authentication
	if (err != Util::Error::NoError || alg.AlgorithmID == 0)
		return Util::Error::DataNotFound;

	if (alg.AlgorithmID == Crypto::AlgoritmID::RSA)
        err = crypto_e.RSASign(File::AppID::OpenPGP, OpenPGPKeyType::Authentication, data, dataOut);
	else
        err = crypto_e.ECCSign(File::AppID::OpenPGP, OpenPGPKeyType::Authentication, data, dataOut);

	return err;
}

std::string_view APDUInternalAuthenticate::GetName() {
	using namespace std::literals;
	return "InternalAuthenticate"sv;
}

Util::Error APDUGenerateAsymmetricKeyPair::Check(uint8_t cla,
		uint8_t ins, uint8_t p1, uint8_t p2) {

	if (ins != Application::APDUcommands::GenerateAsymmKeyPair)
		return Util::Error::WrongCommand;

	if (cla != 0x00 && cla != 0x0c)
		return Util::Error::WrongAPDUCLA;

	if ((p1 != 0x80 && p1 != 0x81) ||
		(p2 != 0x00))
		return Util::Error::WrongAPDUP1P2;

	return Util::Error::NoError;
}

Util::Error APDUGenerateAsymmetricKeyPair::Process(uint8_t cla,
		uint8_t ins, uint8_t p1, uint8_t p2, bstr data, uint8_t le, bstr& dataOut) {

	dataOut.clear();

	auto err_check = Check(cla, ins, p1, p2);
	if (err_check != Util::Error::NoError)
		return err_check;

	if (data.length() != 2)
		return Util::Error::WrongAPDUDataLength;

	Factory::SoloFactory &solo = Factory::SoloFactory::GetSoloFactory();
	File::FileSystem &filesystem = solo.GetFileSystem();
	Crypto::KeyStorage &key_storage = solo.GetKeyStorage();
	Crypto::CryptoLib &cryptolib = solo.GetCryptoLib();

	OpenPGPKeyType key_type = OpenPGPKeyType::Unknown;
	if (data[0] == OpenPGPKeyType::DigitalSignature ||
		data[0] == OpenPGPKeyType::Confidentiality ||
		data[0] == OpenPGPKeyType::Authentication
		)
		key_type = static_cast<OpenPGPKeyType>(data[0]);

	KeyID_t file_id = 0;
	switch (key_type) {
	case OpenPGPKeyType::DigitalSignature:
		file_id = 0xc1;
		break;
	case OpenPGPKeyType::Confidentiality:
		file_id = 0xc2;
		break;
	case OpenPGPKeyType::Authentication:
		file_id = 0xc3;
		break;
	default:
		break;
	};
	if (file_id == 0)
		return Util::Error::DataNotFound;

	printf_device("fileid = 0x%02x\n", file_id);
	OpenPGP::AlgoritmAttr alg;
	auto err = alg.Load(filesystem, file_id);
	if (err != Util::Error::NoError || alg.AlgorithmID == 0)
		return Util::Error::DataNotFound;

	// OpenPGP v3.3.1 page 64
	// 0x80 - Generation of key pair
	// 0x81 - Reading of actual public key template
	if (p1 == 0x80) {
		if (alg.AlgorithmID == Crypto::AlgoritmID::RSA) {
			printf_device("RSA\n");
			Crypto::RSAKey rsa_key;
			err = cryptolib.RSAGenKey(rsa_key, alg.RSAa.NLen);
			if (err != Util::Error::NoError)
				return err;

            err = key_storage.PutRSAFullKey(File::AppID::OpenPGP, key_type, rsa_key);
			if (err != Util::Error::NoError)
				return err;

            err = key_storage.GetPublicKey7F49(File::AppID::OpenPGP, key_type, alg.AlgorithmID, dataOut);
			if (err != Util::Error::NoError)
				return err;

			return Util::Error::NoError;
		}

        if (alg.AlgorithmID == Crypto::AlgoritmID::ECDSAforCDSandIntAuth ||
            alg.AlgorithmID == Crypto::AlgoritmID::EDDSA) {
			printf_device("ECDSA\n");
			Crypto::ECCKey ecdsa_key;
            err = cryptolib.ECCGenKey(key_storage.GetECCCurveID(File::AppID::OpenPGP, file_id), ecdsa_key);
			if (err != Util::Error::NoError)
				return err;

            err = key_storage.PutECCFullKey(File::AppID::OpenPGP, key_type, ecdsa_key);
			if (err != Util::Error::NoError)
				return err;

            err = key_storage.GetPublicKey7F49(File::AppID::OpenPGP, key_type, alg.AlgorithmID, dataOut);
			if (err != Util::Error::NoError)
				return err;

			return Util::Error::NoError;
		}

		return Util::Error::DataNotFound;
	} else {
		printf_device("GetKey only\n");
		err = key_storage.GetPublicKey7F49(
                File::AppID::OpenPGP,
				key_type,
				alg.AlgorithmID,
				dataOut);
		if (err != Util::Error::NoError || dataOut.length() == 0)
			return Util::Error::DataNotFound;

		return Util::Error::NoError;
	}

	return Util::Error::NoError;
}

std::string_view APDUGenerateAsymmetricKeyPair::GetName() {
	using namespace std::literals;
	return "GenerateAsymmetricKeyPair"sv;
}

Util::Error APDUPSO::Check(uint8_t cla, uint8_t ins, uint8_t p1,
		uint8_t p2) {
	if (ins != Application::APDUcommands::PSO)
		return Util::Error::WrongCommand;

	if (cla != 0x00)
		return Util::Error::WrongAPDUCLA;

	if (!((p1 == 0x9e && p2 == 0x9a) ||  // compute digital signature
		  (p1 == 0x80 && p2 == 0x86) ||  // decipher
		  (p1 == 0x86 && p2 == 0x80)))   // encipher
		return Util::Error::WrongAPDUP1P2;

	return Util::Error::NoError;
}

// OpenPGP v3.3.1. page 53
Util::Error APDUPSO::Process(uint8_t cla, uint8_t ins, uint8_t p1,
		uint8_t p2, bstr data, uint8_t le, bstr& dataOut) {

	dataOut.clear();

	Factory::SoloFactory &solo = Factory::SoloFactory::GetSoloFactory();
	File::FileSystem &filesystem = solo.GetFileSystem();
	Crypto::CryptoEngine &crypto_e = solo.GetCryptoEngine();
	OpenPGP::OpenPGPFactory &opgp_factory = solo.GetOpenPGPFactory();
	OpenPGP::Security &security = opgp_factory.GetSecurity();

	auto err_check = Check(cla, ins, p1, p2);
	if (err_check != Util::Error::NoError)
		return err_check;

	PWStatusBytes pwstatus;
	pwstatus.Load(filesystem);

	//PSO:CDS OpenPGP 3.3.1 page 53. iso 7816-8:2004 page 6-8
	if (p1 == 0x9e && p2 == 0x9a) {
		if (!security.GetAuth(OpenPGP::Password::PSOCDS))
			return Util::Error::AccessDenied;

		OpenPGP::AlgoritmAttr alg;
		auto err = alg.Load(filesystem, 0xc1); // DigitalSignature
		if (err != Util::Error::NoError || alg.AlgorithmID == 0)
			return Util::Error::DataNotFound;

		if (alg.AlgorithmID == Crypto::AlgoritmID::RSA)
            err = crypto_e.RSASign(File::AppID::OpenPGP, OpenPGPKeyType::DigitalSignature, data, dataOut);
		else
            err = crypto_e.ECCSign(File::AppID::OpenPGP, OpenPGPKeyType::DigitalSignature, data, dataOut);

		if (!pwstatus.PW1ValidSeveralCDS)
			security.ClearAuth(OpenPGP::Password::PSOCDS);

		// DS-Counter
		auto cntrerr = security.IncDSCounter();
		if (cntrerr != Util::Error::NoError)
			return cntrerr;

		// clear CDS flag if sign can't be done too
		if (err != Util::Error::NoError)
			return err;
	}

	// 	PSO:DECIPHER OpenPGP 3.3.1 page 57. iso 7816-8:2004 page 6-8
	if (p1 == 0x80 && p2 == 0x86) {
		if (!security.GetAuth(OpenPGP::Password::PW1))
			return Util::Error::AccessDenied;

		OpenPGP::AlgoritmAttr alg;
		auto err = alg.Load(filesystem, 0xc2); // Confidentiality
		if (err != Util::Error::NoError || alg.AlgorithmID == 0)
			return Util::Error::DataNotFound;

		// RSA. OpenPGP 3.3.1 page 59
		if (data[0] == 0x00) {
			if (alg.AlgorithmID == Crypto::AlgoritmID::RSA) {
                err = crypto_e.RSADecipher(File::AppID::OpenPGP, OpenPGPKeyType::Confidentiality, data.substr(1, data.length() - 1), dataOut);
			} else {
				return Util::Error::ConditionsNotSatisfied;
			}
		}

		// AES decrypt. OpenPGP 3.3.1 page 59
		if (data[0] == 0x02) {
			// OpenPGP application Version 3.3.1 page 58
			if ((data.length() - 1) % 16)
				return Util::Error::CryptoDataError;

            err = crypto_e.AESDecrypt(File::AppID::OpenPGP, OpenPGPKeyType::AES, data.substr(1, data.length() - 1), dataOut);
			if (err != Util::Error::NoError)
				return err;
		}

		// ECDH. OpenPGP 3.3.1 page 59
		if (data[0] == 0xa6) {
			// data - a6 - 7f49 - 86
			using namespace Util;

			TLVTree tlv;
			auto err = tlv.Init(data);
			if (err != Util::Error::NoError)
				return err;

			TLVElm *tlvpk = tlv.Search(0x86);
			if (!tlvpk || tlvpk->Length() == 0)
				return Util::Error::CryptoDataError;

            err = crypto_e.ECDHComputeShared(File::AppID::OpenPGP, OpenPGPKeyType::Confidentiality, tlvpk->GetData(), dataOut);
			if (err != Util::Error::NoError)
				return err;

			return Util::Error::NoError;
		}

		if (err != Util::Error::NoError)
			return err;
	}

	// 	PSO:ENCIPHER OpenPGP 3.3.1 page 60. iso 7816-8:2004 page 6-8
	if (p1 == 0x86 && p2 == 0x80) {
		if (data.length() % 16)
			return Util::Error::CryptoDataError;

		// append padding byte. OpenPGP 3.3.1 page 60.
		dataOut.append(0x02);

		bstr aesres = bstr(dataOut.uint8Data() + 1, 0, dataOut.free_space());
        auto err = crypto_e.AESEncrypt(File::AppID::OpenPGP, OpenPGPKeyType::AES, data, aesres);
		if (err != Util::Error::NoError)
			return err;

		dataOut.set_length(1 + aesres.length());
	}

	return Util::Error::NoError;
}

std::string_view APDUPSO::GetName() {
	using namespace std::literals;
	return "PSO(Perform Security Operation)"sv;
}

} // namespace OpenPGP
