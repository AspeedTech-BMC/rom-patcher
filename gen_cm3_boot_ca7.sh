#!/bin/bash
SDK=~/sdk
ENCTOOL=$SDK/tools/socsec/socsec
PLAINTEXT_BIN=./boot.bin
CIPHERTEXT_BIN=./boot_enc.bin
CM3_BIN_NAME=ast2605_ssp.bin

set -e
make
./rom_patch $CM3_BIN_NAME > rom_patch.log

if [ "$1" == "sec" ]
then
	$ENCTOOL make_secure_bl1_image \
        	--algorithm RSA2048_SHA256 \
        	--bl1_image $PLAINTEXT_BIN \
        	--rsa_sign_key $SDK/configs/ast2600/security/key/test_oem_dss_private_key_2048_1.pem \
        	--output $CIPHERTEXT_BIN

	dd if=$CIPHERTEXT_BIN of=cm3_u-boot.bin bs=1; dd if=u-boot.bin of=cm3_u-boot.bin bs=1 seek=65536

else
	dd if=$PLAINTEXT_BIN of=cm3_u-boot.bin bs=1; dd if=u-boot.bin of=cm3_u-boot.bin bs=1 seek=65536
fi
