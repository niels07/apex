#include <string.h>
#include <crypt.h>
#include <time.h>
#include <stdint.h>
#include "apexVM.h"
#include "apexMem.h"
#include "apexVal.h"
#include "apexLib.h"
#include "apexErr.h"

#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 16
#define AES_ROUNDS 10

static const uint8_t AES_SBOX[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const uint8_t AES_RCON[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

void aes_sub_bytes(uint8_t state[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[i][j] = AES_SBOX[state[i][j]];
        }
    }
}

void aes_inv_sub_bytes(uint8_t state[4][4]) {
    static const uint8_t inv_sbox[256] = {
        // Inverse S-box values
        0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
        0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
        0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
        0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
        0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
        0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
        0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
        0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
        0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
        0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
        0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
        0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
        0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
        0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
        0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
        0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
    };
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[i][j] = inv_sbox[state[i][j]];
        }
    }
}

void aes_shift_rows(uint8_t state[4][4]) {
    uint8_t temp;
    temp = state[1][0];
    state[1][0] = state[1][1];
    state[1][1] = state[1][2];
    state[1][2] = state[1][3];
    state[1][3] = temp;

    temp = state[2][0];
    uint8_t temp2 = state[2][1];
    state[2][0] = state[2][2];
    state[2][1] = state[2][3];
    state[2][2] = temp;
    state[2][3] = temp2;

    temp = state[3][3];
    state[3][3] = state[3][2];
    state[3][2] = state[3][1];
    state[3][1] = state[3][0];
    state[3][0] = temp;
}

void aes_inv_shift_rows(uint8_t state[4][4]) {
    uint8_t temp;

    // Row 1: rotate right by 1
    temp = state[1][3];
    state[1][3] = state[1][2];
    state[1][2] = state[1][1];
    state[1][1] = state[1][0];
    state[1][0] = temp;

    // Row 2: rotate right by 2
    temp = state[2][0];
    uint8_t temp2 = state[2][1];
    state[2][0] = state[2][2];
    state[2][1] = state[2][3];
    state[2][2] = temp;
    state[2][3] = temp2;

    // Row 3: rotate right by 3
    temp = state[3][0];
    state[3][0] = state[3][1];
    state[3][1] = state[3][2];
    state[3][2] = state[3][3];
    state[3][3] = temp;
}

void aes_add_round_key(uint8_t state[4][4], const uint8_t *round_key) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[i][j] ^= round_key[i * 4 + j];
        }
    }
}

uint8_t aes_gf_mul(uint8_t x, uint8_t y) {
    uint8_t result = 0;
    while (y) {
        if (y & 1) result ^= x;
        x = (x << 1) ^ (x & 0x80 ? 0x1b : 0);
        y >>= 1;
    }
    return result;
}

void aes_mix_columns(uint8_t state[4][4]) {
    uint8_t temp[4];
    for (int i = 0; i < 4; i++) {
        temp[0] = aes_gf_mul(0x02, state[0][i]) ^ aes_gf_mul(0x03, state[1][i]) ^ state[2][i] ^ state[3][i];
        temp[1] = state[0][i] ^ aes_gf_mul(0x02, state[1][i]) ^ aes_gf_mul(0x03, state[2][i]) ^ state[3][i];
        temp[2] = state[0][i] ^ state[1][i] ^ aes_gf_mul(0x02, state[2][i]) ^ aes_gf_mul(0x03, state[3][i]);
        temp[3] = aes_gf_mul(0x03, state[0][i]) ^ state[1][i] ^ state[2][i] ^ aes_gf_mul(0x02, state[3][i]);
        for (int j = 0; j < 4; j++) {
            state[j][i] = temp[j];
        }
    }
}



/**
 * Inverse Mix Columns transformation for AES decryption.
 *
 * This function applies the inverse Mix Columns transformation to the state
 * matrix, which is part of the AES decryption algorithm. It takes the state
 * matrix as input, and produces the transformed state matrix as output.
 *
 * @param state The state matrix to be transformed, which is a 4x4 matrix of
 *              bytes.
 */
void aes_inv_mix_columns(uint8_t state[4][4]) {
    uint8_t temp[4];
    for (int i = 0; i < 4; i++) {
        temp[0] = aes_gf_mul(0x0e, state[0][i]) ^ aes_gf_mul(0x0b, state[1][i]) ^
                  aes_gf_mul(0x0d, state[2][i]) ^ aes_gf_mul(0x09, state[3][i]);
        temp[1] = aes_gf_mul(0x09, state[0][i]) ^ aes_gf_mul(0x0e, state[1][i]) ^
                  aes_gf_mul(0x0b, state[2][i]) ^ aes_gf_mul(0x0d, state[3][i]);
        temp[2] = aes_gf_mul(0x0d, state[0][i]) ^ aes_gf_mul(0x09, state[1][i]) ^
                  aes_gf_mul(0x0e, state[2][i]) ^ aes_gf_mul(0x0b, state[3][i]);
        temp[3] = aes_gf_mul(0x0b, state[0][i]) ^ aes_gf_mul(0x0d, state[1][i]) ^
                  aes_gf_mul(0x09, state[2][i]) ^ aes_gf_mul(0x0e, state[3][i]);

        for (int j = 0; j < 4; j++) {
            state[j][i] = temp[j];
        }
    }
}

/**
 * Expands the AES encryption key into a series of round keys.
 *
 * This function performs the key scheduling operation for the AES
 * encryption algorithm. It generates a series of round keys from the
 * original encryption key, which are used in the encryption and
 * decryption processes.
 *
 * @param key The original AES key, which is expected to be 16 bytes (128 bits) long.
 * @param round_keys A buffer where the expanded round keys will be stored. 
 *                   This buffer should be at least 176 bytes long.
 */
void aes_key_expansion(const uint8_t *key, uint8_t *round_keys) {
    memcpy(round_keys, key, AES_KEY_SIZE);
    memset(round_keys, 0, 176);
    for (int i = 1; i <= AES_ROUNDS; i++) {
        uint8_t temp[4];
        temp[0] = AES_SBOX[round_keys[(i - 1) * 16 + 13]] ^ AES_RCON[i];
        temp[1] = AES_SBOX[round_keys[(i - 1) * 16 + 14]];
        temp[2] = AES_SBOX[round_keys[(i - 1) * 16 + 15]];
        temp[3] = AES_SBOX[round_keys[(i - 1) * 16 + 12]];

        for (int j = 0; j < 4; j++) {
            round_keys[i * 16 + j] = round_keys[(i - 1) * 16 + j] ^ temp[j];
        }

        for (int j = 4; j < 16; j++) {
            round_keys[i * 16 + j] = round_keys[(i - 1) * 16 + j] ^ round_keys[i * 16 + j - 4];
        }
    }
}

/**
 * Encrypts a single block of data using the AES encryption algorithm.
 *
 * This function encrypts a single block of data (16 bytes) using the AES
 * encryption algorithm. It takes the plaintext data and a key as input, and
 * produces the corresponding ciphertext data as output.
 *
 * @param input The plaintext data block to be encrypted, which should be
 *              16 bytes long.
 * @param output A buffer where the ciphertext data block will be stored,
 *               which should also be 16 bytes long.
 * @param key The AES encryption key, which should be 16 bytes long.
 */
void aes_block(const uint8_t *input, uint8_t *output, const uint8_t *key) {
    uint8_t state[4][4];
    uint8_t round_keys[176];
    aes_key_expansion(key, round_keys);

    // Initialize state matrix
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[j][i] = input[i * 4 + j];
        }
    }

    aes_add_round_key(state, round_keys);

    for (int round = 1; round < AES_ROUNDS; round++) {
        aes_sub_bytes(state);
        aes_shift_rows(state);
        aes_mix_columns(state);
        aes_add_round_key(state, round_keys + round * 16);
    }

    aes_sub_bytes(state);
    aes_shift_rows(state);
    aes_add_round_key(state, round_keys + AES_ROUNDS * 16);

    // Copy state matrix to output buffer
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            output[i * 4 + j] = state[j][i];
        }
    }
}

/**
 * Decrypts a single block of data using the AES decryption algorithm.
 *
 * This function decrypts a single block of data (16 bytes) using the AES
 * decryption algorithm. It takes the ciphertext data and a key as input, and
 * produces the corresponding plaintext data as output.
 *
 * @param input The ciphertext data block to be decrypted, which should be
 *              16 bytes long.
 * @param output A buffer where the plaintext data block will be stored,
 *               which should also be 16 bytes long.
 * @param key The AES encryption key, which should be 16 bytes long.
 */
void aes_inv_block(const uint8_t *input, uint8_t *output, const uint8_t *key) {
    uint8_t state[4][4];
    uint8_t round_keys[176];
    aes_key_expansion(key, round_keys);

    // Initialize state matrix from input
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[j][i] = input[i * 4 + j];
        }
    }

    // Add final round key
    aes_add_round_key(state, round_keys + AES_ROUNDS * 16);

    // Perform 9 decryption rounds (in reverse order)
    for (int round = AES_ROUNDS - 1; round > 0; round--) {
        aes_inv_shift_rows(state);
        aes_inv_sub_bytes(state);
        aes_add_round_key(state, round_keys + round * 16);
        aes_inv_mix_columns(state);
    }

    // Final round (no inv_mix_columns)
    aes_inv_shift_rows(state);
    aes_inv_sub_bytes(state);
    aes_add_round_key(state, round_keys);

    // Copy state matrix to output buffer
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            output[i * 4 + j] = state[j][i];
        }
    }
}

/**
 * Encrypts a string using the AES encryption algorithm.
 *
 * This function encrypts a given string using the AES encryption algorithm
 * with the given key. It takes two arguments: a string to encrypt and a key
 * string. The length of the key string should be 16 bytes (128 bits).
 *
 * The function returns the encrypted string in hexadecimal format.
 */
int crypt_aes(ApexVM *vm, int argc) {
    if (argc != 2) {
        apexErr_runtime(vm, "crypt:aes expects exactly 2 arguments");
        return 1;
    }
    ApexValue key_val = apexVM_pop(vm);
    if (apexVal_type(key_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "second argument to crypt:aes must be a string");
        return 1;
    }
    ApexValue str_val = apexVM_pop(vm);
    if (apexVal_type(str_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "first argument to crypt:aes must be a string");
        return 1;
    }
    char *key = apexVal_str(key_val)->value;
    char *str = apexVal_str(str_val)->value;
    size_t len = strlen(str);
    size_t padded_len = ((len + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    uint8_t *input = calloc(padded_len, sizeof(uint8_t));
    memcpy(input, str, len);

    uint8_t padding = (uint8_t)(padded_len - len);
    for (size_t i = len; i < padded_len; i++) {
        input[i] = padding;
    }

    uint8_t *output = calloc(padded_len, sizeof(uint8_t));
    for (size_t i = 0; i < padded_len; i += AES_BLOCK_SIZE) {
        aes_block(input + i, output + i, (uint8_t *)key);
    }

    char *enc = malloc(padded_len * 2 + 1);
    for (size_t i = 0; i < padded_len; i++) {
        sprintf(&enc[i * 2], "%02x", output[i]);
    }
    free(input);
    free(output);
    ApexString *enc_str = apexStr_save(enc, strlen(enc));
    apexVM_pushstr(vm, enc_str);
    return 0;
}

/**
 * Decrypts a string using the AES decryption algorithm.
 *
 * This function decrypts a given string using the AES decryption algorithm
 * with the given key. It takes two arguments: a string to decrypt and a key
 * string. The length of the key string should be 16 bytes (128 bits).
 *
 * The function returns the decrypted string.
 */
int crypt_aes_inv(ApexVM *vm, int argc) {
    if (argc != 2) {
        apexErr_runtime(vm, "crypt:aes_inv expects exactly 2 arguments");
        return 1;
    }
    ApexValue key_val = apexVM_pop(vm);
    if (apexVal_type(key_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "second argument to crypt:aes_inv must be a string");
        return 1;
    }
    ApexValue enc_val = apexVM_pop(vm);
    if (apexVal_type(enc_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "first argument to crypt:aes_inv must be a string");
        return 1;
    }
    char *key = apexVal_str(key_val)->value;
    char *enc = apexVal_str(enc_val)->value;

    size_t len = strlen(enc) / 2;

    uint8_t *input = apexMem_alloc(len);
    for (size_t i = 0; i < len; i++) {
        sscanf(&enc[i * 2], "%2hhx", &input[i]);
    }

    uint8_t *output = calloc(len, sizeof(uint8_t));
    for (size_t i = 0; i < len; i += AES_BLOCK_SIZE) {
        aes_inv_block(input + i, output + i, (uint8_t *)key);
    }
    uint8_t padding = output[len - 1];

    char *dec = malloc(len - padding + 1);
    memcpy(dec, output, len - padding);
    dec[len - padding] = '\0';

    free(input);
    free(output);
    ApexString *dec_str = apexStr_save(dec, strlen(dec));
    apexVM_pushstr(vm, dec_str);
    return 0;
}

int crypt_hash(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "crypt:hash expects exactly 1 argument");
        return 1;
    }

    ApexValue password_val = apexVM_pop(vm);
    if (apexVal_type(password_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "argument to 'crypt:hash' must be a string");
        return 1;
    }

    static const char charset[] = "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    char salt[30];
    snprintf(salt, 30, "$2y$%02d$", 10);
    srand(time(NULL));
    for (int i = 0; i < 22; i++) {
        salt[7 + i] = charset[rand() % (sizeof(charset) - 1)];
    }
    salt[29] = '\0';

    const char *password_str = apexVal_str(password_val)->value;
    char *hash = crypt(password_str, salt);

    if (!hash) {
        apexErr_runtime(vm, "failed to hash the password");
        return 1;
    }

    ApexString *hash_str = apexStr_new(hash, strlen(hash));
    apexVM_pushstr(vm, hash_str);

    return 0;
}

apex_reglib(crypt,
    apex_regfn("hash", crypt_hash),
    apex_regfn("aes", crypt_aes),
    apex_regfn("aes_inv", crypt_aes_inv)
);