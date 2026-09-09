#pragma once

#include "tools/hybrid/basic_block.hpp"

#include <cstdint>

namespace oasis::hybrid::generated {

std::uint32_t fetch_checked(BasicBlockApi& api, std::uint32_t expected);
void movem_l_predecrement(BasicBlockApi& api, std::uint32_t mask);
void clear_w_data_register(BasicBlockApi& api, unsigned data_register);
void move_b_postincrement_to_data_register(BasicBlockApi& api,
                                           unsigned address_register,
                                           unsigned data_register);
void lea_absolute_long(BasicBlockApi& api, std::uint32_t address,
                       unsigned address_register);
void adda_w_data_to_address(BasicBlockApi& api, unsigned data_register,
                            unsigned address_register);
void move_w_postincrement_to_postincrement(BasicBlockApi& api,
                                            unsigned source_register,
                                            unsigned destination_register);
void add_l_data_to_data(BasicBlockApi& api, unsigned source_register,
                        unsigned destination_register);
void add_w_postincrement_to_data_register(BasicBlockApi& api,
                                          unsigned address_register,
                                          unsigned data_register);
void branch_condition(BasicBlockApi& api, unsigned condition, unsigned target,
                      int not_taken_cycles, int extension = -1);
void dbcc(BasicBlockApi& api, unsigned condition, unsigned data_register,
          unsigned target, int extension = -1);
void test_absolute_long(BasicBlockApi& api, std::uint32_t address,
                        unsigned width);
void compare_immediate_w_absolute_long(BasicBlockApi& api, unsigned immediate,
                                       std::uint32_t address);
void compare_immediate_b_data(BasicBlockApi& api, unsigned immediate,
                               unsigned data_register);
void bit_test_immediate_absolute_long(BasicBlockApi& api, unsigned immediate,
                                      std::uint32_t address);
void bit_test_immediate_data(BasicBlockApi& api, unsigned immediate,
                             unsigned data_register);
void moveq_data(BasicBlockApi& api, int immediate, unsigned data_register);
void move_w_data_to_data(BasicBlockApi& api, unsigned source_register,
                         unsigned destination_register);
void add_w_data_to_data(BasicBlockApi& api, unsigned source_register,
                        unsigned destination_register);
void sub_w_data_to_data(BasicBlockApi& api, unsigned source_register,
                        unsigned destination_register);
void addq_w_data(BasicBlockApi& api, unsigned immediate, unsigned data_register);
void subq_b_data(BasicBlockApi& api, unsigned immediate, unsigned data_register);
void subq_w_data(BasicBlockApi& api, unsigned immediate, unsigned data_register);
void andi_w_data(BasicBlockApi& api, unsigned immediate, unsigned data_register);
void andi_b_data(BasicBlockApi& api, unsigned immediate, unsigned data_register);
void or_w_data_to_data(BasicBlockApi& api, unsigned source_register,
                       unsigned destination_register);
void addi_b_data(BasicBlockApi& api, unsigned immediate, unsigned data_register);
void subi_b_data(BasicBlockApi& api, unsigned immediate, unsigned data_register);
void ror_w_data(BasicBlockApi& api, unsigned count, unsigned data_register);
void lsr_w_data(BasicBlockApi& api, unsigned count, unsigned data_register);
void bclr_l_data(BasicBlockApi& api, unsigned immediate, unsigned data_register);
void movea_l_address_to_address(BasicBlockApi& api, unsigned source_register,
                                unsigned destination_register);

} // namespace oasis::hybrid::generated
