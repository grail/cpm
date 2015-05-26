//=======================================================================
// Copyright (c) 2015 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef CPM_CPM_HPP
#define CPM_CPM_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>
#include <functional>

#include <sys/utsname.h>

#include "compiler.hpp"
#include "duration.hpp"
#include "random.hpp"
#include "policy.hpp"
#include "io.hpp"
#include "json.hpp"

namespace cpm {

template<bool Sizes, typename Tuple, typename Functor, std::size_t... I, typename... Args, std::enable_if_t<Sizes, int> = 42>
inline void call_with_data_final(Tuple& data, Functor& functor, std::index_sequence<I...> /*indices*/, Args... args){
    functor(args..., std::get<I>(data)...);
}

template<bool Sizes, typename Tuple, typename Functor, std::size_t... I, typename... Args, std::enable_if_t<!Sizes, int> = 42>
inline void call_with_data_final(Tuple& data, Functor& functor, std::index_sequence<I...> /*indices*/, Args... /*args*/){
    functor(std::get<I>(data)...);
}

template<bool Sizes, typename Tuple, typename Functor, std::size_t... I>
inline void call_with_data(Tuple& data, Functor& functor, std::index_sequence<I...> indices, std::size_t arg){
    call_with_data_final<Sizes>(data, functor, indices, arg);
}

#ifndef CPM_PROPAGATE_TUPLE
template<bool Sizes, typename Tuple, typename Functor, typename... TT, std::size_t... I, std::size_t... I2>
inline void propagate_call_with_data(Tuple& data, Functor& functor, std::index_sequence<I...> indices, std::index_sequence<I2...> /*i2*/, std::tuple<TT...> arg){
    call_with_data_final<Sizes>(data, functor, indices, std::get<I2>(arg)...);
}
template<bool Sizes, typename Tuple, typename Functor, typename... TT, std::size_t... I>
inline void call_with_data(Tuple& data, Functor& functor, std::index_sequence<I...> indices, std::tuple<TT...> arg){
    propagate_call_with_data<Sizes>(data, functor, indices, std::make_index_sequence<sizeof...(TT)>(), arg);
}
#else
template<bool Sizes, typename Tuple, typename Functor, typename... TT, std::size_t... I>
inline void call_with_data(Tuple& data, Functor& functor, std::index_sequence<I...> indices, std::tuple<TT...> arg){
    call_with_data_final<Sizes>(data, functor, indices, arg);
}
#endif

template<typename Functor>
void call_functor(Functor& functor){
    functor();
}

template<typename Functor>
void call_functor(Functor& functor, std::size_t d){
    functor(d);
}

#ifndef CPM_PROPAGATE_TUPLE
template<typename Functor, typename... TT, std::size_t... I>
void propagate_call_functor(Functor& functor, std::tuple<TT...> d, std::index_sequence<I...> /*s*/){
    functor(std::get<I>(d)...);
}

template<typename Functor, typename... TT>
void call_functor(Functor& functor, std::tuple<TT...> d){
    propagate_call_functor(functor, d, std::make_index_sequence<sizeof...(TT)>());
}
#else
template<typename Functor, typename... TT>
void call_functor(Functor& functor, std::tuple<TT...> d){
    functor(d);
}
#endif

template<typename Functor>
auto call_init_functor(Functor& functor, std::size_t d){
    return functor(d);
}

#ifndef CPM_PROPAGATE_TUPLE
template<typename Functor, typename... TT, std::size_t... I>
auto propagate_call_init_functor(Functor& functor, std::tuple<TT...> d, std::index_sequence<I...> /*s*/){
    return functor(std::get<I>(d)...);
}

template<typename Functor, typename... TT>
auto call_init_functor(Functor& functor, std::tuple<TT...> d){
    return propagate_call_init_functor(functor, d, std::make_index_sequence<sizeof...(TT)>());
}
#else
template<typename Functor, typename... TT>
auto call_init_functor(Functor& functor, std::tuple<TT...> d){
    return functor(d);
}
#endif

template<typename DefaultPolicy = std_stop_policy>
struct benchmark;

struct section_data {
    std::string name;

    //TODO This datastructure is probably not ideal
    std::vector<std::string> names;
    std::vector<std::string> sizes;
    std::vector<std::vector<std::size_t>> results;

    section_data() = default;
    section_data(const section_data&) = default;
    section_data& operator=(const section_data&) = default;
    section_data(section_data&&) = default;
    section_data& operator=(section_data&&) = default;
};

template<typename Bench, typename Policy>
struct section {
private:
    Bench& bench;

    section_data data;

public:
    std::size_t warmup = 10;
    std::size_t repeat = 50;

    section(std::string name, Bench& bench) : bench(bench), warmup(bench.warmup), repeat(bench.repeat) {
        data.name = std::move(name);

        if(bench.standard_report){
            std::cout << std::endl;
        }
    }

    //Measure once functor (no policy, no randomization)

    template<typename Functor>
    void measure_once(const std::string& title, Functor functor){
        auto duration = bench.measure_only_simple(*this, functor);
        report(title, std::size_t(1), duration);
    }

    //Measure simple functor (no randomization)

    template<typename Functor>
    void measure_simple(const std::string& title, Functor functor){
        bench.template policy_run<Policy>(
            [&title, &functor, this](auto sizes){
                auto duration = bench.measure_only_simple(*this, functor, sizes);
                report(title, sizes, duration);
                return duration;
            }
        );
    }

    //Measure with two-pass functors (init and functor)

    template<bool Sizes = true, typename Init, typename Functor>
    void measure_two_pass(const std::string& title, Init init, Functor functor){
        bench.template policy_run<Policy>(
            [&title, &functor, &init, this](auto sizes){
                auto duration = bench.template measure_only_two_pass<Sizes>(*this, init, functor, sizes);
                report(title, sizes, duration);
                return duration;
            }
        );
    }

    //measure a function with global references

    template<typename Functor, typename... T>
    void measure_global(const std::string& title, Functor functor, T&... references){
        bench.template policy_run<Policy>(
            [&title, &functor, &references..., this](auto sizes){
                auto duration = bench.measure_only_global(*this, functor, sizes, references...);
                report(title, sizes, duration);
                return duration;
            }
        );
    }

    ~section(){
        if(bench.standard_report){
            if(data.names.empty()){
                return;
            }

            std::vector<int> widths(1 + data.results.size(), 4);

            for(std::size_t i = 0; i < data.sizes.size(); ++i){
                auto s = data.sizes[i];

                widths[0] = std::max(widths[0], static_cast<int>(s.size()));
            }

            widths[0] = std::max(widths[0], static_cast<int>(data.name.size()));

            for(std::size_t i = 0; i < data.results.size(); ++i){
                for(auto d : data.results[i]){
                    widths[i+1] = std::max(widths[i+1], static_cast<int>(us_duration_str(d).size()));
                }
            }

            for(std::size_t i = 0; i < data.names.size(); ++i){
                widths[i+1] = std::max(widths[i+1], static_cast<int>(data.names[i].size()));
            }

            std::size_t tot_width = 1 + std::accumulate(widths.begin(), widths.end(), 0) + 3 * (1 + data.names.size());

            std::cout << " " << std::string(tot_width, '-') << std::endl;;

            printf(" | %*s | ", widths[0], data.name.c_str());
            for(std::size_t i = 0; i < data.names.size(); ++i){
                printf("%*s | ", widths[i+1], data.names[i].c_str());
            }
            printf("\n");

            std::cout << " " << std::string(tot_width, '-') << std::endl;;

            for(std::size_t i = 0; i < data.sizes.size(); ++i){
                auto s = data.sizes[i];

                printf(" | %*s | ", widths[0], s.c_str());

                std::size_t max = 0;
                std::size_t min = std::numeric_limits<int>::max();

                for(std::size_t r = 0; r < data.results.size(); ++r){
                    if(i < data.results[r].size()){
                        max = std::max(max, data.results[r][i]);
                        min = std::min(min, data.results[r][i]);
                    }
                }

                for(std::size_t r = 0; r < data.results.size(); ++r){
                    if(i < data.results[r].size()){
                        if(data.results[r][i] >= 0.99 * static_cast<double>(min) && data.results[r][i] <= 1.01 * static_cast<double>(min)){
                            std::cout << "\033[0;32m";
                        } else if(data.results[r][i] >= 0.99 * static_cast<double>(max) && data.results[r][i] <= 1.01 * static_cast<double>(max)){
                            std::cout << "\033[0;31m";
                        }

                        printf("%*s", widths[r+1], us_duration_str(data.results[r][i]).c_str());
                    } else {
                        std::cout << "\033[0;31m";
                        printf("%*s", widths[r+1], "*");
                    }

                    //Reset the color
                    std::cout << "" << '\033' << "[" << 0 << ";" << 30 << 47 << "m | ";
                }

                printf("\n");
            }

            std::cout << " " << std::string(tot_width, '-') << std::endl;;
        }

        bench.section_results.push_back(std::move(data));
    }

private:
    template<typename Tuple>
    void report(const std::string& title, Tuple d, std::size_t duration){
        if(data.names.empty() || data.names.back() != title){
            data.names.push_back(title);
            data.results.emplace_back();
        }

        if(data.names.size() == 1){
            data.sizes.push_back(size_to_string(d));
        }

        data.results.back().push_back(duration);
    }
};

struct measure_data {
    std::string title;
    std::vector<std::pair<std::string, std::size_t>> results;
};

template<typename DefaultPolicy>
struct benchmark {
private:
    template<typename Bench, typename Policy>
    friend struct section;

    std::size_t tests = 0;
    std::size_t measures = 0;
    std::size_t runs = 0;

    const std::string name;
    std::string folder;
    std::string tag;
    std::string final_file;
    bool folder_ok = false;

    std::string operating_system;
    wall_time_point start_time;

    std::vector<measure_data> results;
    std::vector<section_data> section_results;

public:
    std::size_t warmup = 10;
    std::size_t repeat = 50;

    bool standard_report = true;
    bool auto_save = true;
    bool auto_mkdir = true;

    benchmark(std::string name, std::string f = ".", std::string t = "") : name(std::move(name)), folder(std::move(f)), tag(std::move(t)) {
        //Get absolute cwd
        if(folder == "" || folder == "."){
            folder = get_cwd();
        }

        //Make it absolute
        if(folder.front() != '/'){
            folder = make_absolute(folder);
        }

        //Ensure that the results folder exists
        if(!folder_exists(folder)){
            if(auto_mkdir){
                if(mkdir(folder.c_str(), 0777)){
                    std::cout << "Failed to create the folder" << std::endl;
                } else {
                    folder_ok = true;
                }
            } else {
                std::cout << "Warning: The given folder does not exist or is not a folder" << std::endl;
            }
        } else {
            folder_ok = true;
        }

        //Make sure the folder ends with /
        if(folder_ok && folder.back() != '/'){
            folder += "/";
        }

        //If no tag is provided, select one that does not yet exists
        if(folder_ok && tag.empty()){
            tag = get_free_file(folder);
        }

        final_file = folder + tag + ".cpm";

        //Detect the operating system
        struct utsname buffer;
        if (uname(&buffer) == 0) {
            operating_system += buffer.sysname;
            operating_system += " ";
            operating_system += buffer.machine;
            operating_system += " ";
            operating_system += buffer.release;
        } else {
            std::cout << "Warning: Failed to detect operating system" << std::endl;
            operating_system = "unknown";
        }

        //Store the time
        start_time = wall_clock::now();
    }

    void begin(){
        if(standard_report){
            std::cout << "Start CPM benchmarks" << std::endl;
            if(!folder_ok){
                std::cout << "   Impossible to save the results (invalid folder)" << std::endl;
            } else if(auto_save){
                std::cout << "   Results will be automatically saved in " << final_file << std::endl;
            } else {
                std::cout << "   Results will be saved on-demand in " << final_file << std::endl;
            }
            std::cout << "   Each test is warmed-up " << warmup << " times" << std::endl;
            std::cout << "   Each test is repeated " << repeat << " times" << std::endl;

            auto time = wall_clock::to_time_t(start_time);
            std::cout << "   Time " << std::ctime(&time) << std::endl;

            std::cout << "   Compiler " << COMPILER_FULL << std::endl;
            std::cout << "   Operating System " << operating_system << std::endl;
            std::cout << std::endl;
        }
    }

    ~benchmark(){
        end(auto_save);
    }

    void end(bool save_file = true){
        if(standard_report){
            std::cout << std::endl;
            std::cout << "End of CPM benchmarks" << std::endl;
            std::cout << "   "  << tests << " tests have been run" << std::endl;
            std::cout << "   "  << measures << " measures have been taken" << std::endl;
            std::cout << "   "  << runs << " functors calls" << std::endl;
            std::cout << std::endl;
        }

        if(save_file){
            save();
        }
    }

    template<typename Policy = DefaultPolicy>
    section<benchmark<DefaultPolicy>, Policy> multi(std::string name){
        bool rename = false;
        bool found = false;
        std::size_t id = 0;
        while(true){
            for(auto& section : section_results){
                if(section.name == name){
                    name += "_" + std::to_string(id++);
                    rename = true;
                    found = true;
                    break;
                }
            }

            if(found){
                found = false;
            } else {
                break;
            }
        }

        if(rename){
            std::cout << "Warning: Section already exists. Renamed in \"" << name << "\"\n";
        }

        return {std::move(name), *this};
    }

    //Measure once functor (no policy, no randomization)

    template<typename Functor>
    void measure_once(const std::string& title, Functor functor){
        measure_data data;
        data.title = title;

        auto duration = measure_only_simple(*this, functor);
        report(title, std::size_t(1), duration);
        data.results.push_back(std::make_pair(std::string("1"), duration));

        results.push_back(std::move(data));
    }

    //Measure simple functor (no randomization)

    template<typename Policy = DefaultPolicy, typename Functor>
    void measure_simple(const std::string& title, Functor functor){
        if(standard_report){
            std::cout << std::endl;
        }

        measure_data data;
        data.title = title;

        policy_run<Policy>(
            [&data, &title, &functor, this](auto sizes){
                auto duration = measure_only_simple(*this, functor, sizes);
                report(title, sizes, duration);
                data.results.push_back(std::make_pair(size_to_string(sizes), duration));
                return duration;
            }
        );

        results.push_back(std::move(data));
    }

    //Measure with two-pass functors (init and functor)

    template<bool Sizes = true, typename Policy= DefaultPolicy, typename Init, typename Functor>
    void measure_two_pass(const std::string& title, Init init, Functor functor){
        if(standard_report){
            std::cout << std::endl;
        }

        measure_data data;
        data.title = title;

        policy_run<Policy>(
            [&data, &title, &functor, &init, this](auto sizes){
                auto duration = measure_only_two_pass<Sizes>(*this, init, functor, sizes);
                report(title, sizes, duration);
                data.results.push_back(std::make_pair(size_to_string(sizes), duration));
                return duration;
            }
        );

        results.push_back(std::move(data));
    }

    //measure a function with global references

    template<typename Policy = DefaultPolicy, typename Functor, typename... T>
    void measure_global(const std::string& title, Functor functor, T&... references){
        if(standard_report){
            std::cout << std::endl;
        }

        measure_data data;
        data.title = title;

        policy_run<Policy>(
            [&data, &title, &functor, &references..., this](auto sizes){
                auto duration = measure_only_global(*this, functor, sizes, references...);
                report(title, sizes, duration);
                data.results.push_back(std::make_pair(size_to_string(sizes), duration));
                return duration;
            }
        );

        results.push_back(std::move(data));
    }

    //Measure and return the duration of a simple functor

    template<typename Functor>
    static std::size_t measure_only(Functor functor){
        auto start_time = timer_clock::now();
        functor();
        auto end_time = timer_clock::now();
        auto duration = std::chrono::duration_cast<microseconds>(end_time - start_time);

        return duration.count();
    }

private:
    void save(){
        auto time = wall_clock::to_time_t(start_time);
        std::stringstream ss;
        ss << std::ctime(&time);
        auto time_str = ss.str();

        if(time_str.back() == '\n'){
            time_str.pop_back();
        }

        std::ofstream stream(final_file);

        stream << "{\n";

        std::size_t indent = 2;

        write_value(stream, indent, "name", name);
        write_value(stream, indent, "tag", tag);
        write_value(stream, indent, "compiler", COMPILER_FULL);
        write_value(stream, indent, "os", operating_system);

        write_value(stream, indent, "time", time_str);
        write_value(stream, indent, "timestamp", std::chrono::duration_cast<seconds>(start_time.time_since_epoch()).count());

        start_array(stream, indent, "results");

        for(std::size_t i = 0; i < results.size(); ++i){
            auto& result = results[i];

            start_sub(stream, indent);

            write_value(stream, indent, "title", result.title);
            start_array(stream, indent, "results");

            for(std::size_t j = 0; j < result.results.size(); ++j){
                auto& sub = result.results[j];

                start_sub(stream, indent);

                write_value(stream, indent, "size", sub.first);
                write_value(stream, indent, "duration", sub.second, false);

                close_sub(stream, indent, j < result.results.size() - 1);
            }

            close_array(stream, indent, false);
            close_sub(stream, indent, i < results.size() - 1);
        }

        close_array(stream, indent, true);

        start_array(stream, indent, "sections");

        for(std::size_t i = 0; i < section_results.size(); ++i){
            auto& section = section_results[i];

            start_sub(stream, indent);

            write_value(stream, indent, "name", section.name);
            start_array(stream, indent, "results");

            for(std::size_t j = 0; j < section.names.size(); ++j){
                auto& name = section.names[j];

                start_sub(stream, indent);

                write_value(stream, indent, "name", name);
                start_array(stream, indent, "results");

                for(std::size_t k = 0; k < section.results[j].size(); ++k){
                    start_sub(stream, indent);

                    write_value(stream, indent, "size", section.sizes[k]);
                    write_value(stream, indent, "duration", section.results[j][k], false);

                    close_sub(stream, indent, k < section.results[j].size() - 1);
                }

                close_array(stream, indent, false);
                close_sub(stream, indent, j < section.names.size() - 1);
            }

            close_array(stream, indent, false);
            close_sub(stream, indent, i < section_results.size() - 1);
        }

        close_array(stream, indent, false);

        stream << "}";
    }

    template<typename Policy, typename M>
    void policy_run(M measure){
        ++tests;

        auto d = Policy::begin();
        auto duration = measure(d);

        while(Policy::has_next(d, duration)){
            d = Policy::next(d);

            duration = measure(d);
        }
    }

    template<typename Config, typename Functor, typename... Args>
    std::size_t measure_only_simple(const Config& conf, Functor& functor, Args... args){
        ++measures;

        for(std::size_t i = 0; i < conf.warmup; ++i){
            call_functor(functor, args...);
        }

        std::size_t duration_acc = 0;

        for(std::size_t i = 0; i < conf.repeat; ++i){
            auto start_time = timer_clock::now();
            call_functor(functor, args...);
            auto end_time = timer_clock::now();
            auto duration = std::chrono::duration_cast<microseconds>(end_time - start_time);
            duration_acc += duration.count();
        }

        runs += conf.warmup + conf.repeat;

        return duration_acc / conf.repeat;
    }

    template<bool Sizes, typename Config, typename Init, typename Functor, typename... Args>
    std::size_t measure_only_two_pass(const Config& conf, Init& init, Functor& functor, Args... args){
        ++measures;

        auto data = call_init_functor(init, args...);

        static constexpr const std::size_t tuple_s = std::tuple_size<decltype(data)>::value;
        std::make_index_sequence<tuple_s> sequence;

        for(std::size_t i = 0; i < conf.warmup; ++i){
            randomize_each(data, sequence);
            call_with_data<Sizes>(data, functor, sequence, args...);
        }

        std::size_t duration_acc = 0;

        for(std::size_t i = 0; i < conf.repeat; ++i){
            randomize_each(data, sequence);
            auto start_time = timer_clock::now();
            call_with_data<Sizes>(data, functor, sequence, args...);
            auto end_time = timer_clock::now();
            auto duration = std::chrono::duration_cast<microseconds>(end_time - start_time);
            duration_acc += duration.count();
        }

        runs += conf.warmup + conf.repeat;

        return duration_acc / conf.repeat;
    }

    template<typename Config, typename Functor, typename Tuple, typename... T>
    std::size_t measure_only_global(const Config& conf, Functor& functor, Tuple d, T&... references){
        ++measures;

        for(std::size_t i = 0; i < conf.warmup; ++i){
            using cpm::randomize;
            randomize(references...);
            functor(d);
        }

        std::size_t duration_acc = 0;

        for(std::size_t i = 0; i < conf.repeat; ++i){
            using cpm::randomize;
            randomize(references...);
            auto start_time = timer_clock::now();
            functor(d);
            auto end_time = timer_clock::now();
            auto duration = std::chrono::duration_cast<microseconds>(end_time - start_time);
            duration_acc += duration.count();
        }

        runs += conf.warmup + conf.repeat;

        return duration_acc / conf.repeat;
    }

    template<typename Tuple>
    void report(const std::string& title, Tuple d, std::size_t duration){
        if(standard_report){
            std::cout << title << "(" << size_to_string(d) << ") took " << us_duration_str(duration) << "\n";
        }
    }
};

} //end of namespace cpm

#ifdef CPM_BENCHMARK
#include "cpm_support.hpp"
#endif

#endif //CPM_CPM_HPP
