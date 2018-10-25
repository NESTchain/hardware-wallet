/******************************************
* fm_se.h
* Created on: 20181012
* Author: clu
*******************************************/

#ifndef _FM_SE_H
#define _FM_SE_H

void fm_se_init(void);
void fm_se_get_id(unsigned char* p_device_id);
unsigned char fm_se_random_128bits(unsigned char* p_random);
unsigned char fm_se_ecc_generate_key(void);
unsigned char fm_se_ecc_export_public_key(unsigned char* p_public_key);
unsigned char fm_se_ecdsa_sign(unsigned char* p_sha256, unsigned char* p_signature);
unsigned char fm_se_ecdsa_verify(unsigned char* p_signature, unsigned char* p_sha256);
unsigned char fm_se_ecc_import_private_key(unsigned char* p_private_key);
unsigned char fm_se_ecc_import_public_key(unsigned char* p_public_key);

#endif
