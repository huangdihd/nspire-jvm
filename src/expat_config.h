/* Build configuration for the vendored, unmodified Expat sources. */
#ifndef NSPIRE_EXPAT_CONFIG_H
#define NSPIRE_EXPAT_CONFIG_H
#define BYTEORDER 1234
#define XML_GE 1
#define XML_DTD 1
#define XML_NS 1
#define XML_CONTEXT_BYTES 1024
#ifdef _TINSPIRE
/* Ndless currently has no entropy device. This project only runs trusted
 * programs; this fallback is not a cryptographic random source. */
#define XML_POOR_ENTROPY 1
#else
#define XML_DEV_URANDOM 1
#endif
#endif
