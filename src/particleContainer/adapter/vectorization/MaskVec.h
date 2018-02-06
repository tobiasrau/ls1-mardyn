/*
 * MaskVec.h
 *
 *  Created on: 14 Nov 2016
 *      Author: tchipevn
 */

#ifndef SRC_PARTICLECONTAINER_ADAPTER_VECTORIZATION_MASKVEC_H_
#define SRC_PARTICLECONTAINER_ADAPTER_VECTORIZATION_MASKVEC_H_

#include "./SIMD_TYPES.h"
#include <type_traits>

namespace vcp {

template<typename FloatOrDouble>
class MaskVec {
private:
	vcp_mask_vec _m;

	static bool isFloat32() {
		return std::is_same<FloatOrDouble, float>::value;
	}

public:
	MaskVec() {}

	operator vcp_mask_vec() const {
		return _m;
	}

	MaskVec(const vcp_mask_vec & m) {
		_m = m;
	}

	static MaskVec zero() {
		#if   VCP_VEC_WIDTH == VCP_VEC_W__64
				return 0;
		#elif VCP_VEC_WIDTH == VCP_VEC_W_128
				return _mm_setzero_si128();
		#elif VCP_VEC_WIDTH == VCP_VEC_W_256
				return _mm256_setzero_si256();
		#elif VCP_VEC_WIDTH == VCP_VEC_W_512
				if(isFloat32) {
					return 0x0000;
				} else {
					return 0x00;
				}
		#endif
	}

	static MaskVec ones() {
		#if   VCP_VEC_WIDTH == VCP_VEC_W__64
				return ~0;
		#elif VCP_VEC_WIDTH == VCP_VEC_W_128
				return _mm_set_epi32(~0, ~0, ~0, ~0);
		#elif VCP_VEC_WIDTH == VCP_VEC_W_256
				return _mm256_set_epi32(~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0);
		#elif VCP_VEC_WIDTH == VCP_VEC_W_512
				if(isFloat32) {
					return 0xFFFF;
				} else {
					return 0xFF;
				}
		#endif
	}

	MaskVec operator and (const MaskVec& rhs) const {
		if(isFloat32()) {
			#if   VCP_VEC_WIDTH == VCP_VEC_W__64
					return _m & rhs;
			#elif VCP_VEC_WIDTH == VCP_VEC_W_128
					return _mm_and_si128(_m, rhs);
			#elif VCP_VEC_WIDTH == VCP_VEC_W_256
					return _mm256_castps_si256(_mm256_and_ps(_mm256_castsi256_ps(_m), _mm256_castsi256_ps(rhs)));
			#elif VCP_VEC_WIDTH == VCP_VEC_W_512
					return _m & rhs;
			#endif
		} else {
			#if   VCP_VEC_WIDTH == VCP_VEC_W__64
					return _m & rhs;
			#elif VCP_VEC_WIDTH == VCP_VEC_W_128
					return _mm_and_si128(_m, rhs);
			#elif VCP_VEC_WIDTH == VCP_VEC_W_256
					return _mm256_castpd_si256(_mm256_and_pd(_mm256_castsi256_pd(_m), _mm256_castsi256_pd(rhs)));
			#elif VCP_VEC_WIDTH == VCP_VEC_W_512
					return _m & rhs;
			#endif
		}
	}

	MaskVec operator or (const MaskVec& rhs) const {
		if(isFloat32()) {
			#if   VCP_VEC_WIDTH == VCP_VEC_W__64
					return _m | rhs;
			#elif VCP_VEC_WIDTH == VCP_VEC_W_128
					return _mm_or_si128(_m, rhs);
			#elif VCP_VEC_WIDTH == VCP_VEC_W_256
					return _mm256_castps_si256(_mm256_or_ps(_mm256_castsi256_ps(_m), _mm256_castsi256_ps(rhs)));
			#elif VCP_VEC_WIDTH == VCP_VEC_W_512
					return _m | rhs;
			#endif
		} else {
			#if   VCP_VEC_WIDTH == VCP_VEC_W__64
					return _m | rhs;
			#elif VCP_VEC_WIDTH == VCP_VEC_W_128
					return _mm_or_si128(_m, rhs);
			#elif VCP_VEC_WIDTH == VCP_VEC_W_256
					return _mm256_castpd_si256(_mm256_or_pd(_mm256_castsi256_pd(_m), _mm256_castsi256_pd(rhs)));
			#elif VCP_VEC_WIDTH == VCP_VEC_W_512
					return _m | rhs;
			#endif
		}
	}

	MaskVec operator xor (const MaskVec & rhs) const {
		if(isFloat32()) {
			#if   VCP_VEC_WIDTH == VCP_VEC_W__64
					return _m ^ rhs;
			#elif VCP_VEC_WIDTH == VCP_VEC_W_128
					return _mm_xor_si128(_m, rhs);
			#elif VCP_VEC_WIDTH == VCP_VEC_W_256
					return _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(_m), _mm256_castsi256_ps(rhs)));
			#elif VCP_VEC_WIDTH == VCP_VEC_W_512
					return _m ^ rhs;
			#endif
		} else {
			#if   VCP_VEC_WIDTH == VCP_VEC_W__64
					return _m xor rhs;
			#elif VCP_VEC_WIDTH == VCP_VEC_W_128
					return _mm_xor_si128(_m, rhs);
			#elif VCP_VEC_WIDTH == VCP_VEC_W_256
					return _mm256_castpd_si256(_mm256_xor_pd(_mm256_castsi256_pd(_m), _mm256_castsi256_pd(rhs)));
			#elif VCP_VEC_WIDTH == VCP_VEC_W_512
					return _m ^ rhs;
			#endif
		}
	}

	static MaskVec aligned_load(const vcp_mask_single * const a) {
		#if   VCP_VEC_WIDTH == VCP_VEC_W__64
				return *a;
		#elif VCP_VEC_WIDTH == VCP_VEC_W_128
				return _mm_load_si128((const __m128i*)a);
		#elif VCP_VEC_WIDTH == VCP_VEC_W_256
				return _mm256_load_si256((const __m256i*)a);
		#elif VCP_VEC_WIDTH == VCP_VEC_W_512
				return *a; // is this used ?
		#endif
	}


#if VCP_VEC_TYPE == VCP_VEC_KNL_GATHER
	static vcp_lookupOrMask_vec aligned_load(const vcp_lookupOrMask_single * const a) {

		if (isFloat32()) {
			return _mm512_load_epi32(a);
		} else {
			return _mm512_load_epi64(a);
		}
	}
#endif

	void aligned_store(vcp_mask_single * location) const {
		#if   VCP_VEC_WIDTH == VCP_VEC_W__64
				*location = _m;
		#elif VCP_VEC_WIDTH == VCP_VEC_W_128
				_mm_store_si128((__m128i*)location, _m);
		#elif VCP_VEC_WIDTH == VCP_VEC_W_256
				_mm256_store_si256((__m256i*)location, _m);
		#elif VCP_VEC_WIDTH == VCP_VEC_W_512
				*location = _m; // is this used ?
		#endif
	}

	int movemask() const {
		if (isFloat32()) {
			#if   VCP_VEC_WIDTH == VCP_VEC_W__64
					return _m != MaskVec::zero();
			#elif VCP_VEC_WIDTH == VCP_VEC_W_128
					return _mm_movemask_ps(_mm_castsi128_ps(_m));
			#elif VCP_VEC_WIDTH == VCP_VEC_W_256
					return _mm256_movemask_ps(_mm256_castsi256_ps(_m));
			#elif VCP_VEC_WIDTH == VCP_VEC_W_512
					return _m != MaskVec::zero();
			#endif
		} else {
			#if   VCP_VEC_WIDTH == VCP_VEC_W__64
					return _m != MaskVec::zero();
			#elif VCP_VEC_WIDTH == VCP_VEC_W_128
					return _mm_movemask_pd(_mm_castsi128_pd(_m));
			#elif VCP_VEC_WIDTH == VCP_VEC_W_256
					return _mm256_movemask_pd(_mm256_castsi256_pd(_m));
			#elif VCP_VEC_WIDTH == VCP_VEC_W_512
					return _m != MaskVec::zero();
			#endif
		}
	}

	int countUnmasked() const {
		#if   VCP_VEC_WIDTH == VCP_VEC_W__64
				return _m;
		#elif VCP_VEC_WIDTH == VCP_VEC_W_128 or VCP_VEC_WIDTH == VCP_VEC_W_256
				return __builtin_popcount(movemask());
		#elif VCP_VEC_WIDTH == VCP_VEC_W_512
				return __builtin_popcount(_m);
		#endif
	}
};

} /* namespace vcp */

#endif /* SRC_PARTICLECONTAINER_ADAPTER_VECTORIZATION_MASKVEC_H_ */
