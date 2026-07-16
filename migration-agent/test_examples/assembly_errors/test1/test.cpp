typedef unsigned long long __attribute__((aligned((8)))) u64a;
typedef signed char __int8_t;
typedef __int8_t int8_t;
typedef __attribute__((neon_vector_type(16))) int8_t int8x16_t;

typedef union {
    int8x16_t vect_s8;
} __m128i;

typedef __m128i m128;


static inline __attribute__ ((always_inline, unused)) m128 load_m128_from_u64a(const u64a *p) {
    m128 result;
    __asm__ __volatile__("ldr %d0, %1         \n\t"
                         : "=w"(result)
                         : "Utv"(*p)
                         :
    );
    return result;
}
