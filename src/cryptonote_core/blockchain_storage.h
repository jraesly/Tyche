// Copyright (c) 2014, AEON, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/list.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/global_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/foreach.hpp>
#include <atomic>
#include <cstddef>

#include "syncobj.h"
#include "string_tools.h"
#include "tx_pool.h"
#include "cryptonote_basic.h"
#include "common/util.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "difficulty.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "verification_context.h"
#include "crypto/hash.h"
#include "checkpoints.h"

namespace boost
{
  namespace serialization
  {
    template<class Archive>
    void serialize(Archive & ar, std::tuple<uint32_t,uint32_t,uint32_t>& v, const unsigned int version)
    {
      ar & std::get<0>(v);
      ar & std::get<1>(v);
      ar & std::get<2>(v);
    }
  }
}

namespace cryptonote
{

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class blockchain_storage
  {
  public:
    struct transaction_chain_entry
    {
      transaction tx;
      uint64_t m_keeper_block_height;
      std::vector<uint64_t> m_global_output_indexes;
    };

    struct block_extended_info
    {
      block   bl;
      uint64_t height;
      size_t block_cumulative_size;
      difficulty_type cumulative_difficulty;
      uint64_t already_generated_coins;
    };

    blockchain_storage(tx_memory_pool& tx_pool):m_tx_pool(tx_pool), m_current_block_cumul_sz_limit(0), m_is_in_checkpoint_zone(false), m_is_blockchain_storing(false)
    {};

    //    bool init() { return init(tools::get_default_data_dir()); }
    bool init(const std::string& config_folder, bool pruning_enabled);
    bool deinit();

    void set_checkpoints(checkpoints&& chk_pts) { m_checkpoints = chk_pts; }

    //bool push_new_block();
    bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks, std::list<transaction>& txs);
    bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks);
    bool get_alternative_blocks(std::list<block>& blocks);
    size_t get_alternative_blocks_count();
    crypto::hash get_block_id_by_height(uint64_t height);
    bool get_block_by_hash(const crypto::hash &h, block &blk);
    void get_all_known_block_ids(std::list<crypto::hash> &main, std::list<crypto::hash> &alt, std::list<crypto::hash> &invalid);

    template<class archive_t>
    void serialize(archive_t & ar, const unsigned int version);

    bool have_tx(const crypto::hash &id);
    bool have_tx_keyimges_as_spent(const transaction &tx);
    bool have_tx_keyimg_as_spent(const crypto::key_image &key_im);
    transaction *get_tx(const crypto::hash &id);

    template<class visitor_t>
    bool scan_outputkeys_for_indexes(const txin_to_key& tx_in_to_key, visitor_t& vis, uint64_t* pmax_related_block_height = NULL);

    uint64_t get_current_blockchain_height();
    crypto::hash get_tail_id();
    crypto::hash get_tail_id(uint64_t& height);
    difficulty_type get_difficulty_for_next_block();
    bool add_new_block(const block& bl_, block_verification_context& bvc);
    bool reset_and_set_genesis_block(const block& b);
    bool create_block_template(block& b, const account_public_address& miner_address, difficulty_type& di, uint64_t& height, const blobdata& ex_nonce);
    bool have_block(const crypto::hash& id);
    size_t get_total_transactions();
    bool get_outs(uint64_t amount, std::list<crypto::public_key>& pkeys);
    bool get_short_chain_history(std::list<crypto::hash>& ids);
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp);
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, uint64_t& starter_offset);
    bool find_blockchain_supplement(const uint64_t req_start_block, const std::list<crypto::hash>& qblock_ids, std::list<std::pair<block, std::list<transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count);
    bool handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp, bool& pruned);
    bool handle_get_objects(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res);
    bool get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res);
    bool get_backward_blocks_sizes(size_t from_height, std::vector<size_t>& sz, size_t count);
    bool get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs);
    bool store_blockchain();
    bool check_tx_input(const txin_to_key& txin, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig, uint64_t* pmax_related_block_height = NULL);
    bool check_tx_inputs(const transaction& tx, const crypto::hash& tx_prefix_hash, uint64_t* pmax_used_block_height = NULL);
    bool check_tx_inputs(const transaction& tx, uint64_t* pmax_used_block_height = NULL);
    bool check_tx_inputs(const transaction& tx, uint64_t& pmax_used_block_height, crypto::hash& max_used_block_id);
    uint64_t get_current_comulative_blocksize_limit();
    bool is_storing_blockchain(){return m_is_blockchain_storing;}
    uint64_t block_difficulty(size_t i);

    template<class t_ids_container, class t_blocks_container, class t_missed_container>
    bool get_blocks(const t_ids_container& block_ids, t_blocks_container& blocks, t_missed_container& missed_bs)
    {
      CRITICAL_REGION_LOCAL(m_blockchain_lock);

      BOOST_FOREACH(const auto& bl_id, block_ids)
      {
        auto it = m_blocks_index.find(bl_id);
        if(it == m_blocks_index.end())
          missed_bs.push_back(bl_id);
        else
        {
          CHECK_AND_ASSERT_MES(it->second < m_blocks.size(), false, "Internal error: bl_id=" << epee::string_tools::pod_to_hex(bl_id)
            << " have index record with offset="<<it->second<< ", bigger then m_blocks.size()=" << m_blocks.size());
            blocks.push_back(m_blocks[it->second].bl);
        }
      }
      return true;
    }

    template<class t_ids_container, class t_tx_container, class t_missed_container>
    bool get_transactions(const t_ids_container& txs_ids, t_tx_container& txs, t_missed_container& missed_txs)
    {
      CRITICAL_REGION_LOCAL(m_blockchain_lock);

      BOOST_FOREACH(const auto& tx_id, txs_ids)
      {
        auto it = m_transactions.find(tx_id);
        if(it == m_transactions.end())
        {
          transaction tx;
          if(!m_tx_pool.get_transaction(tx_id, tx))
            missed_txs.push_back(tx_id);
          else
            txs.push_back(tx);
        }
        else
          txs.push_back(it->second.tx);
      }
      return true;
    }
    //debug functions
    void print_blockchain(uint64_t start_index, uint64_t end_index);
    void print_blockchain_index();
    void print_blockchain_outs(const std::string& file);

  private:
    typedef std::unordered_map<crypto::hash, size_t> blocks_by_id_index;
    typedef std::unordered_map<crypto::hash, transaction_chain_entry> transactions_container;
    typedef std::unordered_set<crypto::key_image> key_images_container;
    typedef std::vector<block_extended_info> blocks_container;
    typedef std::unordered_map<crypto::hash, block_extended_info> blocks_ext_by_hash;
    typedef std::map<uint64_t, std::vector<std::tuple<uint32_t,uint32_t,uint32_t>>> outputs_container; // block, tx in block, output in tx

    tx_memory_pool& m_tx_pool;
    epee::critical_section m_blockchain_lock; // TODO: add here reader/writer lock

    // main chain
    blocks_container m_blocks;               // height  -> block_extended_info
    blocks_by_id_index m_blocks_index;       // crypto::hash -> height
    transactions_container m_transactions;
    key_images_container m_spent_keys;
    size_t m_current_block_cumul_sz_limit;

    // all alternative chains
    blocks_ext_by_hash m_alternative_chains; // crypto::hash -> block_extended_info

    // some invalid blocks
    blocks_ext_by_hash m_invalid_blocks;     // crypto::hash -> block_extended_info
    outputs_container m_outputs;

    std::string m_config_folder;
    bool m_pruning_enabled;
    checkpoints m_checkpoints;
    std::atomic<bool> m_is_in_checkpoint_zone;
    std::atomic<bool> m_is_blockchain_storing;

    bool switch_to_alternative_blockchain(std::list<blocks_ext_by_hash::iterator>& alt_chain, bool discard_disconnected_chain);
    bool pop_block_from_blockchain();
    bool purge_block_data_from_blockchain(const block& b, size_t processed_tx_count);
    bool purge_transaction_from_blockchain(const crypto::hash& tx_id);
    bool purge_transaction_keyimages_from_blockchain(const transaction& tx, bool strict_check);
    bool is_block_pruned(const block& bl);
    bool is_block_prunable(const block& bl);
    bool prune_stored_block(uint64_t bl_height);
    bool handle_block_to_main_chain(const block& bl, block_verification_context& bvc);
    bool handle_block_to_main_chain(const block& bl, const crypto::hash& id, block_verification_context& bvc);
    bool handle_alternative_block(const block& b, const crypto::hash& id, block_verification_context& bvc);
    difficulty_type get_next_difficulty_for_alternative_chain(const std::list<blocks_ext_by_hash::iterator>& alt_chain, block_extended_info& bei);
    bool prevalidate_miner_transaction(const block& b, uint64_t height);
    bool validate_miner_transaction(const block& b, size_t cumulative_block_size, uint64_t fee, uint64_t& base_reward, uint64_t already_generated_coins, size_t height);
    bool validate_transaction(const block& b, uint64_t height, const transaction& tx);
    bool rollback_blockchain_switching(std::list<block>& original_chain, size_t rollback_height);
    bool add_transaction_from_block(const transaction& tx, const crypto::hash& tx_id, const crypto::hash& bl_id, uint64_t bl_height, size_t index_in_block);
    bool push_transaction_to_global_outs_index(const transaction& tx, const crypto::hash& tx_id, std::vector<uint64_t>& global_indexes, uint64_t bl_height, size_t index_in_block);
    bool pop_transaction_from_global_index(const transaction& tx, const crypto::hash& tx_id);
    bool get_last_n_blocks_sizes(std::vector<size_t>& sz, size_t count);
    bool add_out_to_get_random_outs(std::vector<std::tuple<uint32_t,uint32_t,uint32_t>>& amount_outs, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& result_outs, uint64_t amount, size_t i);
    bool is_tx_spendtime_unlocked(uint64_t unlock_time);
    bool add_block_as_invalid(const block& bl, const crypto::hash& h);
    bool add_block_as_invalid(const block_extended_info& bei, const crypto::hash& h);
    crypto::hash get_txid_by_out_index(const std::tuple<uint32_t,uint32_t,uint32_t>& out_entry) const;
    size_t find_end_of_allowed_index(const std::vector<std::tuple<uint32_t,uint32_t,uint32_t>>& amount_outs);
    bool check_block_timestamp_main(const block& b);
    bool check_block_timestamp(std::vector<uint64_t> timestamps, const block& b);
    uint64_t get_adjusted_time();
    bool complete_timestamps_vector(uint64_t start_height, std::vector<uint64_t>& timestamps);
    bool update_next_comulative_size_limit();
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  #define CURRENT_BLOCKCHAIN_STORAGE_ARCHIVE_VER    13

  template<class archive_t>
  void blockchain_storage::serialize(archive_t & ar, const unsigned int version)
  {
    if(version < 11)
      return;
    CRITICAL_REGION_LOCAL(m_blockchain_lock);
    ar & m_blocks;
    ar & m_blocks_index;
    ar & m_transactions;
    if (archive_t::is_loading::value && m_pruning_enabled && m_blocks.size() > PRUNING_DEPTH) 
    {
      size_t pruned_count = 0;
      for (size_t i=0; i<m_blocks.size()-PRUNING_DEPTH; i++) 
      {
	if (is_block_prunable(m_blocks[i].bl))
	{
	  if (pruned_count == 0)
	  {
	    LOG_PRINT_L0("Pruning stored blocks...");
	  }
	  prune_stored_block(i);
	  pruned_count++;
	}
      }
      if (pruned_count > 0)
      {
	LOG_PRINT_L0("Pruned " << pruned_count << " blocks (save blockchain and restart to minimize memory usage)");
      }
    } 
    ar & m_spent_keys;
    ar & m_alternative_chains;
    if (version < 13)
    {
      LOG_PRINT_L0("Upgrading blockchain format...");
      typedef std::map<uint64_t, std::vector<std::pair<crypto::hash, size_t>>> old_outputs_container; //crypto::hash - tx hash, size_t - index of out in transaction
      old_outputs_container old_outs;
      ar & old_outs;
      for (const auto& entry : old_outs)
      {
	if (entry.second.size() == 0) {
	  m_outputs[entry.first];
	}
	for (const auto& p : entry.second)
	{
	  const auto& txid = p.first;
	  const auto& tce = m_transactions[txid];
	  const auto& bei = m_blocks[tce.m_keeper_block_height];
	  uint32_t i=0;
	  if (txid != get_transaction_hash(bei.bl.miner_tx))
	  {
	    ++i;
	    for (const auto& t : bei.bl.tx_hashes)
	    {
	      if (txid == t)
		break;
	      ++i;
	    }
	    if (i > bei.bl.tx_hashes.size()) 
	    {
	      LOG_ERROR("Blockchain storage data corruption detected. unable to mach txid to block");
	      throw std::runtime_error("Blockchain data corruption");
	    }
	  }
	  m_outputs[entry.first].push_back(std::make_tuple((uint32_t)tce.m_keeper_block_height,i,(uint32_t)p.second));
	}
      }	   
    }
    else
      ar & m_outputs;
    ar & m_invalid_blocks;
    ar & m_current_block_cumul_sz_limit;
    /*serialization bug workaround*/
    if(version > 11)
    {
      uint64_t total_check_count = m_blocks.size() + m_blocks_index.size() + m_transactions.size() + m_spent_keys.size() + m_alternative_chains.size() + m_outputs.size() + m_invalid_blocks.size() + m_current_block_cumul_sz_limit;
      if(archive_t::is_saving::value)
      {        
        ar & total_check_count;
      }else
      {
        uint64_t total_check_count_loaded = 0;
        ar & total_check_count_loaded;
        if(total_check_count != total_check_count_loaded)
        {
          LOG_ERROR("Blockchain storage data corruption detected. total_count loaded from file = " << total_check_count_loaded << ", expected = " << total_check_count);

          LOG_PRINT_L0("Blockchain storage:" << ENDL << 
            "m_blocks: " << m_blocks.size() << ENDL  << 
            "m_blocks_index: " << m_blocks_index.size() << ENDL  << 
            "m_transactions: " << m_transactions.size() << ENDL  << 
            "m_spent_keys: " << m_spent_keys.size() << ENDL  << 
            "m_alternative_chains: " << m_alternative_chains.size() << ENDL  << 
            "m_outputs: " << m_outputs.size() << ENDL  << 
            "m_invalid_blocks: " << m_invalid_blocks.size() << ENDL  << 
            "m_current_block_cumul_sz_limit: " << m_current_block_cumul_sz_limit);

          throw std::runtime_error("Blockchain data corruption");
        }
      }
    }


    size_t count =0;
    for (const auto& vec : m_outputs)
      count += vec.second.size();

    size_t total_blob_size =0;
    for (const auto& bei : m_blocks) {
      total_blob_size += block_to_blob(bei.bl).size();
      for (const auto& txid : bei.bl.tx_hashes)
	total_blob_size += tx_to_blob(m_transactions[txid].tx).size();
    }

    size_t total_blob_size_headers =0;
    for (const auto& bei : m_blocks) {
      total_blob_size_headers += block_to_blob(bei.bl).size();
      for (const auto& txid : bei.bl.tx_hashes) {
	transaction_prefix p = m_transactions[txid].tx;
	total_blob_size_headers += t_serializable_object_to_blob(p).size();
      }
    }

    LOG_PRINT_L2("Blockchain storage:" << ENDL << 
        "m_blocks: " << m_blocks.size() << ENDL  << 
        "total_blob_size: " << total_blob_size << ENDL  << 
        "total_blob_size_headers: " << total_blob_size_headers << ENDL  << 
        "m_blocks_index: " << m_blocks_index.size() << ENDL  << 
        "m_transactions: " << m_transactions.size() << ENDL  << 
        "m_spent_keys: " << m_spent_keys.size() << ENDL  << 
        "m_alternative_chains: " << m_alternative_chains.size() << ENDL  << 
        "m_outputs: " << m_outputs.size() << ENDL  << 
        "m_outputs_total_entries: " << count << ENDL  << 
        "m_invalid_blocks: " << m_invalid_blocks.size() << ENDL  << 
        "m_current_block_cumul_sz_limit: " << m_current_block_cumul_sz_limit);
  }

  //------------------------------------------------------------------
  template<class visitor_t>
  bool blockchain_storage::scan_outputkeys_for_indexes(const txin_to_key& tx_in_to_key, visitor_t& vis, uint64_t* pmax_related_block_height)
  {
    CRITICAL_REGION_LOCAL(m_blockchain_lock);
    auto it = m_outputs.find(tx_in_to_key.amount);
    if(it == m_outputs.end() || !tx_in_to_key.key_offsets.size())
      return false;

    std::vector<uint64_t> absolute_offsets = relative_output_offsets_to_absolute(tx_in_to_key.key_offsets);

    std::vector<std::tuple<uint32_t,uint32_t,uint32_t>>& amount_outs_vec = it->second;
    size_t count = 0;
    BOOST_FOREACH(uint64_t i, absolute_offsets)
    {
      if(i >= amount_outs_vec.size() )
      {
        LOG_PRINT_L0("Wrong index in transaction inputs: " << i << ", expected maximum " << amount_outs_vec.size() - 1);
        return false;
      }
      auto tx_id = get_txid_by_out_index(amount_outs_vec[i]);
      transactions_container::iterator tx_it = m_transactions.find(tx_id);
      CHECK_AND_ASSERT_MES(tx_it != m_transactions.end(), false, "Wrong transaction id in output indexes: " << tx_id);
      size_t out_idx = std::get<2>(amount_outs_vec[i]);
      CHECK_AND_ASSERT_MES(out_idx < tx_it->second.tx.vout.size(), false,
        "Wrong index in transaction outputs: " << out_idx << ", expected less then " << tx_it->second.tx.vout.size());
      if(!vis.handle_output(tx_it->second.tx, tx_it->second.tx.vout[out_idx]))
      {
        LOG_PRINT_L0("Failed to handle_output for output no = " << count << ", with absolute offset " << i);
        return false;
      }
      if(count++ == absolute_offsets.size()-1 && pmax_related_block_height)
      {
        if(*pmax_related_block_height < tx_it->second.m_keeper_block_height)
          *pmax_related_block_height = tx_it->second.m_keeper_block_height;
      }
    }

    return true;
  }
}



BOOST_CLASS_VERSION(cryptonote::blockchain_storage, CURRENT_BLOCKCHAIN_STORAGE_ARCHIVE_VER)
