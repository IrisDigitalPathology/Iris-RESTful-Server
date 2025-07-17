//
//  IrisRestfulSelfSigned.cpp
//  IrisRESTful
//
//  Created by Ryan Landvater on 6/23/25.
//
//#include <stdexcept>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif // __clang__
#include <boost/asio/ssl.hpp>       // Asio openssl interface
#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__

#include "IrisRestfulPriv.hpp"

// Modular Exponential (MODP) Groups for the Internet Key Exchange (IKE) protocol
// See https://www.ietf.org/rfc/rfc3526.txt for more information

constexpr char g_dh1024_sz[] =
"-----BEGIN DH PARAMETERS-----\n"
"MIGHAoGBAP//////////yQ/aoiFowjTExmKLgNwc0SkCTgiKZ8x0Agu+pjsTmyJR\n"
"Sgh5jjQE3e+VGbPNOkMbMCsKbfJfFDdP4TVtbVHCReSFtXZiXn7G9ExC6aY37WsL\n"
"/1y29Aa37e44a/taiZ+lrp8kEXxLH+ZJKGZR7OZTgf//////////AgEC\n"
"-----END DH PARAMETERS-----\n";

constexpr char g_dh1536_sz[] = "-----BEGIN DH PARAMETERS-----\n"
"MIHHAoHBAP//////////yQ/aoiFowjTExmKLgNwc0SkCTgiKZ8x0Agu+pjsTmyJR\n"
"Sgh5jjQE3e+VGbPNOkMbMCsKbfJfFDdP4TVtbVHCReSFtXZiXn7G9ExC6aY37WsL\n"
"/1y29Aa37e44a/taiZ+lrp8kEXxLH+ZJKGZR7ORbPcIAfLihY78FmNpINhxV05pp\n"
"Fj+o/STPX4NlXSPco62WHGLzViCFUrue1SkHcJaWbWcMNU5KvJgE8XRsCMojcyf/\n"
"/////////wIBAg==\n"
"-----END DH PARAMETERS-----\n";

// IETF RFC 3526 Group 14 Diffie-Hellman Parameters
// This prime is: 2^2048 - 2^1984 - 1 + 2^64 * { [2^1918 pi] + 124476 }
constexpr char g_dh2048_sz[] =
"-----BEGIN DH PARAMETERS-----\n"
"MIIBCAKCAQEA///////////JD9qiIWjCNMTGYouA3BzRKQJOCIpnzHQCC76mOxOb\n"
"IlFKCHmONATd75UZs806QxswKwpt8l8UN0/hNW1tUcJF5IW1dmJefsb0TELppjft\n"
"awv/XLb0Brft7jhr+1qJn6WunyQRfEsf5kkoZlHs5Fs9wgB8uKFjvwWY2kg2HFXT\n"
"mmkWP6j9JM9fg2VdI9yjrZYcYvNWIIVSu57VKQdwlpZtZww1Tkq8mATxdGwIyhgh\n"
"fDKQXkYuNs474553LBgOhgObJ4Oi7Aeij7XFXfBvTFLJ3ivL9pVYFxg5lUl86pVq\n"
"5RXSJhiY+gUQFXKOWoqsqmj//////////wIBAg==\n"
"-----END DH PARAMETERS-----\n";

constexpr char g_dh3072_sz[] =
"-----BEGIN DH PARAMETERS-----\n"
"MIIBiAKCAYEA///////////JD9qiIWjCNMTGYouA3BzRKQJOCIpnzHQCC76mOxOb\n"
"IlFKCHmONATd75UZs806QxswKwpt8l8UN0/hNW1tUcJF5IW1dmJefsb0TELppjft\n"
"awv/XLb0Brft7jhr+1qJn6WunyQRfEsf5kkoZlHs5Fs9wgB8uKFjvwWY2kg2HFXT\n"
"mmkWP6j9JM9fg2VdI9yjrZYcYvNWIIVSu57VKQdwlpZtZww1Tkq8mATxdGwIyhgh\n"
"fDKQXkYuNs474553LBgOhgObJ4Oi7Aeij7XFXfBvTFLJ3ivL9pVYFxg5lUl86pVq\n"
"5RXSJhiY+gUQFXKOWoqqxC2tMxcNBFB6M6hVIavfHLpk7PuFBFjb7wqK6nFXXQYM\n"
"fbOXD4Wm4eTHq/WujNsJM9cejJTgSiVhnc7j0iYa0u5r8S/6BtmKCGTYdgJzPshq\n"
"ZFIfKxgXeyAMu+EXV3phXWx3CYjAutlG4gjiT6B05asxQ9tb/OD9EI5LgtEgqTrS\n"
"yv//////////AgEC\n"
"-----END DH PARAMETERS-----\n";

constexpr char g_dh4096_sz[] =
"-----BEGIN DH PARAMETERS-----\n"
"MIICCAKCAgEA///////////JD9qiIWjCNMTGYouA3BzRKQJOCIpnzHQCC76mOxOb\n"
"IlFKCHmONATd75UZs806QxswKwpt8l8UN0/hNW1tUcJF5IW1dmJefsb0TELppjft\n"
"awv/XLb0Brft7jhr+1qJn6WunyQRfEsf5kkoZlHs5Fs9wgB8uKFjvwWY2kg2HFXT\n"
"mmkWP6j9JM9fg2VdI9yjrZYcYvNWIIVSu57VKQdwlpZtZww1Tkq8mATxdGwIyhgh\n"
"fDKQXkYuNs474553LBgOhgObJ4Oi7Aeij7XFXfBvTFLJ3ivL9pVYFxg5lUl86pVq\n"
"5RXSJhiY+gUQFXKOWoqqxC2tMxcNBFB6M6hVIavfHLpk7PuFBFjb7wqK6nFXXQYM\n"
"fbOXD4Wm4eTHq/WujNsJM9cejJTgSiVhnc7j0iYa0u5r8S/6BtmKCGTYdgJzPshq\n"
"ZFIfKxgXeyAMu+EXV3phXWx3CYjAutlG4gjiT6B05asxQ9tb/OD9EI5LgtEgqSEI\n"
"ARpyPBKnh+bXiHGaEL26WyaZwycYavTiPBqUaDS2FQvaJYPpyirUTOjbu8LbBN6O\n"
"+S6O/BQfvsqmKHxZR05rwF2ZspZPoJDDoiM7oYZRW+ftH2EpcM7i16+4G912IXBI\n"
"HNAGkSfVsFqpk7TqmI2P3cGG/7fckKbAj030Nck0BjGZ//////////8CAQI=\n"
"-----END DH PARAMETERS-----\n";

//#include <openssl/evp.h>
//#include <openssl/pem.h>
//#include <openssl/x509.h>
//#include <openssl/x509v3.h>

namespace Iris {
namespace RESTful {
inline Result LOAD_CERTIFICATE_AND_KEY (const std::filesystem::path& cert_path,
                                        const std::filesystem::path& key_path,
                                        Iris::Buffer & cert, Iris::Buffer & key,
                                        uint32_t& bits)
{
    Result result = IRIS_SUCCESS;
    FILE *f_cert        = NULL,
         *f_key         = NULL;
    X509 *x509          = NULL;
    EVP_PKEY *pkey      = NULL;
    BIO *cert_mem       = NULL,
        *key_mem        = NULL;
    BUF_MEM *cert_bptr  = NULL,
        *key_bptr       = NULL;
    
    try {
        f_cert = fopen(cert_path.string().c_str(), "r");
        if (!f_cert) throw std::runtime_error
            ("Failed to open designated cert file (" + cert_path.string() + "). Do you have file read permission?");
        
        f_key = fopen(key_path.string().c_str(), "r");
        if (!f_key) throw std::runtime_error
            ("Failed to open designated key file (" + key_path.string() + "). Do you have file read permission?");
        
        x509 = PEM_read_X509(f_cert, NULL, NULL, NULL);
        if (!x509) throw std::runtime_error
            ("PEM_read_X509 failed to read provided cert ("+ cert_path.string() +")");
        
        pkey = PEM_read_PrivateKey(f_key, NULL, NULL, NULL);
        if (!pkey) throw std::runtime_error
            ("PEM_read_PrivateKey failed to read the private key (" + key_path.string() + ")");
        
        bits = EVP_PKEY_get_bits(pkey);
        
        // Write cert to buffer
        cert_mem = BIO_new(BIO_s_mem());
        if (!PEM_write_bio_X509(cert_mem, x509)) throw std::runtime_error
            ("Failed to write the certificate to buffer stream");
        BIO_get_mem_ptr(cert_mem, &cert_bptr);
        cert = Iris::Copy_strong_buffer_from_data (cert_bptr->data, cert_bptr->length);
        
        // Write key to buffer
        key_mem = BIO_new(BIO_s_mem());
        if (!PEM_write_bio_PrivateKey
            (key_mem, pkey, NULL, NULL, 0, NULL, NULL)) throw std::runtime_error
            ("Failed to write private key to buffer stream");
        BIO_get_mem_ptr(key_mem, &key_bptr);
        key = Iris::Copy_strong_buffer_from_data (key_bptr->data, key_bptr->length);
        
    } catch (std::runtime_error & error) {
        std::string msg = error.what() ? error.what() :
        std::string("[undefined error in file") + __FILE__ + "]";
        result = Result(IRIS_FAILURE,
                        std::string("[ERROR] Failed to read the provided certificate/key files: ") +
                        msg + "\n");
    }

    if (cert_mem)   BIO_free(cert_mem);
    if (key_mem)    BIO_free(key_mem);
    if (pkey)       EVP_PKEY_free (pkey);
    if (x509)       X509_free (x509);
    if (f_key)      fclose (f_key);
    if (f_cert)     fclose (f_cert);

    return result;
}
constexpr unsigned char C  [] = "US";
constexpr unsigned char O  [] = "Iris Digital Pathology";
constexpr unsigned char CN [] = "localhost";
inline Result GENERATE_SELF_SIGNED_CERT (Iris::Buffer & cert, Iris::Buffer & key)
{
    Result result       = IRIS_SUCCESS;
    X509 *x509          = NULL;
    EVP_PKEY *pkey      = NULL;
    X509_NAME *name     = NULL;
    BIO *cert_mem       = NULL,
        *key_mem        = NULL;
    BUF_MEM *cert_bptr  = NULL,
        *key_bptr       = NULL;
    
    try {
        OpenSSL_add_all_algorithms();
        
        // Generate key
        pkey = EVP_RSA_gen(2048);
        if (!pkey) throw std::runtime_error
            ("EVPA_RSA_gen failed to generate a private key");
        
        // Generate cert
        x509 = X509_new();
        if (!x509) throw std::runtime_error
            ("X509_new failed to generate a certifiacte");
        
        // Set CERT perameters (key, expiration, and serial number)
        ASN1_INTEGER_set(X509_get_serialNumber(x509), 1); // Set SN 1 (req for some browsers)
        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        X509_gmtime_adj(X509_get_notAfter(x509), 31536000UL); // 1 year
        X509_set_pubkey(x509, pkey);
        
        // Set subject and issuer name (self-cert)
        name = X509_get_subject_name(x509);
        X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, C, -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, O, -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, CN, -1, -1, 0);
        X509_set_issuer_name(x509, name);
        
        // Sign cert
        if (!X509_sign(x509, pkey, EVP_sha256())) throw std::runtime_error
            ("Failed to sign self-signed cert");
        
        // Write cert to buffer
        cert_mem = BIO_new(BIO_s_mem());
        if (!PEM_write_bio_X509(cert_mem, x509)) throw std::runtime_error
            ("Failed to write the certificate to buffer stream");
        BIO_get_mem_ptr(cert_mem, &cert_bptr);
        cert = Iris::Copy_strong_buffer_from_data (cert_bptr->data, cert_bptr->length);
        
        // Write key to buffer
        key_mem = BIO_new(BIO_s_mem());
        if (!PEM_write_bio_PrivateKey
            (key_mem, pkey, NULL, NULL, 0, NULL, NULL)) throw std::runtime_error
            ("Failed to write private key to buffer stream");
        BIO_get_mem_ptr(key_mem, &key_bptr);
        key = Iris::Copy_strong_buffer_from_data (key_bptr->data, key_bptr->length);
        
    } catch (std::runtime_error& error) {
        std::string msg = error.what() ? error.what() :
        std::string("[undefined error in file") + __FILE__ + "]";
        result = Result(IRIS_FAILURE,
                        std::string("[ERROR] Failed to generate a self-signed certificate: ") +
                        msg + "\n");
    }
    
    if (cert_mem)   BIO_free(cert_mem);
    if (key_mem)    BIO_free(key_mem);
    if (pkey)       EVP_PKEY_free (pkey);
    if (x509)       X509_free (x509);
    
    return result;
}
std::shared_ptr<boost::asio::ssl::context> CREATE_SSL_CONTEXT (const std::filesystem::path& cert_path,
                                                               const std::filesystem::path& key_path) {
    
    Result result = IRIS_FAILURE;
    Iris::Buffer cert = NULL, key = NULL;
    uint32_t bits = 0;
    
    // Get the CERT and KEY in PEM format.
    if (!cert_path.empty() && !key_path.empty())
        result = LOAD_CERTIFICATE_AND_KEY (cert_path, key_path, cert, key, bits);
    else {
        std::cout   << "[WARNING] a certificate and corresponding private key were not provided. Iris RESTful "
                    << "will generate a self-signed certificatefor use in the secure socket layer. This should "
                    << "really only be used for debugging and you should use a trusted certificate for deployment.\n";
        result = GENERATE_SELF_SIGNED_CERT (cert, key);
        bits = 2048;
    } if (result & IRIS_FAILURE || !cert || !key) return NULL;
    
    // Generate the context
    auto ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv13_server);
    ctx->use_certificate_chain (boost::asio::buffer(cert->data(),cert->size()));
    ctx->use_private_key (boost::asio::buffer(key->data(), key->size()),
                          boost::asio::ssl::context::file_format::pem);
    switch (bits) {
        case 1024: ctx->use_tmp_dh (boost::asio::buffer(g_dh1024_sz)); break;
        case 1536: ctx->use_tmp_dh (boost::asio::buffer(g_dh1536_sz)); break;
        case 2048: ctx->use_tmp_dh (boost::asio::buffer(g_dh2048_sz)); break;
        case 3072: ctx->use_tmp_dh (boost::asio::buffer(g_dh3072_sz)); break;
        case 4096: ctx->use_tmp_dh (boost::asio::buffer(g_dh4096_sz)); break;
        default:
            std::cerr   << "[WARNING] This server will NOT use DH parameters and will be much less secure. "
            << "This is because the key length is unsupported (" << bits <<" bits). Iris RESTful currently only "
            << "supports 1024, 1536, 2048, 3072, and 4096 bit ciphers for Diffie-Hellman key agreement protocols.\n";
    }
    
    // Return the newly constructed SSL Context
    return ctx;
}
}
}
