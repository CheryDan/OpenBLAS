/***************************************************************************
Copyright (c) 2020, The OpenBLAS Project
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
3. Neither the name of the OpenBLAS project nor the names of
its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE OPENBLAS PROJECT OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include "common.h"
#include <math.h>
#include <float.h>

#if defined(DOUBLE)

#define VSETVL(n) __riscv_vsetvl_e64m4(n)
#define FLOAT_V_T vfloat64m4_t
#define FLOAT_V_T_M1 vfloat64m1_t
#define VLEV_FLOAT __riscv_vle64_v_f64m4
#define VLSEV_FLOAT __riscv_vlse64_v_f64m4
#define VFREDMAXVS_FLOAT __riscv_vfredmax_vs_f64m4_f64m1
#define MASK_T vbool16_t
#define VMFLTVV_FLOAT __riscv_vmflt_vv_f64m4_b16
#define VFMVVF_FLOAT __riscv_vfmv_v_f_f64m4
#define VFMVVF_FLOAT_M1 __riscv_vfmv_v_f_f64m1
#define VFMAXVV_FLOAT __riscv_vfmax_vv_f64m4
#define VMFGEVF_FLOAT __riscv_vmfge_vf_f64m4_b16
#define VMFIRSTM __riscv_vfirst_m_b16
#define UINT_V_T vuint64m4_t
#define VIDV_UINT __riscv_vid_v_u64m4
#define VADDVX_MASK_UINT __riscv_vadd_vx_u64m4_mu
#define VADDVX_UINT __riscv_vadd_vx_u64m4
#define VMVVX_UINT __riscv_vmv_v_x_u64m4
#define VFABS_FLOAT __riscv_vfabs_v_f64m4
#define VCOMPRESS __riscv_vcompress_vm_u64m4
#define VMV_X __riscv_vmv_x_s_u64m4_u64
#else

#define VSETVL(n) __riscv_vsetvl_e32m4(n)
#define FLOAT_V_T vfloat32m4_t
#define FLOAT_V_T_M1 vfloat32m1_t
#define VLEV_FLOAT __riscv_vle32_v_f32m4
#define VLSEV_FLOAT __riscv_vlse32_v_f32m4
#define VFREDMAXVS_FLOAT __riscv_vfredmax_vs_f32m4_f32m1
#define MASK_T vbool8_t
#define VMFLTVV_FLOAT __riscv_vmflt_vv_f32m4_b8
#define VFMVVF_FLOAT __riscv_vfmv_v_f_f32m4
#define VFMVVF_FLOAT_M1 __riscv_vfmv_v_f_f32m1
#define VFMAXVV_FLOAT __riscv_vfmax_vv_f32m4
#define VMFGEVF_FLOAT __riscv_vmfge_vf_f32m4_b8
#define VMFIRSTM __riscv_vfirst_m_b8
#define UINT_V_T vuint32m4_t
#define VIDV_UINT __riscv_vid_v_u32m4
#define VADDVX_MASK_UINT __riscv_vadd_vx_u32m4_mu
#define VADDVX_UINT __riscv_vadd_vx_u32m4
#define VMVVX_UINT __riscv_vmv_v_x_u32m4
#define VFABS_FLOAT __riscv_vfabs_v_f32m4
#define VCOMPRESS __riscv_vcompress_vm_u32m4
#define VMV_X __riscv_vmv_x_s_u32m4_u32
#endif


BLASLONG CNAME(BLASLONG n, FLOAT *x, BLASLONG inc_x)
{
        BLASLONG i=0, j=0;
        unsigned int max_index = 0;
        if (n <= 0 || inc_x <= 0) return(max_index);
        FLOAT maxf=-FLT_MAX;

        FLOAT_V_T vx, v_max;
        UINT_V_T v_max_index;
        MASK_T mask;
        unsigned int gvl = 0;
        FLOAT_V_T_M1 v_res;
        v_res = VFMVVF_FLOAT_M1(-FLT_MAX, 1);

        gvl = VSETVL(n);
        UINT_V_T vid = VIDV_UINT(gvl);

        if(inc_x == 1){
                v_max_index = VMVVX_UINT(0, gvl);
                v_max = VFMVVF_FLOAT(-FLT_MAX, gvl);
                for(i=0,j=0; i < n/gvl; i++){
                        vx = VLEV_FLOAT(&x[j], gvl);
                        vx = VFABS_FLOAT(vx, gvl);

                        //index where element greater than v_max
                        mask = VMFLTVV_FLOAT(v_max, vx, gvl);
                        v_max_index = VADDVX_MASK_UINT(mask, v_max_index, vid, j, gvl);

                        //update v_max and start_index j
                        v_max = VFMAXVV_FLOAT(v_max, vx, gvl);
                        j += gvl;
                }
                v_res = VFREDMAXVS_FLOAT(v_max, v_res, gvl);
                maxf = EXTRACT_FLOAT(v_res);
                mask = VMFGEVF_FLOAT(v_max, maxf, gvl);
                UINT_V_T compressed;
                compressed = VCOMPRESS(v_max_index, mask, gvl);
                max_index = VMV_X(compressed);

                if(j < n){
                        gvl = VSETVL(n-j);
                        v_max = VLEV_FLOAT(&x[j], gvl);
                        v_max = VFABS_FLOAT(v_max, gvl);

                        v_res = VFREDMAXVS_FLOAT(v_max, v_res, gvl);
                        FLOAT cur_maxf = EXTRACT_FLOAT(v_res);
                        if(cur_maxf > maxf){
                                //tail index
                                v_max_index = VADDVX_UINT(vid, j, gvl);

                                mask = VMFGEVF_FLOAT(v_max, cur_maxf, gvl);
                                UINT_V_T compressed;
                                compressed = VCOMPRESS(v_max_index, mask, gvl);
                                max_index = VMV_X(compressed);
                        }
                }
        }else{
                gvl = VSETVL(n);
                unsigned int stride_x = inc_x * sizeof(FLOAT);
                unsigned int idx = 0, inc_v = gvl * inc_x;

                v_max = VFMVVF_FLOAT(-FLT_MAX, gvl);
                v_max_index = VMVVX_UINT(0, gvl);
                for(i=0,j=0; i < n/gvl; i++){
                        vx = VLSEV_FLOAT(&x[idx], stride_x, gvl);
                        vx = VFABS_FLOAT(vx, gvl);

                        //index where element greater than v_max
                        mask = VMFLTVV_FLOAT(v_max, vx, gvl);
                        v_max_index = VADDVX_MASK_UINT(mask, v_max_index, vid, j, gvl);

                        //update v_max and start_index j
                        v_max = VFMAXVV_FLOAT(v_max, vx, gvl);
                        j += gvl;
                        idx += inc_v;
                }

                v_res = VFREDMAXVS_FLOAT(v_max, v_res, gvl);
                maxf = EXTRACT_FLOAT(v_res);
                mask = VMFGEVF_FLOAT(v_max, maxf, gvl);
                UINT_V_T compressed;
                compressed = VCOMPRESS(v_max_index, mask, gvl);
                max_index = VMV_X(compressed);

                if(j < n){
                        gvl = VSETVL(n-j);
                        v_max = VLSEV_FLOAT(&x[idx], stride_x, gvl);
                        v_max = VFABS_FLOAT(v_max, gvl);

                        v_res = VFREDMAXVS_FLOAT(v_max, v_res, gvl);
                        FLOAT cur_maxf = EXTRACT_FLOAT(v_res);

                        if(cur_maxf > maxf){
                                //tail index
                                v_max_index = VADDVX_UINT(vid, j, gvl);

                                mask = VMFGEVF_FLOAT(v_max, cur_maxf, gvl);

                                UINT_V_T compressed;
                                compressed = VCOMPRESS(v_max_index, mask, gvl);
                                max_index = VMV_X(compressed);
                        }
                }
        }
        return(max_index+1);
}
