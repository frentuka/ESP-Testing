# Install script for directory: R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/esp-now-test")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/Users/srleg/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin/xtensa-esp32s3-elf-objdump.exe")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/mbedtls" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ FILES
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/aes.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/aria.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/asn1.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/asn1write.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/base64.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/bignum.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/block_cipher.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/build_info.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/camellia.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ccm.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/chacha20.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/chachapoly.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/check_config.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/cipher.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/cmac.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/compat-2.x.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_adjust_legacy_crypto.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_adjust_legacy_from_psa.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_adjust_psa_from_legacy.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_adjust_psa_superset_legacy.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_adjust_ssl.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_adjust_x509.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/config_psa.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/constant_time.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ctr_drbg.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/debug.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/des.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/dhm.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ecdh.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ecdsa.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ecjpake.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ecp.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/entropy.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/error.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/gcm.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/hkdf.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/hmac_drbg.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/lms.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/mbedtls_config.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/md.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/md5.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/memory_buffer_alloc.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/net_sockets.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/nist_kw.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/oid.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/pem.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/pk.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/pkcs12.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/pkcs5.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/pkcs7.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/platform.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/platform_time.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/platform_util.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/poly1305.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/private_access.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/psa_util.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ripemd160.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/rsa.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/sha1.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/sha256.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/sha3.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/sha512.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ssl.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ssl_cache.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ssl_ciphersuites.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ssl_cookie.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/ssl_ticket.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/threading.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/timing.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/version.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/x509.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/x509_crl.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/x509_crt.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/mbedtls/x509_csr.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/psa" TYPE FILE PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ FILES
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/build_info.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_adjust_auto_enabled.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_adjust_config_dependencies.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_adjust_config_key_pair_types.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_adjust_config_synonyms.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_builtin_composites.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_builtin_key_derivation.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_builtin_primitives.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_compat.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_config.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_driver_common.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_driver_contexts_composites.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_driver_contexts_key_derivation.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_driver_contexts_primitives.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_extra.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_legacy.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_platform.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_se_driver.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_sizes.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_struct.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_types.h"
    "R:/appcrap/esp-idf-v5.5.1/components/mbedtls/mbedtls/include/psa/crypto_values.h"
    )
endif()

