/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/*
 * This header file includes all of the fix point signal processing library (SPL) function
 * descriptions and declarations.
 * For specific function calls, see bottom of file.
 */

#ifndef WEBRTC_SPL_SIGNAL_PROCESSING_LIBRARY_H_
#define WEBRTC_SPL_SIGNAL_PROCESSING_LIBRARY_H_

#include <string.h>
#include <stdint.h>

// Macros specific for the fixed point implementation
#define WEBRTC_SPL_WORD16_MAX       32767
#define WEBRTC_SPL_WORD16_MIN       -32768
#define WEBRTC_SPL_WORD32_MAX       (int32_t)0x7fffffff
#define WEBRTC_SPL_WORD32_MIN       (int32_t)0x80000000
#define WEBRTC_SPL_MAX_LPC_ORDER    14
#define WEBRTC_SPL_MIN(A, B)        (A < B ? A : B)  // Get min value
#define WEBRTC_SPL_MAX(A, B)        (A > B ? A : B)  // Get max value
// TODO(kma/bjorn): For the next two macros, investigate how to correct the code
// for inputs of a = WEBRTC_SPL_WORD16_MIN or WEBRTC_SPL_WORD32_MIN.
#define WEBRTC_SPL_ABS_W16(a) \
    (((int16_t)a >= 0) ? ((int16_t)a) : -((int16_t)a))
#define WEBRTC_SPL_ABS_W32(a) \
    (((int32_t)a >= 0) ? ((int32_t)a) : -((int32_t)a))

#define WEBRTC_SPL_MUL(a, b) \
    ((int32_t) ((int32_t)(a) * (int32_t)(b)))
#define WEBRTC_SPL_UMUL(a, b) \
    ((uint32_t) ((uint32_t)(a) * (uint32_t)(b)))
#define WEBRTC_SPL_UMUL_32_16(a, b) \
    ((uint32_t) ((uint32_t)(a) * (uint16_t)(b)))
#define WEBRTC_SPL_MUL_16_U16(a, b) \
    ((int32_t)(int16_t)(a) * (uint16_t)(b))

#ifndef WEBRTC_ARCH_ARM_V7
// For ARMv7 platforms, these are inline functions in spl_inl_armv7.h
#ifndef MIPS32_LE
// For MIPS platforms, these are inline functions in spl_inl_mips.h
#define WEBRTC_SPL_MUL_16_16(a, b) \
    ((int32_t) (((int16_t)(a)) * ((int16_t)(b))))
#define WEBRTC_SPL_MUL_16_32_RSFT16(a, b) \
    (WEBRTC_SPL_MUL_16_16(a, b >> 16) \
     + ((WEBRTC_SPL_MUL_16_16(a, (b & 0xffff) >> 1) + 0x4000) >> 15))
#endif
#endif

#define WEBRTC_SPL_MUL_16_32_RSFT11(a, b) \
    ((WEBRTC_SPL_MUL_16_16(a, (b) >> 16) << 5) \
    + (((WEBRTC_SPL_MUL_16_U16(a, (uint16_t)(b)) >> 1) + 0x0200) >> 10))
#define WEBRTC_SPL_MUL_16_32_RSFT14(a, b) \
    ((WEBRTC_SPL_MUL_16_16(a, (b) >> 16) << 2) \
    + (((WEBRTC_SPL_MUL_16_U16(a, (uint16_t)(b)) >> 1) + 0x1000) >> 13))
#define WEBRTC_SPL_MUL_16_32_RSFT15(a, b) \
    ((WEBRTC_SPL_MUL_16_16(a, (b) >> 16) << 1) \
    + (((WEBRTC_SPL_MUL_16_U16(a, (uint16_t)(b)) >> 1) + 0x2000) >> 14))

#define WEBRTC_SPL_MUL_16_16_RSFT(a, b, c) \
    (WEBRTC_SPL_MUL_16_16(a, b) >> (c))

#define WEBRTC_SPL_MUL_16_16_RSFT_WITH_ROUND(a, b, c) \
    ((WEBRTC_SPL_MUL_16_16(a, b) + ((int32_t) \
                                  (((int32_t)1) << ((c) - 1)))) >> (c))

// C + the 32 most significant bits of A * B
#define WEBRTC_SPL_SCALEDIFF32(A, B, C) \
    (C + (B >> 16) * A + (((uint32_t)(0x0000FFFF & B) * A) >> 16))

#define WEBRTC_SPL_SAT(a, b, c)         (b > a ? a : b < c ? c : b)

// Shifting with negative numbers allowed
// Positive means left shift
#define WEBRTC_SPL_SHIFT_W32(x, c) \
    (((c) >= 0) ? ((x) << (c)) : ((x) >> (-(c))))

// Shifting with negative numbers not allowed
// We cannot do casting here due to signed/unsigned problem
#define WEBRTC_SPL_LSHIFT_W32(x, c)     ((x) << (c))

#define WEBRTC_SPL_RSHIFT_U32(x, c)     ((uint32_t)(x) >> (c))

#define WEBRTC_SPL_RAND(a) \
    ((int16_t)(WEBRTC_SPL_MUL_16_16_RSFT((a), 18816, 7) & 0x00007fff))


#ifdef __cplusplus
extern "C" {
#endif

#define WEBRTC_SPL_MEMCPY_W16(v1, v2, length) \
  memcpy(v1, v2, (length) * sizeof(int16_t))

// inline functions:
#include "spl_inl.h"

// Initialize SPL. Currently it contains only function pointer initialization.
// If the underlying platform is known to be ARM-Neon (WEBRTC_ARCH_ARM_NEON
// defined), the pointers will be assigned to code optimized for Neon; otherwise
// if run-time Neon detection (WEBRTC_DETECT_ARM_NEON) is enabled, the pointers
// will be assigned to either Neon code or generic C code; otherwise, generic C
// code will be assigned.
// Note that this function MUST be called in any application that uses SPL
// functions.
void WebRtcSpl_Init();

int16_t WebRtcSpl_GetScalingSquare(int16_t* in_vector,
                                   int in_vector_length,
                                   int times);

// Copy and set operations. Implementation in copy_set_operations.c.
// Descriptions at bottom of file.
void WebRtcSpl_MemSetW16(int16_t* vector,
                         int16_t set_value,
                         int vector_length);
void WebRtcSpl_MemSetW32(int32_t* vector,
                         int32_t set_value,
                         int vector_length);
void WebRtcSpl_MemCpyReversedOrder(int16_t* out_vector,
                                   int16_t* in_vector,
                                   int vector_length);
void WebRtcSpl_CopyFromEndW16(const int16_t* in_vector,
                              int in_vector_length,
                              int samples,
                              int16_t* out_vector);
void WebRtcSpl_ZerosArrayW16(int16_t* vector,
                             int vector_length);
void WebRtcSpl_ZerosArrayW32(int32_t* vector,
                             int vector_length);
// End: Copy and set operations.


// Minimum and maximum operation functions and their pointers.
// Implementation in min_max_operations.c.

// Returns the largest absolute value in a signed 16-bit vector.
//
// Input:
//      - vector : 16-bit input vector.
//      - length : Number of samples in vector.
//
// Return value  : Maximum absolute value in vector;
//                 or -1, if (vector == NULL || length <= 0).
typedef int16_t (*MaxAbsValueW16)(const int16_t* vector, int length);
extern MaxAbsValueW16 WebRtcSpl_MaxAbsValueW16;
int16_t WebRtcSpl_MaxAbsValueW16C(const int16_t* vector, int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int16_t WebRtcSpl_MaxAbsValueW16Neon(const int16_t* vector, int length);
#endif
#if defined(MIPS32_LE)
int16_t WebRtcSpl_MaxAbsValueW16_mips(const int16_t* vector, int length);
#endif

// Returns the largest absolute value in a signed 32-bit vector.
//
// Input:
//      - vector : 32-bit input vector.
//      - length : Number of samples in vector.
//
// Return value  : Maximum absolute value in vector;
//                 or -1, if (vector == NULL || length <= 0).
typedef int32_t (*MaxAbsValueW32)(const int32_t* vector, int length);
extern MaxAbsValueW32 WebRtcSpl_MaxAbsValueW32;
int32_t WebRtcSpl_MaxAbsValueW32C(const int32_t* vector, int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int32_t WebRtcSpl_MaxAbsValueW32Neon(const int32_t* vector, int length);
#endif
#if defined(MIPS_DSP_R1_LE)
int32_t WebRtcSpl_MaxAbsValueW32_mips(const int32_t* vector, int length);
#endif

// Returns the maximum value of a 16-bit vector.
//
// Input:
//      - vector : 16-bit input vector.
//      - length : Number of samples in vector.
//
// Return value  : Maximum sample value in |vector|.
//                 If (vector == NULL || length <= 0) WEBRTC_SPL_WORD16_MIN
//                 is returned. Note that WEBRTC_SPL_WORD16_MIN is a feasible
//                 value and we can't catch errors purely based on it.
typedef int16_t (*MaxValueW16)(const int16_t* vector, int length);
extern MaxValueW16 WebRtcSpl_MaxValueW16;
int16_t WebRtcSpl_MaxValueW16C(const int16_t* vector, int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int16_t WebRtcSpl_MaxValueW16Neon(const int16_t* vector, int length);
#endif
#if defined(MIPS32_LE)
int16_t WebRtcSpl_MaxValueW16_mips(const int16_t* vector, int length);
#endif

// Returns the maximum value of a 32-bit vector.
//
// Input:
//      - vector : 32-bit input vector.
//      - length : Number of samples in vector.
//
// Return value  : Maximum sample value in |vector|.
//                 If (vector == NULL || length <= 0) WEBRTC_SPL_WORD32_MIN
//                 is returned. Note that WEBRTC_SPL_WORD32_MIN is a feasible
//                 value and we can't catch errors purely based on it.
typedef int32_t (*MaxValueW32)(const int32_t* vector, int length);
extern MaxValueW32 WebRtcSpl_MaxValueW32;
int32_t WebRtcSpl_MaxValueW32C(const int32_t* vector, int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int32_t WebRtcSpl_MaxValueW32Neon(const int32_t* vector, int length);
#endif
#if defined(MIPS32_LE)
int32_t WebRtcSpl_MaxValueW32_mips(const int32_t* vector, int length);
#endif

// Returns the minimum value of a 16-bit vector.
//
// Input:
//      - vector : 16-bit input vector.
//      - length : Number of samples in vector.
//
// Return value  : Minimum sample value in |vector|.
//                 If (vector == NULL || length <= 0) WEBRTC_SPL_WORD16_MAX
//                 is returned. Note that WEBRTC_SPL_WORD16_MAX is a feasible
//                 value and we can't catch errors purely based on it.
typedef int16_t (*MinValueW16)(const int16_t* vector, int length);
extern MinValueW16 WebRtcSpl_MinValueW16;
int16_t WebRtcSpl_MinValueW16C(const int16_t* vector, int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int16_t WebRtcSpl_MinValueW16Neon(const int16_t* vector, int length);
#endif
#if defined(MIPS32_LE)
int16_t WebRtcSpl_MinValueW16_mips(const int16_t* vector, int length);
#endif

// Returns the minimum value of a 32-bit vector.
//
// Input:
//      - vector : 32-bit input vector.
//      - length : Number of samples in vector.
//
// Return value  : Minimum sample value in |vector|.
//                 If (vector == NULL || length <= 0) WEBRTC_SPL_WORD32_MAX
//                 is returned. Note that WEBRTC_SPL_WORD32_MAX is a feasible
//                 value and we can't catch errors purely based on it.
typedef int32_t (*MinValueW32)(const int32_t* vector, int length);
extern MinValueW32 WebRtcSpl_MinValueW32;
int32_t WebRtcSpl_MinValueW32C(const int32_t* vector, int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int32_t WebRtcSpl_MinValueW32Neon(const int32_t* vector, int length);
#endif
#if defined(MIPS32_LE)
int32_t WebRtcSpl_MinValueW32_mips(const int32_t* vector, int length);
#endif

// Returns the vector index to the largest absolute value of a 16-bit vector.
//
// Input:
//      - vector : 16-bit input vector.
//      - length : Number of samples in vector.
//
// Return value  : Index to the maximum absolute value in vector, or -1,
//                 if (vector == NULL || length <= 0).
//                 If there are multiple equal maxima, return the index of the
//                 first. -32768 will always have precedence over 32767 (despite
//                 -32768 presenting an int16 absolute value of 32767);
int WebRtcSpl_MaxAbsIndexW16(const int16_t* vector, int length);

// Returns the vector index to the maximum sample value of a 16-bit vector.
//
// Input:
//      - vector : 16-bit input vector.
//      - length : Number of samples in vector.
//
// Return value  : Index to the maximum value in vector (if multiple
//                 indexes have the maximum, return the first);
//                 or -1, if (vector == NULL || length <= 0).
int WebRtcSpl_MaxIndexW16(const int16_t* vector, int length);

// Returns the vector index to the maximum sample value of a 32-bit vector.
//
// Input:
//      - vector : 32-bit input vector.
//      - length : Number of samples in vector.
//
// Return value  : Index to the maximum value in vector (if multiple
//                 indexes have the maximum, return the first);
//                 or -1, if (vector == NULL || length <= 0).
int WebRtcSpl_MaxIndexW32(const int32_t* vector, int length);

// Returns the vector index to the minimum sample value of a 16-bit vector.
//
// Input:
//      - vector : 16-bit input vector.
//      - length : Number of samples in vector.
//
// Return value  : Index to the mimimum value in vector  (if multiple
//                 indexes have the minimum, return the first);
//                 or -1, if (vector == NULL || length <= 0).
int WebRtcSpl_MinIndexW16(const int16_t* vector, int length);

// Returns the vector index to the minimum sample value of a 32-bit vector.
//
// Input:
//      - vector : 32-bit input vector.
//      - length : Number of samples in vector.
//
// Return value  : Index to the mimimum value in vector  (if multiple
//                 indexes have the minimum, return the first);
//                 or -1, if (vector == NULL || length <= 0).
int WebRtcSpl_MinIndexW32(const int32_t* vector, int length);

// End: Minimum and maximum operations.


// Vector scaling operations. Implementation in vector_scaling_operations.c.
// Description at bottom of file.
void WebRtcSpl_VectorBitShiftW16(int16_t* out_vector,
                                 int16_t vector_length,
                                 const int16_t* in_vector,
                                 int16_t right_shifts);
void WebRtcSpl_VectorBitShiftW32(int32_t* out_vector,
                                 int16_t vector_length,
                                 const int32_t* in_vector,
                                 int16_t right_shifts);
void WebRtcSpl_VectorBitShiftW32ToW16(int16_t* out_vector,
                                      int vector_length,
                                      const int32_t* in_vector,
                                      int right_shifts);
void WebRtcSpl_ScaleVector(const int16_t* in_vector,
                           int16_t* out_vector,
                           int16_t gain,
                           int16_t vector_length,
                           int16_t right_shifts);
void WebRtcSpl_ScaleVectorWithSat(const int16_t* in_vector,
                                  int16_t* out_vector,
                                  int16_t gain,
                                  int16_t vector_length,
                                  int16_t right_shifts);
void WebRtcSpl_ScaleAndAddVectors(const int16_t* in_vector1,
                                  int16_t gain1, int right_shifts1,
                                  const int16_t* in_vector2,
                                  int16_t gain2, int right_shifts2,
                                  int16_t* out_vector,
                                  int vector_length);

// The functions (with related pointer) perform the vector operation:
//   out_vector[k] = ((scale1 * in_vector1[k]) + (scale2 * in_vector2[k])
//        + round_value) >> right_shifts,
//   where  round_value = (1 << right_shifts) >> 1.
//
// Input:
//      - in_vector1       : Input vector 1
//      - in_vector1_scale : Gain to be used for vector 1
//      - in_vector2       : Input vector 2
//      - in_vector2_scale : Gain to be used for vector 2
//      - right_shifts     : Number of right bit shifts to be applied
//      - length           : Number of elements in the input vectors
//
// Output:
//      - out_vector       : Output vector
// Return value            : 0 if OK, -1 if (in_vector1 == NULL
//                           || in_vector2 == NULL || out_vector == NULL
//                           || length <= 0 || right_shift < 0).
typedef int (*ScaleAndAddVectorsWithRound)(const int16_t* in_vector1,
                                           int16_t in_vector1_scale,
                                           const int16_t* in_vector2,
                                           int16_t in_vector2_scale,
                                           int right_shifts,
                                           int16_t* out_vector,
                                           int length);
extern ScaleAndAddVectorsWithRound WebRtcSpl_ScaleAndAddVectorsWithRound;
int WebRtcSpl_ScaleAndAddVectorsWithRoundC(const int16_t* in_vector1,
                                           int16_t in_vector1_scale,
                                           const int16_t* in_vector2,
                                           int16_t in_vector2_scale,
                                           int right_shifts,
                                           int16_t* out_vector,
                                           int length);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int WebRtcSpl_ScaleAndAddVectorsWithRoundNeon(const int16_t* in_vector1,
                                              int16_t in_vector1_scale,
                                              const int16_t* in_vector2,
                                              int16_t in_vector2_scale,
                                              int right_shifts,
                                              int16_t* out_vector,
                                              int length);
#endif
#if defined(MIPS_DSP_R1_LE)
int WebRtcSpl_ScaleAndAddVectorsWithRound_mips(const int16_t* in_vector1,
                                               int16_t in_vector1_scale,
                                               const int16_t* in_vector2,
                                               int16_t in_vector2_scale,
                                               int right_shifts,
                                               int16_t* out_vector,
                                               int length);
#endif
// End: Vector scaling operations.

// iLBC specific functions. Implementations in ilbc_specific_functions.c.
// Description at bottom of file.
void WebRtcSpl_ReverseOrderMultArrayElements(int16_t* out_vector,
                                             const int16_t* in_vector,
                                             const int16_t* window,
                                             int16_t vector_length,
                                             int16_t right_shifts);
void WebRtcSpl_ElementwiseVectorMult(int16_t* out_vector,
                                     const int16_t* in_vector,
                                     const int16_t* window,
                                     int16_t vector_length,
                                     int16_t right_shifts);
void WebRtcSpl_AddVectorsAndShift(int16_t* out_vector,
                                  const int16_t* in_vector1,
                                  const int16_t* in_vector2,
                                  int16_t vector_length,
                                  int16_t right_shifts);
void WebRtcSpl_AddAffineVectorToVector(int16_t* out_vector,
                                       int16_t* in_vector,
                                       int16_t gain,
                                       int32_t add_constant,
                                       int16_t right_shifts,
                                       int vector_length);
void WebRtcSpl_AffineTransformVector(int16_t* out_vector,
                                     int16_t* in_vector,
                                     int16_t gain,
                                     int32_t add_constant,
                                     int16_t right_shifts,
                                     int vector_length);
// End: iLBC specific functions.

// Signal processing operations.

// A 32-bit fix-point implementation of auto-correlation computation
//
// Input:
//      - in_vector        : Vector to calculate autocorrelation upon
//      - in_vector_length : Length (in samples) of |vector|
//      - order            : The order up to which the autocorrelation should be
//                           calculated
//
// Output:
//      - result           : auto-correlation values (values should be seen
//                           relative to each other since the absolute values
//                           might have been down shifted to avoid overflow)
//
//      - scale            : The number of left shifts required to obtain the
//                           auto-correlation in Q0
//
// Return value            :
//      - -1, if |order| > |in_vector_length|;
//      - Number of samples in |result|, i.e. (order+1), otherwise.
int WebRtcSpl_AutoCorrelation(const int16_t* in_vector,
                              int in_vector_length,
                              int order,
                              int32_t* result,
                              int* scale);

// A 32-bit fix-point implementation of the Levinson-Durbin algorithm that
// does NOT use the 64 bit class
//
// Input:
//      - auto_corr : Vector with autocorrelation values of length >=
//                    |use_order|+1
//      - use_order : The LPC filter order (support up to order 20)
//
// Output:
//      - lpc_coef  : lpc_coef[0..use_order] LPC coefficients in Q12
//      - refl_coef : refl_coef[0...use_order-1]| Reflection coefficients in
//                    Q15
//
// Return value     : 1 for stable 0 for unstable
int16_t WebRtcSpl_LevinsonDurbin(int32_t* auto_corr,
                                 int16_t* lpc_coef,
                                 int16_t* refl_coef,
                                 int16_t order);

// Converts reflection coefficients |refl_coef| to LPC coefficients |lpc_coef|.
// This version is a 16 bit operation.
//
// NOTE: The 16 bit refl_coef -> lpc_coef conversion might result in a
// "slightly unstable" filter (i.e., a pole just outside the unit circle) in
// "rare" cases even if the reflection coefficients are stable.
//
// Input:
//      - refl_coef : Reflection coefficients in Q15 that should be converted
//                    to LPC coefficients
//      - use_order : Number of coefficients in |refl_coef|
//
// Output:
//      - lpc_coef  : LPC coefficients in Q12
void WebRtcSpl_ReflCoefToLpc(const int16_t* refl_coef,
                             int use_order,
                             int16_t* lpc_coef);

// Converts LPC coefficients |lpc_coef| to reflection coefficients |refl_coef|.
// This version is a 16 bit operation.
// The conversion is implemented by the step-down algorithm.
//
// Input:
//      - lpc_coef  : LPC coefficients in Q12, that should be converted to
//                    reflection coefficients
//      - use_order : Number of coefficients in |lpc_coef|
//
// Output:
//      - refl_coef : Reflection coefficients in Q15.
void WebRtcSpl_LpcToReflCoef(int16_t* lpc_coef,
                             int use_order,
                             int16_t* refl_coef);

// Calculates reflection coefficients (16 bit) from auto-correlation values
//
// Input:
//      - auto_corr : Auto-correlation values
//      - use_order : Number of coefficients wanted be calculated
//
// Output:
//      - refl_coef : Reflection coefficients in Q15.
void WebRtcSpl_AutoCorrToReflCoef(const int32_t* auto_corr,
                                  int use_order,
                                  int16_t* refl_coef);

// The functions (with related pointer) calculate the cross-correlation between
// two sequences |seq1| and |seq2|.
// |seq1| is fixed and |seq2| slides as the pointer is increased with the
// amount |step_seq2|. Note the arguments should obey the relationship:
// |dim_seq| - 1 + |step_seq2| * (|dim_cross_correlation| - 1) <
//      buffer size of |seq2|
//
// Input:
//      - seq1           : First sequence (fixed throughout the correlation)
//      - seq2           : Second sequence (slides |step_vector2| for each
//                            new correlation)
//      - dim_seq        : Number of samples to use in the cross-correlation
//      - dim_cross_correlation : Number of cross-correlations to calculate (the
//                            start position for |vector2| is updated for each
//                            new one)
//      - right_shifts   : Number of right bit shifts to use. This will
//                            become the output Q-domain.
//      - step_seq2      : How many (positive or negative) steps the
//                            |vector2| pointer should be updated for each new
//                            cross-correlation value.
//
// Output:
//      - cross_correlation : The cross-correlation in Q(-right_shifts)
typedef void (*CrossCorrelation)(int32_t* cross_correlation,
                                 const int16_t* seq1,
                                 const int16_t* seq2,
                                 int16_t dim_seq,
                                 int16_t dim_cross_correlation,
                                 int16_t right_shifts,
                                 int16_t step_seq2);
extern CrossCorrelation WebRtcSpl_CrossCorrelation;
void WebRtcSpl_CrossCorrelationC(int32_t* cross_correlation,
                                 const int16_t* seq1,
                                 const int16_t* seq2,
                                 int16_t dim_seq,
                                 int16_t dim_cross_correlation,
                                 int16_t right_shifts,
                                 int16_t step_seq2);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
void WebRtcSpl_CrossCorrelationNeon(int32_t* cross_correlation,
                                    const int16_t* seq1,
                                    const int16_t* seq2,
                                    int16_t dim_seq,
                                    int16_t dim_cross_correlation,
                                    int16_t right_shifts,
                                    int16_t step_seq2);
#endif
#if defined(MIPS32_LE)
void WebRtcSpl_CrossCorrelation_mips(int32_t* cross_correlation,
                                     const int16_t* seq1,
                                     const int16_t* seq2,
                                     int16_t dim_seq,
                                     int16_t dim_cross_correlation,
                                     int16_t right_shifts,
                                     int16_t step_seq2);
#endif



// Creates (the first half of) a Hanning window. Size must be at least 1 and
// at most 512.
//
// Input:
//      - size      : Length of the requested Hanning window (1 to 512)
//
// Output:
//      - window    : Hanning vector in Q14.
void WebRtcSpl_GetHanningWindow(int16_t* window, int16_t size);

// Calculates y[k] = sqrt(1 - x[k]^2) for each element of the input vector
// |in_vector|. Input and output values are in Q15.
//
// Inputs:
//      - in_vector     : Values to calculate sqrt(1 - x^2) of
//      - vector_length : Length of vector |in_vector|
//
// Output:
//      - out_vector    : Output values in Q15
void WebRtcSpl_SqrtOfOneMinusXSquared(int16_t* in_vector,
                                      int vector_length,
                                      int16_t* out_vector);
// End: Signal processing operations.

// Randomization functions. Implementations collected in
// randomization_functions.c and descriptions at bottom of this file.
int16_t WebRtcSpl_RandU(uint32_t* seed);
int16_t WebRtcSpl_RandN(uint32_t* seed);
int16_t WebRtcSpl_RandUArray(int16_t* vector,
                             int16_t vector_length,
                             uint32_t* seed);
// End: Randomization functions.

// Math functions
int32_t WebRtcSpl_Sqrt(int32_t value);
int32_t WebRtcSpl_SqrtFloor(int32_t value);

// Divisions. Implementations collected in division_operations.c and
// descriptions at bottom of this file.
uint32_t WebRtcSpl_DivU32U16(uint32_t num, uint16_t den);
int32_t WebRtcSpl_DivW32W16(int32_t num, int16_t den);
int16_t WebRtcSpl_DivW32W16ResW16(int32_t num, int16_t den);
int32_t WebRtcSpl_DivResultInQ31(int32_t num, int32_t den);
int32_t WebRtcSpl_DivW32HiLow(int32_t num, int16_t den_hi, int16_t den_low);
// End: Divisions.

int32_t WebRtcSpl_Energy(int16_t* vector, int vector_length, int* scale_factor);

// Calculates the dot product between two (int16_t) vectors.
//
// Input:
//      - vector1       : Vector 1
//      - vector2       : Vector 2
//      - vector_length : Number of samples used in the dot product
//      - scaling       : The number of right bit shifts to apply on each term
//                        during calculation to avoid overflow, i.e., the
//                        output will be in Q(-|scaling|)
//
// Return value         : The dot product in Q(-scaling)
int32_t WebRtcSpl_DotProductWithScale(const int16_t* vector1,
                                      const int16_t* vector2,
                                      int length,
                                      int scaling);

// Filter operations.
int WebRtcSpl_FilterAR(const int16_t* ar_coef,
                       int ar_coef_length,
                       const int16_t* in_vector,
                       int in_vector_length,
                       int16_t* filter_state,
                       int filter_state_length,
                       int16_t* filter_state_low,
                       int filter_state_low_length,
                       int16_t* out_vector,
                       int16_t* out_vector_low,
                       int out_vector_low_length);

void WebRtcSpl_FilterMAFastQ12(int16_t* in_vector,
                               int16_t* out_vector,
                               int16_t* ma_coef,
                               int16_t ma_coef_length,
                               int16_t vector_length);

// Performs a AR filtering on a vector in Q12
// Input:
//      - data_in            : Input samples
//      - data_out           : State information in positions
//                               data_out[-order] .. data_out[-1]
//      - coefficients       : Filter coefficients (in Q12)
//      - coefficients_length: Number of coefficients (order+1)
//      - data_length        : Number of samples to be filtered
// Output:
//      - data_out           : Filtered samples
void WebRtcSpl_FilterARFastQ12(const int16_t* data_in,
                               int16_t* data_out,
                               const int16_t* __restrict coefficients,
                               int coefficients_length,
                               int data_length);

// The functions (with related pointer) perform a MA down sampling filter
// on a vector.
// Input:
//      - data_in            : Input samples (state in positions
//                               data_in[-order] .. data_in[-1])
//      - data_in_length     : Number of samples in |data_in| to be filtered.
//                               This must be at least
//                               |delay| + |factor|*(|out_vector_length|-1) + 1)
//      - data_out_length    : Number of down sampled samples desired
//      - coefficients       : Filter coefficients (in Q12)
//      - coefficients_length: Number of coefficients (order+1)
//      - factor             : Decimation factor
//      - delay              : Delay of filter (compensated for in out_vector)
// Output:
//      - data_out           : Filtered samples
// Return value              : 0 if OK, -1 if |in_vector| is too short
typedef int (*DownsampleFast)(const int16_t* data_in,
                              int data_in_length,
                              int16_t* data_out,
                              int data_out_length,
                              const int16_t* __restrict coefficients,
                              int coefficients_length,
                              int factor,
                              int delay);
extern DownsampleFast WebRtcSpl_DownsampleFast;
int WebRtcSpl_DownsampleFastC(const int16_t* data_in,
                              int data_in_length,
                              int16_t* data_out,
                              int data_out_length,
                              const int16_t* __restrict coefficients,
                              int coefficients_length,
                              int factor,
                              int delay);
#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
int WebRtcSpl_DownsampleFastNeon(const int16_t* data_in,
                                 int data_in_length,
                                 int16_t* data_out,
                                 int data_out_length,
                                 const int16_t* __restrict coefficients,
                                 int coefficients_length,
                                 int factor,
                                 int delay);
#endif
#if defined(MIPS32_LE)
int WebRtcSpl_DownsampleFast_mips(const int16_t* data_in,
                                  int data_in_length,
                                  int16_t* data_out,
                                  int data_out_length,
                                  const int16_t* __restrict coefficients,
                                  int coefficients_length,
                                  int factor,
                                  int delay);
#endif

// End: Filter operations.

// FFT operations

int WebRtcSpl_ComplexFFT(int16_t vector[], int stages, int mode);
int WebRtcSpl_ComplexIFFT(int16_t vector[], int stages, int mode);

// Treat a 16-bit complex data buffer |complex_data| as an array of 32-bit
// values, and swap elements whose indexes are bit-reverses of each other.
//
// Input:
//      - complex_data  : Complex data buffer containing 2^|stages| real
//                        elements interleaved with 2^|stages| imaginary
//                        elements: [Re Im Re Im Re Im....]
//      - stages        : Number of FFT stages. Must be at least 3 and at most
//                        10, since the table WebRtcSpl_kSinTable1024[] is 1024
//                        elements long.
//
// Output:
//      - complex_data  : The complex data buffer.

void WebRtcSpl_ComplexBitReverse(int16_t* __restrict complex_data, int stages);

// End: FFT operations

/************************************************************
 *
 * RESAMPLING FUNCTIONS AND THEIR STRUCTS ARE DEFINED BELOW
 *
 ************************************************************/

/*******************************************************************
 * resample.c
 *
 * Includes the following resampling combinations
 * 22 kHz -> 16 kHz
 * 16 kHz -> 22 kHz
 * 22 kHz ->  8 kHz
 *  8 kHz -> 22 kHz
 *
 ******************************************************************/

// state structure for 22 -> 16 resampler
typedef struct {
  int32_t S_22_44[8];
  int32_t S_44_32[8];
  int32_t S_32_16[8];
} WebRtcSpl_State22khzTo16khz;

void WebRtcSpl_Resample22khzTo16khz(const int16_t* in,
                                    int16_t* out,
                                    WebRtcSpl_State22khzTo16khz* state,
                                    int32_t* tmpmem);

void WebRtcSpl_ResetResample22khzTo16khz(WebRtcSpl_State22khzTo16khz* state);

// state structure for 16 -> 22 resampler
typedef struct {
  int32_t S_16_32[8];
  int32_t S_32_22[8];
} WebRtcSpl_State16khzTo22khz;

void WebRtcSpl_Resample16khzTo22khz(const int16_t* in,
                                    int16_t* out,
                                    WebRtcSpl_State16khzTo22khz* state,
                                    int32_t* tmpmem);

void WebRtcSpl_ResetResample16khzTo22khz(WebRtcSpl_State16khzTo22khz* state);

// state structure for 22 -> 8 resampler
typedef struct {
  int32_t S_22_22[16];
  int32_t S_22_16[8];
  int32_t S_16_8[8];
} WebRtcSpl_State22khzTo8khz;

void WebRtcSpl_Resample22khzTo8khz(const int16_t* in, int16_t* out,
                                   WebRtcSpl_State22khzTo8khz* state,
                                   int32_t* tmpmem);

void WebRtcSpl_ResetResample22khzTo8khz(WebRtcSpl_State22khzTo8khz* state);

// state structure for 8 -> 22 resampler
typedef struct {
  int32_t S_8_16[8];
  int32_t S_16_11[8];
  int32_t S_11_22[8];
} WebRtcSpl_State8khzTo22khz;

void WebRtcSpl_Resample8khzTo22khz(const int16_t* in, int16_t* out,
                                   WebRtcSpl_State8khzTo22khz* state,
                                   int32_t* tmpmem);

void WebRtcSpl_ResetResample8khzTo22khz(WebRtcSpl_State8khzTo22khz* state);

/*******************************************************************
 * resample_fractional.c
 * Functions for internal use in the other resample functions
 *
 * Includes the following resampling combinations
 * 48 kHz -> 32 kHz
 * 32 kHz -> 24 kHz
 * 44 kHz -> 32 kHz
 *
 ******************************************************************/

void WebRtcSpl_Resample48khzTo32khz(const int32_t* In, int32_t* Out,
                                    int32_t K);

void WebRtcSpl_Resample32khzTo24khz(const int32_t* In, int32_t* Out,
                                    int32_t K);

void WebRtcSpl_Resample44khzTo32khz(const int32_t* In, int32_t* Out,
                                    int32_t K);

/*******************************************************************
 * resample_48khz.c
 *
 * Includes the following resampling combinations
 * 48 kHz -> 16 kHz
 * 16 kHz -> 48 kHz
 * 48 kHz ->  8 kHz
 *  8 kHz -> 48 kHz
 *
 ******************************************************************/

typedef struct {
  int32_t S_48_48[16];
  int32_t S_48_32[8];
  int32_t S_32_16[8];
} WebRtcSpl_State48khzTo16khz;

void WebRtcSpl_Resample48khzTo16khz(const int16_t* in, int16_t* out,
                                    WebRtcSpl_State48khzTo16khz* state,
                                    int32_t* tmpmem);

void WebRtcSpl_ResetResample48khzTo16khz(WebRtcSpl_State48khzTo16khz* state);

typedef struct {
  int32_t S_16_32[8];
  int32_t S_32_24[8];
  int32_t S_24_48[8];
} WebRtcSpl_State16khzTo48khz;

void WebRtcSpl_Resample16khzTo48khz(const int16_t* in, int16_t* out,
                                    WebRtcSpl_State16khzTo48khz* state,
                                    int32_t* tmpmem);

void WebRtcSpl_ResetResample16khzTo48khz(WebRtcSpl_State16khzTo48khz* state);

typedef struct {
  int32_t S_48_24[8];
  int32_t S_24_24[16];
  int32_t S_24_16[8];
  int32_t S_16_8[8];
} WebRtcSpl_State48khzTo8khz;

void WebRtcSpl_Resample48khzTo8khz(const int16_t* in, int16_t* out,
                                   WebRtcSpl_State48khzTo8khz* state,
                                   int32_t* tmpmem);

void WebRtcSpl_ResetResample48khzTo8khz(WebRtcSpl_State48khzTo8khz* state);

typedef struct {
  int32_t S_8_16[8];
  int32_t S_16_12[8];
  int32_t S_12_24[8];
  int32_t S_24_48[8];
} WebRtcSpl_State8khzTo48khz;

void WebRtcSpl_Resample8khzTo48khz(const int16_t* in, int16_t* out,
                                   WebRtcSpl_State8khzTo48khz* state,
                                   int32_t* tmpmem);

void WebRtcSpl_ResetResample8khzTo48khz(WebRtcSpl_State8khzTo48khz* state);

/*******************************************************************
 * resample_by_2.c
 *
 * Includes down and up sampling by a factor of two.
 *
 ******************************************************************/

void WebRtcSpl_DownsampleBy2(const int16_t* in, int len,
                             int16_t* out, int32_t* filtState);

void WebRtcSpl_UpsampleBy2(const int16_t* in, int len,
                           int16_t* out, int32_t* filtState);

/************************************************************
 * END OF RESAMPLING FUNCTIONS
 ************************************************************/
void WebRtcSpl_AnalysisQMF(const int16_t* in_data,
                           int in_data_length,
                           int16_t* low_band,
                           int16_t* high_band,
                           int32_t* filter_state1,
                           int32_t* filter_state2);
void WebRtcSpl_SynthesisQMF(const int16_t* low_band,
                            const int16_t* high_band,
                            int band_length,
                            int16_t* out_data,
                            int32_t* filter_state1,
                            int32_t* filter_state2);


/* Float functions.
 */
void FloatS16ToS16(const float* src, size_t size, int16_t* dest);
void S16ToFloatS16(const int16_t* src, size_t size, float* dest);
void FloatToS16(const float* src, size_t size, int16_t* dest);
void S16ToFloat(const int16_t* src, size_t size, float* dest);
void FloatToFloatS16(const float* src, size_t size, float* dest);
void FloatS16ToFloat(const float* src, size_t size, float* dest);

typedef struct {
  int16_t y[4];
  int16_t x[2];
  const int16_t* ba;
} FilterState;

void init_highpass_filter(FilterState *hpf, uint32_t fs);
int highpass_filter(FilterState* hpf, int16_t* data, int length);

#ifdef __cplusplus
}
#endif


#endif
