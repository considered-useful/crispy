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
struct polish : qi::grammar<Iterator, qi::space_type>
{
    polish() : polish::base_type(start)
    {
        using qi::lit;
        using qi::int_;
        using qi::char_;

        op = lit('+') || lit('-') || lit('/') || lit('*'); 
        // didn't work
        // op = char_("+-*/");
        expr = int_ || (
            lit('(') >> op >> +int_ >> lit(')')
        );

        start = op >> +expr;
    }

    using rule_t = qi::rule<Iterator, qi::space_type>;
    
    qi::rule<Iterator> op;
    rule_t expr;
    rule_t start;
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
   
  std::cout << "Crispy Version 0.0.0.0.2" << std::endl;
  std::cout << "Press Ctrl+c to Exit" << std::endl;
  std::cout << std::endl;
   
  while (1) {
    auto input = _readline("crispy>");
    add_history(input.c_str());

    using parser = polish<decltype(std::begin(input))>;
    parser p;
    auto result = phrase_parse(std::begin(input)
        , std::end(input)
        , p
        , qi::space);

    std::cout << "The phrase '" << input << "' "
        << (result ? "is" : "is not")
        << " polish" << std::endl;
  }
}

