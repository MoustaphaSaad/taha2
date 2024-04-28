#pragma once

// choose the calling convention
#if TAHA_COMPILER_MSVC
	#if ((_MSC_FULL_VER >= 170065501) && (_MSC_VER < 1800)) || (_MSC_FULL_VER >= 180020418)
		// #define TAHA_XCALL __vectorcall
		#define TAHA_XCALL
	#else
		#define TAHA_XCALL __fastcall
	#endif
	#define TAHA_FORCE_INLINE __forceinline
#elif TAHA_COMPILER_GNU
	// GCC doesn't have a vectorcall convention
	#define TAHA_XCALL
	#define TAHA_FORCE_INLINE inline __attribute__((__always_inline__))
#elif TAHA_COMPILER_CLANG
	#define TAHA_XCALL __attribute__((vectorcall))
	#define TAHA_FORCE_INLINE inline __attribute__((__always_inline__))
#endif
