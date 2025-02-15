// Copyright(c) 2020-2021, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
#pragma once
#include "afu_test.h"
#include "host_exerciser.h"

using test_afu = opae::afu_test::afu;
using opae::fpga::types::shared_buffer;

namespace host_exerciser {

class host_exerciser_cmd : public test_command
{
public:
    host_exerciser_cmd()
        :host_exe_(NULL) {
          he_lpbk_cfg_.value = 0;
          he_lpbk_ctl_.value = 0;
    }
    virtual ~host_exerciser_cmd() {}

    void host_exerciser_status()
    {
        he_status0 he_status0;
        he_status1 he_status1;

        he_status0.value = host_exe_->read64(HE_STATUS0);
        he_status1.value = host_exe_->read64(HE_STATUS1);

        std::cout << "Host Exerciser numReads:" << he_status0.numReads << std::endl;
        std::cout << "Host Exerciser numWrites:" << he_status0.numWrites << std::endl;

        std::cout << "Host Exerciser numPendReads:" << he_status1.numPendReads << std::endl;
        std::cout << "Host Exerciser numPendWrites:" << he_status1.numPendWrites << std::endl;
    }

    void host_exerciser_errors()
    {
        he_error he_error;

        if (host_exe_ == NULL)
            return;

        he_error.value = host_exe_->read64(HE_ERROR);

        std::cout << "Host Exerciser Error:" << he_error.error << std::endl;

    }

    void host_exerciser_swtestmsg()
    {
        uint64_t swtest_msg;

        if (host_exe_ == NULL)
            return;

        swtest_msg = host_exe_->read64(HE_SWTEST_MSG);

        std::cout << "Host Exerciser swtest msg:" << swtest_msg << std::endl;
    }

    inline uint64_t cacheline_aligned_addr(uint64_t num) {
        return num >> LOG2_CL;
    }

    int parse_input_options()
    {

        if (!host_exe_)
            return -1;

        // Host Exerciser Mode
        he_lpbk_cfg_.TestMode = host_exe_->he_modes_;

        // Host Exerciser Read
        he_lpbk_cfg_.ReqLen = host_exe_->he_req_cls_len_;

        // Host Exerciser  lpbk delay
        if (host_exe_->he_delay_)
             he_lpbk_cfg_.DelayEn = 1;

        //test rollover or test termination
        if (host_exe_->he_continuousmode_)
             he_lpbk_cfg_.Continuous = 1;

        // Set interleave in Throughput
        if (he_lpbk_cfg_.TestMode == HOST_EXEMODE_TRUPT) {
              he_lpbk_cfg_.TputInterleave = host_exe_->he_interleave_;
        }

        return 0;
    }


    virtual int run(test_afu *afu, CLI::App *app)
    {
        (void)app;

        auto d_afu = dynamic_cast<host_exerciser*>(afu);
        host_exe_ = dynamic_cast<host_exerciser*>(afu);

        auto ret = parse_input_options();
        if (ret != 0) {
            std::cerr << "Failed to parese input options" << std::endl;
            return ret;
        }
        std::cout << "Input Config:" << he_lpbk_cfg_.value << std::endl;

        // Stop he-lpbk
        he_lpbk_ctl_.value = 0;
        he_lpbk_ctl_.ForcedTestCmpl = 1;
        d_afu->write32(HE_CTL, he_lpbk_ctl_.value);
        usleep(1000);


        // Reset he-lpbk
        he_lpbk_ctl_.value = 0;
        he_lpbk_ctl_.ResetL = 1;
        d_afu->write32(HE_CTL, he_lpbk_ctl_.value);


        /* Allocate Source Buffer
        Write to CSR_SRC_ADDR */
        std::cout << "Allocate SRC Buffer" << std::endl;
        source_ = d_afu->allocate(LPBK1_BUFFER_ALLOCATION_SIZE);
        d_afu->write64(HE_SRC_ADDR, cacheline_aligned_addr(source_->io_address()));
        std::fill_n(source_->c_type(), LPBK1_BUFFER_SIZE, 0xAF);

        /* Allocate Destination Buffer
            Write to CSR_DST_ADDR */
        std::cout << "Allocate DST Buffer" << std::endl;
        destination_ = d_afu->allocate(LPBK1_BUFFER_ALLOCATION_SIZE);
        d_afu->write64(HE_DST_ADDR, cacheline_aligned_addr(destination_->io_address()));
        std::fill_n(destination_->c_type(), LPBK1_BUFFER_SIZE, 0xBE);

        /* Allocate DSM Buffer
            Write to CSR_AFU_DSM_BASEL */
        std::cout << "Allocate DSM Buffer" << std::endl;
        dsm_ = d_afu->allocate(LPBK1_DSM_SIZE);
        d_afu->write32(HE_DSM_BASEL, cacheline_aligned_addr(dsm_->io_address()));
        std::fill_n(dsm_->c_type(), LPBK1_DSM_SIZE, 0x0);


        // Number of cache lines
        d_afu->write64(HE_NUM_LINES, (LPBK1_BUFFER_SIZE / (1 * CL)) -1);

       // Write to CSR_CFG
        d_afu->write32(HE_CFG, he_lpbk_cfg_.value);

        // Write to CSR_CTL
        std::cout << "Start Test" << std::endl;
        he_lpbk_ctl_.value = 0;
        he_lpbk_ctl_.Start = 1;
        he_lpbk_ctl_.ResetL = 1;
        d_afu->write32(HE_CTL, he_lpbk_ctl_.value);

        /* Wait for test completion */
        uint32_t           timeout = HELPBK_TEST_TIMEOUT;
        volatile uint8_t* status_ptr = dsm_->c_type() ;


        while (0 == ((*status_ptr) & 0x1))
        {
            usleep(HELPBK_TEST_SLEEP_INVL);
            if (--timeout == 0) {
                std::cout << "HE LPBK TIME OUT" << std::endl;
                host_exerciser_errors();
                return -1;
            }
        }


        std::cout << "Test Completed" << std::endl;
        host_exerciser_swtestmsg();
        host_exerciser_status();

        /* Compare buffer contents only loopback test mode*/
        if (he_lpbk_cfg_.TestMode == HOST_EXEMODE_LPBK1)
            d_afu->compare(source_, destination_);

        return 0;
    }

protected:
    he_cfg he_lpbk_cfg_;
    he_ctl he_lpbk_ctl_;
    host_exerciser *host_exe_;
    shared_buffer::ptr_t source_;
    shared_buffer::ptr_t destination_;
    shared_buffer::ptr_t dsm_;
};

} // end of namespace host_exerciser
