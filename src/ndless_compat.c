/* Ndless owns the native constructor/destructor sequence in crt0.S.
 * Ubuntu's generic bare-metal newlib additionally references the conventional
 * ELF _fini hook. This C application has no .fini section; there is no extra
 * work for that hook. A toolchain-provided strong definition takes precedence.
 */
#ifdef _TINSPIRE
void __attribute__((weak)) _fini(void) {}
#endif
