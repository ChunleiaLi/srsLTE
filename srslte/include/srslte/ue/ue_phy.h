/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2014 The srsLTE Developers. See the
 * COPYRIGHT file at the top-level directory of this distribution.
 *
 * \section LICENSE
 *
 * This file is part of the srsLTE library.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

/******************************************************************************
 *  File:         ue_phy.h
 *
 *  Description:  Top-level class wrapper for PHY layer.
 *
 *  Reference:
 *****************************************************************************/

#include "srslte/srslte.h"
#include "srslte/utils/queue.h"

#ifndef UEPHY_H
#define UEPHY_H

#define SYNC_MODE_CV       0
#define SYNC_MODE_CALLBACK 1
#define SYNC_MODE          SYNC_MODE_CV

namespace srslte {

typedef _Complex float cf_t; 

class ue_phy
{
public:
  
  typedef enum {DOWNLINK, UPLINK} direction_t; 

  typedef enum {
    PDCCH_UL_SEARCH_CRNTI = 0,
    PDCCH_UL_SEARCH_RA_PROC,
    PDCCH_UL_SEARCH_SPS,
    PDCCH_UL_SEARCH_TEMPORAL,
    PDCCH_UL_SEARCH_TPC_PUSCH,
    PDCCH_UL_SEARCH_TPC_PUCCH
  } pdcch_ul_search_t; 

  typedef enum {
    PDCCH_DL_SEARCH_CRNTI = 0,
    PDCCH_DL_SEARCH_SIRNTI,
    PDCCH_DL_SEARCH_PRNTI,
    PDCCH_DL_SEARCH_RARNTI,
    PDCCH_DL_SEARCH_TEMPORAL,    
    PDCCH_DL_SEARCH_SPS
  } pdcch_dl_search_t; 
    
  /* Uplink/Downlink scheduling grant generated by a successfully decoded PDCCH */ 
  class sched_grant {
  public:
    uint16_t get_rnti();
    uint32_t get_rv();
    void     set_rv(uint32_t rv);
    bool     get_ndi();
    bool     get_cqi_request();
    uint32_t get_harq_process();    
  private: 
    union {
      srslte_ra_pusch_t ul_grant;
      srslte_ra_pdsch_t dl_grant;
    }; 
    direction_t         dir; 
  };

  
  /* Uplink scheduling assignment. The MAC instructs the PHY to prepare an UL packet (PUSCH or PUCCH) 
   * for transmission. The MAC must call generate_pusch() to set the packet ready for transmission
   */
  class ul_buffer : public queue::element {
  public: 
             ul_buffer(srslte_cell_t cell);
    void     generate_pusch(sched_grant pusch_grant, uint8_t *payload, srslte_uci_data_t uci_data);    
    void     generate_pucch(srslte_uci_data_t uci_data);
  private: 
    srslte_ue_ul_t ue_ul; 
    bool           signal_generated = false; 
    cf_t*          signal_buffer    = NULL;
    uint32_t       tti              = 0; 
  };

  /* Class for the processing of Downlink buffers. The MAC obtains a buffer for a given TTI and then 
   * gets ul/dl scheduling grants and/or processes phich/pdsch channels 
   */
  class dl_buffer : public queue::element {
  public:
                 dl_buffer(srslte_cell_t cell);
    sched_grant  get_ul_grant(pdcch_ul_search_t mode, uint32_t rnti);
    sched_grant  get_dl_grant(pdcch_dl_search_t mode, uint32_t rnti);
    bool         decode_phich(sched_grant pusch_grant);
    bool         decode_pdsch(sched_grant pdsch_grant, uint8_t *payload); // returns true or false for CRC OK/KO
  private: 
    srslte_ue_dl_t ue_dl; 
    srslte_phich_t phich; 
    cf_t          *signal_buffer          = NULL; 
    bool           sf_symbols_and_ce_done = false; 
    bool           pdcch_llr_extracted    = false;    
    uint32_t       tti                    = 0; 
  };
  
  
#if SYNC_MODE==SYNC_MODE_CALLBACK
  typedef (*ue_phy_tti_clock_fcn_t) (void); 
  ue_phy(ue_phy_tti_clock_fcn_t tti_clock_callback);
#else
  ue_phy();
#endif
  ~ue_phy();
  
  void measure(); // TBD  
  void dl_bch();  
  void start_rxtx();
  void stop_rxtx();
  void init_prach();
  void send_prach(/* prach_cfg_t in prach.h with power, seq idx, etc */);  
  void set_param(); 

  uint32_t                get_tti();
#if SYNC_MODE==SYNC_MODE_CV
  std::condition_variable tti_cv; 
  std::mutex              tti_mutex; 
#endif
  
  ul_buffer get_ul_buffer(uint32_t tti);
  dl_buffer get_dl_buffer(uint32_t tti);
  
private:
  enum {
    IDLE, MEASURE, RX_BCH, RXTX
  } phy_state; 

  bool prach_initiated     = false;   
  bool prach_ready_to_send = false;   
  srslte_prach_t prach; 

  queue ul_buffer_queue;
  queue dl_buffer_queue; 
  
  pthread_t radio_thread; 
  void     *radio_handler; 
};

}
#endif
