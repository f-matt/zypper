#include <iostream>

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
//#include <boost/spirit/include/phoenix_stl.hpp>

// #include <boost/lambda/lambda.hpp>
// #include <boost/bind.hpp>

#include <zypp/base/LogTools.h>
#include <zypp/base/Functional.h>
#include <zypp/FileChecker.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using namespace zypp;

#define NCNM(CL) \
  CL( const CL & )		= delete;	\
  CL & operator=( const CL & )	= delete

#define NCBM(CL) \
  CL( const CL & )		= delete;	\
  CL & operator=( CL & )	= delete;	\
  CL( CL && )			= default;	\
  CL & operator=( CL && )	= default


template <typename Tp>
void outs( Tp && t )
{ cout << "outs: " << __PRETTY_FUNCTION__ << endl; }


namespace zypp
{
  // https://rpm.org/user_doc/query_format.html
  namespace qf
  {
    enum class TokenType
    {
      String,		///> Literal string
      Tag,		///> %{TAG:format}: A known tag (1st entry if array type)
      Array,		///> [%{=TAGA} %{TAGB}\n]:Parallel iterated arrays (equal sized or sized 1/0/=)
      Conditional,	///> %|TAG?{present}:{missing}|: Conditional; tenary if tag exists or not
    };


    struct Token
    {
      NCBM( Token );

      Token( TokenType type_r, std::string raw_r = std::string() )
      : _type	{ type_r }
      , _raw	{ std::move(raw_r) }
      {}

      const TokenType	_type;
      std::string	_raw;
    };

    inline std::ostream & operator<<( std::ostream & str, const Token & obj )
    {
      str << obj._type << " : " << obj._raw;
      return str;
    }


    struct String : public Token
    {
      String( std::string raw_r = std::string() )
      : Token	{ TokenType::String, std::move(raw_r) }
      {}
    };

    struct Tag : public Token
    {
      Tag( std::string raw_r = std::string() )
      : Token	{ TokenType::Tag, std::move(raw_r) }
      {}
    };

    struct Array : public Token
    {
      Array( std::string raw_r = std::string() )
      : Token	{ TokenType::Array, std::move(raw_r) }
      {}
    };

    struct Conditional : public Token
    {
      Conditional( std::string raw_r = std::string() )
      : Token	{ TokenType::Conditional, std::move(raw_r) }
      {}
    };


    /// Parsed queryformat.
    struct Format //: public std::vector<std::string>
    {
      typedef std::string value_type;
      typedef void* iterator;
      iterator end()
      { return nullptr; }
      iterator insert( iterator pos, const value_type & value )
      {
	cout << "Format::insert(" << value << ")" << endl;
	return nullptr;
      }

      Format(const std::string & value )
      {
	cout << "Format::Format(" << value << ")" << endl;
      }
      Format & operator=(const std::string & value )
      {
	cout << "Format::assign(" << value << ")" << endl;
	return *this;
      }

      //NCNM(Format);

      Format() {}

      Format & operator+=( const std::string & value )
      {
	cout << "Format::+ssign(" << value << ")" << endl;
	if ( value.empty() )
	  throw 13;
	return *this;
      }

      std::vector<Token> _tokens;
    };

    inline std::ostream & operator<<( std::ostream & str, const Format & obj )
    {
      //str << obj._tokens;
      return str;
    }


    namespace parser
    {
      namespace px = boost::phoenix;
      namespace qi = boost::spirit::qi;
      namespace ascii = boost::spirit::ascii;

      /// Symboltable to unescape special chars
      struct unesc_char_ : qi::symbols<char, const char *>
      {
	unesc_char_()
	{
	  add
	  ( "\\a", "\a" )
	  ( "\\b", "\b" )
	  ( "\\f", "\f" )
	  ( "\\n", "@@" )
	  ( "\\r", "\r" )
	  ( "\\t", "\t" )
	  ( "\\v", "\v" )
	  ;
	}
      } unesc_char;

      /// Queryformat grammar.
      template <typename Iterator>
      struct Grammar : qi::grammar<Iterator, Format()>
      {
	Grammar()
	: Grammar::base_type( format )
	{
	  using px::ref;
	  using qi::_val;
	  using qi::_1;

	  tok_special	%= qi::lit('%') | '[';
	  identifyer	%= +qi::alpha;


	  tok_string	%= +( unesc_char | '\\'>>qi::char_ | qi::char_-tok_start )
	  ;


	  tag_field	%= -qi::char_('-') >> +qi::digit;
	  tag_name	%= identifyer;
	  tag_format	%= identifyer;

	  tok_tag	%= qi::lit('%') >> -(tag_field) >> '{' >> tag_name >> -(':'>>tag_format) >> '}';


// 	  tok_array;
//
// 	  tok_conditional;


	  format	%= *( tok_tag | tok_array | tok_conditional | tok_string );
								      ^^^ vorne oder hinten?

	}

	qi::rule<Iterator, std::string()>	tok_special;
	qi::rule<Iterator, std::string()>	identifyer;

	qi::rule<Iterator, std::string()>	tag_field;
	qi::rule<Iterator, std::string()>	tag_name;
	qi::rule<Iterator, std::string()>	tag_format;

	qi::rule<Iterator, std::string()>	tok_string;
	qi::rule<Iterator, std::string()>	tok_tag;
	qi::rule<Iterator, std::string()>	tok_array;
	qi::rule<Iterator, std::string()>	tok_conditional;

	qi::rule<Iterator, Format()>		format;



      };
    } // namespace parser


    template <typename Iterator>
    bool parse( Iterator iter, Iterator end, Format & result_r )
    {
      parser::Grammar<Iterator> grammar;
      bool ret = parse( iter, end, grammar, result_r );

      if (ret && iter == end )
      {
	std::cout << "-------------------------\n";
	std::cout << "Parsing succeeded\n";
// 	std::cout << "result = " << result_r << std::endl;
	std::cout << "-------------------------\n";
      }
      else
      {
	std::string rest(iter, end);
	std::cout << "-------------------------\n";
	std::cout << "Parsing failed\n";
	std::cout << "stopped at: \"" << rest << "\"\n";
	std::cout << "-------------------------\n";
      }
      return ret;
    }

    bool parse( const std::string & qf_r, Format & result_r )
    {
      cout << ">>>>>" << qf_r << "<<<<<" << endl;
      return parse( qf_r.begin(), qf_r.end(), result_r );
    }
  } // qf
} // namespace zypp

#if 0
{
using boost::spirit::qi::int_;
using boost::spirit::qi::parse;
using client::print;
using client::writer;
using client::print_action;

{ // example using plain function

char const *first = "{42}", *last = first + std::strlen(first);
//[tutorial_attach_actions1
parse(first, last, '{' >> int_[&print] >> '}');
//]
}

{ // example using simple function object

char const *first = "{43}", *last = first + std::strlen(first);
//[tutorial_attach_actions2
parse(first, last, '{' >> int_[print_action()] >> '}');
//]
}

{ // example using boost.bind with a plain function

char const *first = "{44}", *last = first + std::strlen(first);
//[tutorial_attach_actions3
parse(first, last, '{' >> int_[boost::bind(&print, _1)] >> '}');
//]
}

{ // example using boost.bind with a member function

char const *first = "{44}", *last = first + std::strlen(first);
//[tutorial_attach_actions4
writer w;
parse(first, last, '{' >> int_[boost::bind(&writer::print, &w, _1)] >> '}');
//]
}

{ // example using boost.lambda

namespace lambda = boost::lambda;
char const *first = "{45}", *last = first + std::strlen(first);
using lambda::_1;
//[tutorial_attach_actions5
parse(first, last, '{' >> int_[std::cout << _1 << '\n'] >> '}');
//]
}

#endif
int main( int argc, const char ** argv )
{
  ++argv,--argc;

  if ( argc )
  {
    qf::Format result;
    qf::parse( *argv, result );
  }
  else
  {
    for ( const char * qf : {
      //     "", "\\n", "\%", "\\x",
      //     "%{NAME} %{SIZE}\\n",
      "%-30{NAME} %10{SIZE}\\n",
      //     "[%-50{FILENAMES} %10{FILESIZES}\\n]",
      //     "[%{NAME} %{FILENAMES}\\n]",
      //     "[%{=NAME} %{FILENAMES}\\n]",
      //     "%{NAME} %{INSTALLTIME:date}\\n",
      //     "[%{FILEMODES:perms} %{FILENAMES}\\n]",
      //     "[%{REQUIRENAME} %{REQUIREFLAGS:depflags} %{REQUIREVERSION}\\n]",
      "\\%\\x\\n%|SOMETAG?{present}:{missing}|",
    } )
    {
      qf::Format result;
      qf::parse( qf, result );
    }
  }
  return 0;
}


