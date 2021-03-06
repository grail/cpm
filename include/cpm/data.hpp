//=======================================================================
// Copyright (c) 2015-2016 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef CPM_DATA_HPP
#define CPM_DATA_HPP

#include "cpm/rapidjson.hpp"

namespace cpm {

using document_t = rapidjson::Document;
using document_ref = std::reference_wrapper<document_t>;
using document_cref = std::reference_wrapper<const document_t>;

struct reports_data {
    std::set<std::string> compilers;
    std::set<std::string> configurations;
    std::vector<document_t> documents;

    //Temporary data (changed for each generated file)
    std::string file;
    std::string sub_part;
    std::vector<std::pair<std::string, std::string>> files;
};

} //end of namespace cpm

#endif //CPM_DATA_HPP
