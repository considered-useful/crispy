#include <iostream>
#include <memory>
#include <string>

// seems little point wrapping this if I need a different header
// for arch linux (histedit.h) as opposed to other linux
#include <editline/readline.h>
#include <histedit.h>

// spirit headers
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>               
#include <boost/spirit/include/phoenix_operator.hpp>

namespace spirit = boost::spirit;
namespace qi = spirit::qi;
namespace ascii = spirit::ascii;

namespace
{

template <typename Iterator>
struct doge : qi::grammar<Iterator>
{
    doge() : doge::base_type(start)
    {
        using qi::lit;
        adjective = lit("wow") || lit("many") || lit("so") || lit("such");
        noun = lit("lisp") || lit("language") || lit("book") 
            || lit("build") || lit("c");

        // couldn't figure out the skip parser on a rule
        phrase = adjective >> *ascii::space >> noun;
        start = +phrase;
    }
    
    qi::rule<Iterator> noun;
    qi::rule<Iterator> adjective;
    qi::rule<Iterator> phrase;
    qi::rule<Iterator> start;
};

std::string _readline(const char* prompt)
{
    auto input = readline(prompt);
    auto rv = std::string(input);
    free(input);
    return rv;
}

} // namespace

int main(int argc, char** argv) {
   
  std::cout << "Crispy Version 0.0.0.0.1" << std::endl;
  std::cout << "Press Ctrl+c to Exit" << std::endl;
  std::cout << std::endl;
   
  while (1) {
    auto input = _readline("crispy>");
    add_history(input.c_str());
    std::cout << "No you're a " << input << std::endl;

    using doge_parser = doge<decltype(std::begin(input))>;
    doge_parser parser;
    auto result = parse(std::begin(input)
        , std::end(input)
        , parser);

    std::cout << "doge?:" << result << '\n';
  }
}

