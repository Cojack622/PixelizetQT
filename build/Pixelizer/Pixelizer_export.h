
#ifndef PIX_EXPORT_H
#define PIX_EXPORT_H

#ifdef PIX_STATIC_DEFINE
#  define PIX_EXPORT
#  define PIX_NO_EXPORT
#else
#  ifndef PIX_EXPORT
#    ifdef Pixelizer_EXPORTS
        /* We are building this library */
#      define PIX_EXPORT 
#    else
        /* We are using this library */
#      define PIX_EXPORT 
#    endif
#  endif

#  ifndef PIX_NO_EXPORT
#    define PIX_NO_EXPORT 
#  endif
#endif

#ifndef PIX_DEPRECATED
#  define PIX_DEPRECATED __declspec(deprecated)
#endif

#ifndef PIX_DEPRECATED_EXPORT
#  define PIX_DEPRECATED_EXPORT PIX_EXPORT PIX_DEPRECATED
#endif

#ifndef PIX_DEPRECATED_NO_EXPORT
#  define PIX_DEPRECATED_NO_EXPORT PIX_NO_EXPORT PIX_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef PIX_NO_DEPRECATED
#    define PIX_NO_DEPRECATED
#  endif
#endif

#endif /* PIX_EXPORT_H */
