// https://stackoverflow.com/a/23232211/390557 was very helpful

#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <tuple>

// seems little point wrapping this if I need a different header
// for arch linux (histedit.h) as opposed to other linux
#include <editline/readline.h>
#include <histedit.h>

// rational number support
#include <boost/rational.hpp>

// spirit headers
#define BOOST_SPIRIT_USE_PHOENIX_V3 1
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
enum Operation { Add, Subtract, Divide, Multiply, Modulo, Exponent, Min, Max};

template <typename T>
T initial_value(Operation op)
{
    switch (op)
    {
    case Add:
        return T{0};
    
    case Subtract:
        return T{0};

    case Divide:
        return T{1};

    case Multiply:
        return T{1};

    case Modulo:
        return T{1};

    case Exponent:
        return T{1};

    case Min:
        return T{0};

    case Max:
        return T{0};
    }
    return T{};  // shouldn't happen!
}

template <typename T> struct Expression;
template <typename T>
using Operand = boost::variant<T, boost::recursive_wrapper<Expression<T>>>;

template <typename T>
struct Expression
{
    Operation op;
    std::vector<Operand<T>> operands;
};

// modulo operator for boost::rational
template <typename T>
boost::rational<T> operator%(const boost::rational<T>& lhs, const boost::rational<T>& rhs) {
    auto c = lhs / rhs;
    return c.numerator() % c.denominator();
}

// pow for boost::rational
template <typename T>
boost::rational<T> pow(const boost::rational<T>& base, const boost::rational<T>& exp) {
    // TODO - it's the wrong return type
    return boost::rational<T>(
        static_cast<T>(
            std::pow(boost::rational_cast<double>(base), boost::rational_cast<double>(exp))
        )
    ); 
}

// based off http://coliru.stacked-crooked.com/a/92b61075295538e0
struct Print : boost::static_visitor<std::ostream&>
{
    Print(std::ostream& os) : os(os) {}
    std::ostream& os;

    template <typename T>
    std::ostream& operator()(T t) const
    {
        return os << t << ' ';
    }

    template <typename T>
    std::ostream& operator()(boost::rational<T> r) const
    {
        return r.denominator() == 1
            ? os << r.numerator() << ' '
            : os << r << ' ';
    }

    template <typename T>
    std::ostream& operator()(const Expression<T>& e) const
    {
        auto opString = [this](Operation o)->std::ostream&
        {
            switch (o)
            {
                case Add: return os << "Add:";
                case Subtract: return os << "Subtract:";
                case Divide: return os << "Divide:";
                case Multiply: return os << "Multiply:";
                case Modulo: return os << "Modulo:";
                case Exponent: return os << "Exponent:";
                case Min: return os << "Min:";
                case Max: return os << "Max:";
            }
            // shouldn't happen!
            return os;
        };

        opString(e.op) << "(";
        for (const auto& arg : e.operands)
        {
            Print printer(os);
            printer(arg);
        }
        return os << ")";
    }

    template <typename... TVariant>
    std::ostream& operator()(boost::variant<TVariant...> const& v) const { 
        return boost::apply_visitor(*this, v); 
    }
};

template <typename T>
struct Evaluate : boost::static_visitor<T>
{
    T operator()(T i) const { return i; }

    T operator()(const Expression<T>& e) const
    {
        std::vector<T> operands;
        for (const auto& arg : e.operands)
        {
            Evaluate evaluator;
            operands.push_back(evaluator(arg));
        }

        auto operation = [](Operation o)->std::function<T(T,T)>
        {
            switch (o)
            {
                case Add: return [](T a, T b){ return a + b; };
                case Subtract: return [](T a, T b){ return a - b; };
                case Divide: return [](T a, T b)
                { 
                    if (b == 0) throw std::overflow_error("divide by zero");
                    return a / b;
                };
                case Multiply: return [](T a, T b){ return a * b; };
                case Modulo: return [](T a, T b)
                { 
                    if (b == 0) throw std::overflow_error("divide by zero");
                    return a % b;
                };
                case Exponent: return [](T a, T b)
                { 
                    // TODO - there's no error checking here.  see http://en.cppreference.com/w/cpp/numeric/math/pow for what can go wrong
                    return pow(a, b);
                };
                case Min: return [](T a, T b){ return std::min(a,b); };
                case Max: return [](T a, T b){ return std::max(a,b); };
            }
            return nullptr;  // shouldn't happen!
        }(e.op); 

        // find the initial value for std::accumulate.  0 for +/-
        // 1 for * and /.
        decltype(std::begin(operands)) begin;
        T initial;
        std::tie(begin, initial) = [&operands, e](){
            return operands.size() > 1
                ? std::make_tuple(std::next(std::begin(operands)), *std::begin(operands))
                : std::make_tuple(std::begin(operands), initial_value<T>(e.op));
        }();

        return std::accumulate(begin
            , std::end(operands)
            , initial
            , operation);
    }

    template <typename... TVariant>
    T operator()(boost::variant<TVariant...> const& v) const { 
        return boost::apply_visitor(*this, v); 
    }
};



} // namespace
BOOST_FUSION_ADAPT_TPL_STRUCT(
    (T),
    (Expression) (T),
    (Operation, op)
    (std::vector<Operand<T>>, operands)
);

namespace {

template <typename Iterator, typename T>
struct polish : qi::grammar<Iterator, Expression<T>(), qi::space_type>
{
    polish() : polish::base_type(expression_)
    {
        using qi::lit;
        using qi::int_;

        operator_.add
            ("+", Add)
            ("-", Subtract)
            ("/", Divide)
            ("*", Multiply)
            ("%", Modulo)
            ("^", Exponent)
            ("min", Min)
            ("max", Max);
        expression_ = operator_ >> +operand_;
        operand_ = int_ | ( lit('(') >> expression_ >> lit(')') );
    }

    qi::symbols<char, Operation> operator_;
    qi::rule<Iterator, Operand<T>(), qi::space_type> operand_;
    qi::rule<Iterator, Expression<T>(), qi::space_type> expression_;
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

    using result_t = boost::rational<int>;
    using parser = polish<decltype(std::begin(input)), result_t>;
    parser p;
    Expression<result_t> expression;
    auto success = phrase_parse(std::begin(input)
        , std::end(input)
        , p
        , qi::space
        , expression);

    if (success)
    {
        Print printer(std::cout);
        printer(expression); 
        std::cout << std::endl;

        Evaluate<result_t> evaluator;
        try
        {
            auto result = evaluator(expression);
            std::cout << "="; 
            printer(result);
            std::cout << std::endl;
        }
        catch (std::exception& e)
        {
            std::cout << "Error:" << e.what() << std::endl;
        }
    }
    else 
    {
        std::cout << "The phrase '" << input << " is not polish" << std::endl;
    }
  }
}

