from tlv import *
from re import match, DOTALL


def get_data_object(card, tag):
    tagh = tag >> 8
    tagl = tag & 0xff
    return card.cmd_get_data(tagh, tagl)


def check_null(data_object):
    return data_object == None or len(data_object) == 0


def check_zeroes(data_object):
    for c in data_object:
        if c != 0x00:
            return False
    return True


def get_pk_info(pk):
    pktlv = TLV(pk)
    #pktlv.show()
    tag81 = pktlv.search(0x81)
    tag82 = pktlv.search(0x82)
    tag86 = pktlv.search(0x86) # format `04 || x || y`
    if tag86 is None:
        assert not (tag81 is None)
        assert not (tag82 is None)
        return tag81.data, tag82.data
    else:
        return tag86.data, None


def create_ecdsa_signature(r, s):
    ktlv = TLV(b"\x30\x00")
    elm30 = ktlv.search(0x30)

    elm30.append(0x02, r)
    elm30.append(0x02, s)

    #ktlv.show()
    return ktlv.encode()


def create_ecdsa_4D_key(KeyType, PrivateKey, PublicKey):
    ktlv = TLV(b"\x4d\x00")
    elm4d = ktlv.search(0x4d)

    elm4d.append(KeyType, b"")
    elm4d.append(0x7f48, encode_taglen(0x92, len(PrivateKey)) + encode_taglen(0x99, len(PublicKey)))
    elm4d.append(0x5f48, PrivateKey + PublicKey)
    #ktlv.show()
    return ktlv.encode()


def ecdh_public_key_encode(PublicKey):
    ktlv = TLV(b"\xa6\x00")
    elm = ktlv.search(0xa6)

    elm.append(0x7f49, b"")
    elm = ktlv.search(0x7f49)
    elm.append(0x86, PublicKey)
    #ktlv.show()
    return ktlv.encode()


def check_extended_capabilities(data):
    return match(b'[\x70\x74\x75\x7f]\x00\x00[\x20\x40\x80][\x00\x04\x08\x10]\x00[\x00\x01]\xff\x01\x00', data)


def check_pw_status(data):
    return match(b'\x00...\x03[\x00\x03]\x03', data, DOTALL)
