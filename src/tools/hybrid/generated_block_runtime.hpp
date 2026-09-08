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

} // namespace oasis::hybrid::generated
