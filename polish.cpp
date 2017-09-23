#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>

// seems little point wrapping this if I need a different header
// for arch linux (histedit.h) as opposed to other linux
#include <editline/readline.h>
#include <histedit.h>

// spirit headers
#define BOOST_SPIRIT_USE_PHOENIX_V3
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>               
#include <boost/spirit/include/phoenix.hpp>
#include <boost/variant.hpp>

namespace spirit = boost::spirit;
namespace qi = spirit::qi;
namespace ascii = spirit::ascii;
namespace phoenix = boost::phoenix;

namespace
{
using operation_t = std::function<int(int, int)>;

enum Operation { Add, Subtract, Divide, Multiply };

template <typename T, Operation>
struct ApplyOp;

template <typename T>
struct ApplyOp<T, Add>
{ using op = std::plus<T>; static const T start{0}; };

template <typename T>
struct ApplyOp<T, Subtract>
{ using op = std::minus<T>; static const T start{0}; };

template <typename T>
struct ApplyOp<T, Divide>
{ using op = std::divides<T>; static const T start{1}; };

template <typename T>
struct ApplyOp<T, Multiply>
{ using op = std::multiplies<T>; static const T start{1}; };

struct operation_ : qi::symbols<char, Operation>
{
    operation_()
    {
        add
            ("+", Add)
            ("-", Subtract)
            ("*", Multiply)
            ("/", Divide)
        ;
    }
} operation;

template <typename T, Operation Op>
T apply_op_impl(T base, T value)
{
    using impl = typename ApplyOp<T, Op>::op;
    impl func;
    return func(base, value);
}

template <typename T, Operation Op>
T initial_value_impl()
{
    return ApplyOp<T, Op>::start;
}

template <typename T>
T apply_operation(T base, T value, Operation op)
{
    switch (op)
    {
    case Add:
        return apply_op_impl<T,Add>(std::move(base), std::move(value));
        return apply_op_impl<T,Subtract>(std::move(base), std::move(value));

    case Divide:
        return apply_op_impl<T,Divide>(std::move(base), std::move(value));

    case Multiply:
        return apply_op_impl<T,Multiply>(std::move(base), std::move(value));
    }
    return T{};  // shouldn't happen!
}

template <typename T>
T initial_value(Operation op)
{
    switch (op)
    {
    case Add:
        return initial_value_impl<T,Add>();
    
    case Subtract:
        return initial_value_impl<T,Subtract>();

    case Divide:
        return initial_value_impl<T,Divide>();

    case Multiply:
        return initial_value_impl<T,Multiply>();
    }
    return T{};  // shouldn't happen!
}

struct Expression;;
using Operand = boost::variant<int, boost::recursive_wrapper<Expression>>;

struct Expression
{
    Operation op;
    std::vector<Operand> operands;
};

} // namespace
BOOST_FUSION_ADAPT_STRUCT(
    Expression,
    (Operation, op)
    (std::vector<Operand>, operands)
);

namespace {

template <typename Iterator>
struct polish : qi::grammar<Iterator, Expression(), qi::space_type>
{
    polish() : polish::base_type(start)
    {
        using qi::lit;
        using qi::int_;

        operator_.add("+", Add)("-", Subtract)("/", Divide)("*", Multiply);
        expression_ = operator_ >> +operand_;
        operand_ = int_ || ( lit('(') >> expression_ >> lit(')') );
        start = expression_;
    }

    qi::symbols<char, Operation> operator_;
    qi::rule<Iterator, Operand(), qi::space_type> operand_;
    qi::rule<Iterator, Expression(), qi::space_type> expression_;
    qi::rule<Iterator, Expression(), qi::space_type> start;
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
    Expression result;
    auto success = phrase_parse(std::begin(input)
        , std::end(input)
        , p
        , qi::space
        , result);

    std::cout << "The phrase '" << input << "' "
        << (success ? "is" : "is not")
        << " polish" << std::endl;
    // std::cout << "=" << result << std::endl;
  }
}

