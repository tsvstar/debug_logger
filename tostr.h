#ifndef TOSTR_H_
#define TOSTR_H_ 1

/*********************************************************************
  Purpose: Display named variables and expression
  Author: Taranenko Sergey (tsvstar@gmail.com)
  Date: 02-Aug-2017
  License: BSD. See License.txt
**********************************************************************/

#include <vector>
#include "tostr_handler.h"

/******************** MACRO TO USE ***********************************/

// Print to string list of arguments with their names
//   Example:  std::cout << TOSTR_ARGS( str_var, " abc", 3, !func(arg) ) << "\n";
//   Output:   strvar = "VALUE1" abc, 3 = 3, !func(arg) = VALUE2
#define TOSTR_ARGS(...) ::tsv::util::tostr::Printer( ::tsv::util::tostr::Printer::ENUM_PRINT_ARGS, { MACRO_TOSTR__EXPAND_EACH_VAL(__VA_ARGS__) } ).do_print( __VA_ARGS__ )

// The same as TOSTR_ARGS, but use extended representation of value (could be different for user-defined complex class printers)
#define TOSTR_ARGS_EXTENDED(...) ::tsv::util::tostr::Printer( ::tsv::util::tostr::Printer::ENUM_PRINT_ARGS_EXTENDED, { MACRO_TOSTR__EXPAND_EACH_VAL(__VA_ARGS__) } ).do_print( __VA_ARGS__ )


// Print to string converted concatenation of all given values ( implicitly converted to string using toStr() )
//   Example:  std::cout << TOSTR_JOIN( str_var, " abc", 3, !func(arg) ) << "\n";
//   Output:   VALUE1 abc3VALUE2
#define TOSTR_JOIN(...) ::tsv::util::tostr::Printer( ::tsv::util::tostr::Printer::ENUM_PRINT_STR, {} ).do_print( __VA_ARGS__ )

// Combination of TOSTR_ARGS() and TOSTR_JOIN() - use literals/arithmetical as is, but name_of_var with its value for vars/calls/..
// Use it for quickly represent simple formulas
//   Example:  std::cout << TOSTR_EXPR( res, "=", 3, "+", !func(arg) ) << "\n";
//   Output:  res{VALUE_RES} = 3 + !func(arg){RETURN_VALUE_OF_CALL}
#define TOSTR_EXPR(...) ::tsv::util::tostr::Printer( ::tsv::util::tostr::Printer::ENUM_PRINT_EXPR, { MACRO_TOSTR__EXPAND_EACH_VAL(__VA_ARGS__) } ).do_print( __VA_ARGS__ )

// The same as TOSTR_EXPR, but use extended representation of value (could be different for user-defined complex class printers)
#define TOSTR_EXPR_EXTENDED(...) ::tsv::util::tostr::Printer( ::tsv::util::tostr::Printer::ENUM_PRINT_EXPR_EXTENDED, { MACRO_TOSTR__EXPAND_EACH_VAL(__VA_ARGS__) } ).do_print( __VA_ARGS__ )

/************************* Implementation ***********************************/

namespace tsv {
namespace util{
namespace tostr{


class Printer
{
public:
    enum Mode { ENUM_PRINT_ARGS, ENUM_PRINT_ARGS_EXTENDED, ENUM_PRINT_STR, ENUM_PRINT_EXPR, ENUM_PRINT_EXPR_EXTENDED };

    std::vector<const char*> names_;    // List of variable names (used for ENUM_PRINT_ARGS, ENUM_PRINT_EXPR)
    Mode mode_;                         // Printing mode

    int index_;                         // current index of argument
    std::string acc_str_;               // string which accumulate output of do_print()

    Printer( Mode mode, std::vector<const char*> names )
     {
         mode_ = mode;
         names_ = names;
     }

    template<typename... StrTail>
     std::string do_print( StrTail&&... tail )
    {
        index_ = 0;
        acc_str_ = "";
        return print( std::forward<StrTail>( tail )... );
    }

private:
    // Final print
    std::string print()
    {
       return std::move( acc_str_ );
    }

    // Recursive parse templates
    template<typename Head, typename... Tail>
    std::string print( const Head& head, Tail&&... tail )
    {
        if ( mode_ == ENUM_PRINT_ARGS || mode_ == ENUM_PRINT_ARGS_EXTENDED )
        {
            if ( !acc_str_.empty() )
            {
                char prev = names_[index_-1][0];
                if (  prev != '"' )
                    acc_str_ += ", ";
                else
                    acc_str_ += " ";
            }
            char first = names_[index_][0];
            if (  first == '"' )
                acc_str_ += toStr( head );
            else
                acc_str_ += std::string(names_[index_]) + " = " + toStr( head, mode_==ENUM_PRINT_ARGS ? ENUM_TOSTR_REPR : ENUM_TOSTR_EXTENDED );
        }
        else if ( mode_ == ENUM_PRINT_EXPR || mode_ == ENUM_PRINT_EXPR_EXTENDED )
        {
            char first = names_[index_][0];
            if (  first == '"' || ( first >= '0' && first <= '9' ) )
                acc_str_ += toStr( head ) + " ";
            else
                acc_str_ += std::string(names_[index_]) + "{" + toStr( head,  mode_==ENUM_PRINT_EXPR ? ENUM_TOSTR_REPR : ENUM_TOSTR_EXTENDED  ) + "} ";
        }
        else if ( mode_ == ENUM_PRINT_STR )
        {
            acc_str_ += toStr( head, ENUM_TOSTR_DEFAULT );
        }

        index_++;
        return print( std::forward<Tail>( tail )... );
    }

};

} // end of namespace tostr
} // end of namespace util
} // end of namespace tsv

/****************  Auxilarly macros ******************/

#define MACRO_TOSTR__CHOOSE_NTH(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29,a30,a31, num, ...) num
#define MACRO_TOSTR__GET_ARG_NUM(...) MACRO_TOSTR__CHOOSE_NTH( __VA_ARGS__, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 09, 08, 07, 06, 05, 04, 03, 02, 01, 00 )
#define MACRO_TOSTR__EXPAND_EACH_00()
#define MACRO_TOSTR__EXPAND_EACH_01(a01) #a01
#define MACRO_TOSTR__EXPAND_EACH_02(a01,a02) #a01, #a02
#define MACRO_TOSTR__EXPAND_EACH_03(a01,a02,a03) #a01, #a02, #a03
#define MACRO_TOSTR__EXPAND_EACH_04(a01,a02,a03,a04) #a01, #a02, #a03, #a04
#define MACRO_TOSTR__EXPAND_EACH_05(a01,a02,a03,a04,a05) #a01, #a02, #a03, #a04, #a05
#define MACRO_TOSTR__EXPAND_EACH_06(a01,a02,a03,a04,a05,a06) #a01, #a02, #a03, #a04, #a05, #a06
#define MACRO_TOSTR__EXPAND_EACH_07(a01,a02,a03,a04,a05,a06,a07) #a01, #a02, #a03, #a04, #a05, #a06, #a07
#define MACRO_TOSTR__EXPAND_EACH_08(a01,a02,a03,a04,a05,a06,a07,a08) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08
#define MACRO_TOSTR__EXPAND_EACH_09(a01,a02,a03,a04,a05,a06,a07,a08,a09) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09
#define MACRO_TOSTR__EXPAND_EACH_10(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10
#define MACRO_TOSTR__EXPAND_EACH_11(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11
#define MACRO_TOSTR__EXPAND_EACH_12(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12
#define MACRO_TOSTR__EXPAND_EACH_13(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13
#define MACRO_TOSTR__EXPAND_EACH_14(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14
#define MACRO_TOSTR__EXPAND_EACH_15(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15
#define MACRO_TOSTR__EXPAND_EACH_16(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16
#define MACRO_TOSTR__EXPAND_EACH_17(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17
#define MACRO_TOSTR__EXPAND_EACH_18(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18
#define MACRO_TOSTR__EXPAND_EACH_19(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19
#define MACRO_TOSTR__EXPAND_EACH_20(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19, #a20
#define MACRO_TOSTR__EXPAND_EACH_21(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19, #a20, #a21
#define MACRO_TOSTR__EXPAND_EACH_22(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19, #a20, #a21, #a22
#define MACRO_TOSTR__EXPAND_EACH_23(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19, #a20, #a21, #a22, #a23
#define MACRO_TOSTR__EXPAND_EACH_24(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19, #a20, #a21, #a22, #a23, #a24
#define MACRO_TOSTR__EXPAND_EACH_25(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19, #a20, #a21, #a22, #a23, #a24, #a25
#define MACRO_TOSTR__EXPAND_EACH_26(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19, #a20, #a21, #a22, #a23, #a24, #a25, #a26
#define MACRO_TOSTR__EXPAND_EACH_27(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19, #a20, #a21, #a22, #a23, #a24, #a25, #a26, #a27
#define MACRO_TOSTR__EXPAND_EACH_28(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19, #a20, #a21, #a22, #a23, #a24, #a25, #a26, #a27, #a28
#define MACRO_TOSTR__EXPAND_EACH_29(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19, #a20, #a21, #a22, #a23, #a24, #a25, #a26, #a27, #a28, #a29
#define MACRO_TOSTR__EXPAND_EACH_30(a01,a02,a03,a04,a05,a06,a07,a08,a09,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29,a30) #a01, #a02, #a03, #a04, #a05, #a06, #a07, #a08, #a09, #a10, #a11, #a12, #a13, #a14, #a15, #a16, #a17, #a18, #a19, #a20, #a21, #a22, #a23, #a24, #a25, #a26, #a27, #a28, #a29, #a30
#define MACRO_TOSTR__TOKEN_PASTE( x, y ) x ## y
#define MACRO_TOSTR__TOKEN_PASTE2( x, y ) MACRO_TOSTR__TOKEN_PASTE( x, y )
#define MACRO_TOSTR__EXPAND_EACH_VAL(...) MACRO_TOSTR__TOKEN_PASTE2( MACRO_TOSTR__EXPAND_EACH_, MACRO_TOSTR__GET_ARG_NUM(__VA_ARGS__))( __VA_ARGS__ )

#endif
