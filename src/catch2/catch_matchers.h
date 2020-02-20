/*
 * Distributed under the Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef TWOBLUECUBES_CATCH_MATCHERS_HPP_INCLUDED
#define TWOBLUECUBES_CATCH_MATCHERS_HPP_INCLUDED

#include <catch2/catch_common.h>
#include <catch2/catch_capture_matchers.h>

#include <string>
#include <vector>

namespace Catch {
namespace Matchers {

    class MatcherUntypedBase {
    public:
        MatcherUntypedBase() = default;
        MatcherUntypedBase(MatcherUntypedBase const&) = default;
        MatcherUntypedBase& operator = (MatcherUntypedBase const&) = delete;
        std::string toString() const;

    protected:
        virtual ~MatcherUntypedBase(); // = default;
        virtual std::string describe() const = 0;
        mutable std::string m_cachedToString;
    };

#ifdef __clang__
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif

    template<typename ObjectT>
    struct MatcherMethod {
        virtual bool match(ObjectT const& arg) const = 0;
    };

#if defined(__OBJC__)
    // Hack to fix Catch GH issue #1661. Could use id for generic Object support.
    // use of const for Object pointers is very uncommon and under ARC it causes some kind of signature mismatch that breaks compilation
    template<>
    struct MatcherMethod<NSString*> {
        virtual bool match(NSString* arg) const = 0;
    };
#endif

#ifdef __clang__
#    pragma clang diagnostic pop
#endif

    template<typename T>
    struct MatcherBase : MatcherUntypedBase, MatcherMethod<T> {};

    namespace Detail {

        template<typename ArgT>
        struct MatchAllOf final : MatcherBase<ArgT> {
            MatchAllOf() = default;
            MatchAllOf(MatchAllOf const&) = delete;
            MatchAllOf& operator=(MatchAllOf const&) = delete;
            MatchAllOf(MatchAllOf&&) = default;
            MatchAllOf& operator=(MatchAllOf&&) = default;


            bool match( ArgT const& arg ) const override {
                for( auto matcher : m_matchers ) {
                    if (!matcher->match(arg))
                        return false;
                }
                return true;
            }
            std::string describe() const override {
                std::string description;
                description.reserve( 4 + m_matchers.size()*32 );
                description += "( ";
                bool first = true;
                for( auto matcher : m_matchers ) {
                    if( first )
                        first = false;
                    else
                        description += " and ";
                    description += matcher->toString();
                }
                description += " )";
                return description;
            }

            friend MatchAllOf operator&& (MatchAllOf&& lhs, MatcherBase<ArgT> const& rhs) {
                lhs.m_matchers.push_back(&rhs);
                return std::move(lhs);
            }
            friend MatchAllOf operator&& (MatcherBase<ArgT> const& lhs, MatchAllOf&& rhs) {
                rhs.m_matchers.insert(rhs.m_matchers.begin(), &lhs);
                return std::move(rhs);
            }

        private:
            std::vector<MatcherBase<ArgT> const*> m_matchers;
        };

        //! lvalue overload is intentionally deleted, users should
        //! not be trying to compose stored composition matchers
        template<typename ArgT>
        MatchAllOf<ArgT> operator&& (MatchAllOf<ArgT> const& lhs, MatcherBase<ArgT> const& rhs) = delete;
        //! lvalue overload is intentionally deleted, users should
        //! not be trying to compose stored composition matchers
        template<typename ArgT>
        MatchAllOf<ArgT> operator&& (MatcherBase<ArgT> const& lhs, MatchAllOf<ArgT> const& rhs) = delete;

        template<typename ArgT>
        struct MatchAnyOf final : MatcherBase<ArgT> {
            MatchAnyOf() = default;
            MatchAnyOf(MatchAnyOf const&) = delete;
            MatchAnyOf& operator=(MatchAnyOf const&) = delete;
            MatchAnyOf(MatchAnyOf&&) = default;
            MatchAnyOf& operator=(MatchAnyOf&&) = default;

            bool match( ArgT const& arg ) const override {
                for( auto matcher : m_matchers ) {
                    if (matcher->match(arg))
                        return true;
                }
                return false;
            }
            std::string describe() const override {
                std::string description;
                description.reserve( 4 + m_matchers.size()*32 );
                description += "( ";
                bool first = true;
                for( auto matcher : m_matchers ) {
                    if( first )
                        first = false;
                    else
                        description += " or ";
                    description += matcher->toString();
                }
                description += " )";
                return description;
            }

            friend MatchAnyOf operator|| (MatchAnyOf&& lhs, MatcherBase<ArgT> const& rhs) {
                lhs.m_matchers.push_back(&rhs);
                return std::move(lhs);
            }
            friend MatchAnyOf operator|| (MatcherBase<ArgT> const& lhs, MatchAnyOf&& rhs) {
                rhs.m_matchers.insert(rhs.m_matchers.begin(), &lhs);
                return std::move(rhs);
            }

        private:
            std::vector<MatcherBase<ArgT> const*> m_matchers;
        };

        //! lvalue overload is intentionally deleted, users should
        //! not be trying to compose stored composition matchers
        template<typename ArgT>
        MatchAnyOf<ArgT> operator|| (MatchAnyOf<ArgT> const& lhs, MatcherBase<ArgT> const& rhs) = delete;
        //! lvalue overload is intentionally deleted, users should
        //! not be trying to compose stored composition matchers
        template<typename ArgT>
        MatchAnyOf<ArgT> operator|| (MatcherBase<ArgT> const& lhs, MatchAnyOf<ArgT> const& rhs) = delete;

        template<typename ArgT>
        struct MatchNotOf final : MatcherBase<ArgT> {

            explicit MatchNotOf( MatcherBase<ArgT> const& underlyingMatcher ):
                m_underlyingMatcher( underlyingMatcher )
            {}

            bool match( ArgT const& arg ) const override {
                return !m_underlyingMatcher.match( arg );
            }

            std::string describe() const override {
                return "not " + m_underlyingMatcher.toString();
            }

        private:
            MatcherBase<ArgT> const& m_underlyingMatcher;
        };

    } // namespace Detail

    template <typename T>
    Detail::MatchAllOf<T> operator&& (MatcherBase<T> const& lhs, MatcherBase<T> const& rhs) {
        return Detail::MatchAllOf<T>{} && lhs && rhs;
    }
    template <typename T>
    Detail::MatchAnyOf<T> operator|| (MatcherBase<T> const& lhs, MatcherBase<T> const& rhs) {
        return Detail::MatchAnyOf<T>{} || lhs || rhs;
    }

    template <typename T>
    Detail::MatchNotOf<T> operator! (MatcherBase<T> const& matcher) {
        return Detail::MatchNotOf<T>{ matcher };
    }


} // namespace Matchers
} // namespace Catch


#if defined(CATCH_CONFIG_PREFIX_ALL) && !defined(CATCH_CONFIG_DISABLE)
  #define CATCH_REQUIRE_THROWS_WITH( expr, matcher ) INTERNAL_CATCH_THROWS_STR_MATCHES( "CATCH_REQUIRE_THROWS_WITH", Catch::ResultDisposition::Normal, matcher, expr )
  #define CATCH_REQUIRE_THROWS_MATCHES( expr, exceptionType, matcher ) INTERNAL_CATCH_THROWS_MATCHES( "CATCH_REQUIRE_THROWS_MATCHES", exceptionType, Catch::ResultDisposition::Normal, matcher, expr )

  #define CATCH_CHECK_THROWS_WITH( expr, matcher ) INTERNAL_CATCH_THROWS_STR_MATCHES( "CATCH_CHECK_THROWS_WITH", Catch::ResultDisposition::ContinueOnFailure, matcher, expr )
  #define CATCH_CHECK_THROWS_MATCHES( expr, exceptionType, matcher ) INTERNAL_CATCH_THROWS_MATCHES( "CATCH_CHECK_THROWS_MATCHES", exceptionType, Catch::ResultDisposition::ContinueOnFailure, matcher, expr )

  #define CATCH_CHECK_THAT( arg, matcher ) INTERNAL_CHECK_THAT( "CATCH_CHECK_THAT", matcher, Catch::ResultDisposition::ContinueOnFailure, arg )
  #define CATCH_REQUIRE_THAT( arg, matcher ) INTERNAL_CHECK_THAT( "CATCH_REQUIRE_THAT", matcher, Catch::ResultDisposition::Normal, arg )

#elif defined(CATCH_CONFIG_PREFIX_ALL) && defined(CATCH_CONFIG_DISABLE)

  #define CATCH_REQUIRE_THROWS_WITH( expr, matcher )                   (void)(0)
  #define CATCH_REQUIRE_THROWS_MATCHES( expr, exceptionType, matcher ) (void)(0)

  #define CATCH_CHECK_THROWS_WITH( expr, matcher )                     (void)(0)
  #define CATCH_CHECK_THROWS_MATCHES( expr, exceptionType, matcher )   (void)(0)

  #define CATCH_CHECK_THAT( arg, matcher )                             (void)(0)
  #define CATCH_REQUIRE_THAT( arg, matcher )                           (void)(0)

#elif !defined(CATCH_CONFIG_PREFIX_ALL) && !defined(CATCH_CONFIG_DISABLE)

  #define REQUIRE_THROWS_WITH( expr, matcher ) INTERNAL_CATCH_THROWS_STR_MATCHES( "REQUIRE_THROWS_WITH", Catch::ResultDisposition::Normal, matcher, expr )
  #define REQUIRE_THROWS_MATCHES( expr, exceptionType, matcher ) INTERNAL_CATCH_THROWS_MATCHES( "REQUIRE_THROWS_MATCHES", exceptionType, Catch::ResultDisposition::Normal, matcher, expr )

  #define CHECK_THROWS_WITH( expr, matcher ) INTERNAL_CATCH_THROWS_STR_MATCHES( "CHECK_THROWS_WITH", Catch::ResultDisposition::ContinueOnFailure, matcher, expr )
  #define CHECK_THROWS_MATCHES( expr, exceptionType, matcher ) INTERNAL_CATCH_THROWS_MATCHES( "CHECK_THROWS_MATCHES", exceptionType, Catch::ResultDisposition::ContinueOnFailure, matcher, expr )

  #define CHECK_THAT( arg, matcher ) INTERNAL_CHECK_THAT( "CHECK_THAT", matcher, Catch::ResultDisposition::ContinueOnFailure, arg )
  #define REQUIRE_THAT( arg, matcher ) INTERNAL_CHECK_THAT( "REQUIRE_THAT", matcher, Catch::ResultDisposition::Normal, arg )

#elif !defined(CATCH_CONFIG_PREFIX_ALL) && defined(CATCH_CONFIG_DISABLE)

  #define REQUIRE_THROWS_WITH( expr, matcher )                   (void)(0)
  #define REQUIRE_THROWS_MATCHES( expr, exceptionType, matcher ) (void)(0)

  #define CHECK_THROWS_WITH( expr, matcher )                     (void)(0)
  #define CHECK_THROWS_MATCHES( expr, exceptionType, matcher )   (void)(0)

  #define CHECK_THAT( arg, matcher )                             (void)(0)
  #define REQUIRE_THAT( arg, matcher )                           (void)(0)

#endif // end of user facing macro declarations

#endif // TWOBLUECUBES_CATCH_MATCHERS_HPP_INCLUDED
