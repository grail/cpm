//=======================================================================
// Copyright (c) 2015 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef CPM_BOOTSTRAP_THEME_HPP
#define CPM_BOOTSTRAP_THEME_HPP

#include "cpm/io.hpp"
#include "cpm/data.hpp"

namespace cpm {

struct bootstrap_theme {
    const reports_data& data;
    cxxopts::Options& options;
    std::ostream& stream;

    bootstrap_theme(const reports_data& data, cxxopts::Options& options, std::ostream& stream) : data(data), options(options), stream(stream) {}

    void include(){
        stream << "<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js\"></script>\n";
        stream << "<link href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css\" rel=\"stylesheet\">\n";
        stream << "<link href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap-theme.min.css\" rel=\"stylesheet\">\n";
    }

    void header(){
        stream << R"=====(
            <nav id="myNavbar" class="navbar navbar-default navbar-inverse navbar-fixed-top" role="navigation">
            <div class="container-fluid">
            <div class="navbar-header">
            <button type="button" class="navbar-toggle" data-toggle="collapse" data-target="#navbarCollapse">
            <span class="sr-only">Toggle navigation</span>
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
            </button>
            <a class="navbar-brand" href="#">Results</a>
            </div>
            <div class="collapse navbar-collapse" id="navbarCollapse">
            <ul class="nav navbar-nav">
            <li class="active"><a href="index.html">Home</a></li>
            <li><a href="http://github.com/wichtounet/cpm">Generated with CPM</a></li>
            </ul>
            </div>
            </div>
            </nav>
        )=====";
    }

    void footer(){
        stream << R"=====(
            <hr>
            <div class="row">
            <div class="col-xs-12">
            <footer>
            <p>Generated with <a href="https://github.com/wichtounet/cpm">CPM</a></p>
            </footer>
            </div>
            </div>
            </div>
        )=====";
    }

    void before_information(){
        stream << "<div class=\"jumbotron\">\n";
        stream << "<div class=\"container-fluid\">\n";
    }

    void after_information(){
        stream << "</div>\n";
        stream << "</div>\n";
        stream << "<div class=\"container-fluid\">\n";
    }

    void compiler_buttons(const std::string& current_compiler){
        stream << R"=====(<div class="row">)=====";
        stream << R"=====(<div class="col-xs-12">)=====";

        stream << R"=====(<span>Select compiler: </span>)=====";

        stream << R"=====(<div class="btn-group" role="group">)=====";
        for(auto& compiler : data.compilers){
            if(compiler == current_compiler){
                stream << "<a class=\"btn btn-primary\" href=\"" << cpm::filify(compiler)  << "\">" << compiler << "</a>\n";
            } else {
                stream << "<a class=\"btn btn-default\" href=\"" << cpm::filify(compiler)  << "\">" << compiler << "</a>\n";
            }
        }
        stream << "</div>\n";

        stream << "</div>\n";
        stream << "</div>\n";
    }

    virtual void start_column(const std::string& style = ""){
        std::size_t columns = 1; //Always the first grapah

        if(data.documents.size() > 1 && !options.count("disable-time")){
            ++columns;
        }

        if(data.compilers.size() > 1 && !options.count("disable-compiler")){
            ++columns;
        }

        if(!options.count("disable-summary")){
            ++columns;
        }

        stream << "<div class=\"col-xs-" << 12 / columns << "\"" << style << ">\n";
    }

    virtual void close_column(){
        stream << "</div>\n";
    }

    void before_graph(std::size_t id){
        start_column();

        stream << "<div id=\"chart_" << id << "\" style=\"height: 400px;\"></div>\n";
    }

    void after_graph(){
        close_column();
    }

    void before_result(const std::string& title, bool sub = false){
        stream << "<div class=\"page-header\">\n";
        stream << "<h2>" << title << "</h2>\n";
        stream << "</div>\n";

        if(sub){
            stream << "<div class=\"row\" style=\"display:flex; align-items: flex-end\">\n";
        } else {
            stream << "<div class=\"row\">\n";
        }
    }

    void after_result(){
        stream << "</div>\n";
    }

    void before_sub_graphs(std::size_t id, std::vector<std::string> graphs){
        start_column("style=\"align-self: flex-start; \"");

        stream << "<div role=\"tabpanel\">\n";
        stream << "<ul class=\"nav nav-tabs\" role=\"tablist\">\n";

        std::string active = "class=\"active\"";
        std::size_t sub = 0;
        for(auto& g : graphs){
            auto sub_id = std::string("sub") + std::to_string(id) + "-" + std::to_string(sub++);
            stream << "<li " << active << " role=\"presentation\"><a href=\"#" << sub_id << "\" aria-controls=\""
                << sub_id << "\" role=\"tab\" data-toggle=\"tab\">" << g << "</a></li>\n";
            active = "";
        }

        stream << "</ul>\n";
        stream << "<div class=\"tab-content\">\n";
    }

    void after_sub_graphs(){
        stream << "</div>\n";
        stream << "</div>\n";

        close_column();
    }

    void before_sub_graph(std::size_t id, std::size_t sub){
        auto sub_id = std::string("sub") + std::to_string(id) + "-" + std::to_string(sub);
        std::string active;
        if(sub == 0){
            active = " active";
        }
        stream << "<div role=\"tabpanel\" class=\"tab-pane" << active << "\" id=\"" << sub_id << "\">\n";

        stream << "<div id=\"chart_" << id << "-" << sub << "\" style=\"height: 400px;\"></div>\n";
    }

    void after_sub_graph(){
        stream << "</div>\n";
    }

    void before_summary(){
        start_column();

        stream << "<table class=\"table\">\n";
    }

    void after_summary(){
        stream << "</table>\n";

        close_column();
    }

    void before_sub_summary(std::size_t id, std::size_t sub){
        auto sub_id = std::string("sub") + std::to_string(id) + "-" + std::to_string(sub);
        std::string active;
        if(sub == 0){
            active = " active";
        }

        stream << "<div role=\"tabpanel\" class=\"tab-pane" << active << "\" id=\"" << sub_id << "\">\n";
        stream << "<table class=\"table\">\n";
    }

    void after_sub_summary(){
        stream << "</table>\n";
        stream << "</div>\n";
    }

    void cell(const std::string& v){
        stream << "<td>" << v << "</td>\n";
    }

    void red_cell(const std::string& v){
        stream << "<td class=\"danger\">"<< v << "</td>\n";
    }

    void green_cell(const std::string& v){
        stream << "<td class=\"success\">" << v << "</td>\n";
    }

    template<typename T>
    bootstrap_theme& operator<<(const T& v){
        stream << v;
        return *this;
    }
};

} //end of namespace cpm

#endif //CPM_BOOTSTRAP_THEME_HPP
