/* SPDX-License-Identifier: Apache-2.0 */

#include "substrait/textplan/tests/ParseResultMatchers.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

#include "substrait/textplan/ParseResult.h"
#include "substrait/textplan/SymbolTable.h"
#include "substrait/textplan/SymbolTablePrinter.h"

namespace io::substrait::textplan {
namespace {

std::vector<std::string> symbolNames(
    const std::vector<std::shared_ptr<SymbolInfo>>& symbols) {
  std::vector<std::string> names;
  for (const auto& symbol : symbols) {
    names.push_back(symbol->name);
  }
  return names;
}

bool stringEqSquashingWhitespace(
    const std::string& have,
    const std::string& expected) {
  auto atHave = have.begin();
  auto atExpected = expected.begin();
  while (atHave != have.end() && atExpected != expected.end()) {
    if (isspace(*atExpected)) {
      if (!isspace(*atHave)) {
        return false;
      }
      // Have a match, consume all remaining space.
      do {
        atExpected++;
      } while (atExpected != expected.end() && isspace(*atExpected));
      do {
        atHave++;
      } while (atHave != have.end() && isspace(*atHave));
      continue;
    }
    if (*atHave != *atExpected) {
      return false;
    }
    atHave++;
    atExpected++;
  }
  // For convenience consume any trailing whitespace on both sides.
  if (atExpected != expected.end()) {
    do {
      atExpected++;
    } while (atExpected != expected.end() && isspace(*atExpected));
  }
  if (atHave != have.end()) {
    do {
      atHave++;
    } while (atHave != have.end() && isspace(*atHave));
  }
  return atHave == have.end() && atExpected == expected.end();
}

} // namespace

class ParsesOkMatcher {
 public:
  using is_gtest_matcher = void;

  static bool MatchAndExplain( // NOLINT
      const ParseResult& result,
      std::ostream* /* listener */) {
    return result.successful();
  }

  static void DescribeTo(std::ostream* os) { // NOLINT
    *os << "parses successfully";
  }

  static void DescribeNegationTo(std::ostream* os) { // NOLINT
    *os << "does not parse successfully";
  }
};

[[maybe_unused]] ::testing::Matcher<const ParseResult&> ParsesOk() { // NOLINT
  return ParsesOkMatcher();
}

class HasSymbolsMatcher {
 public:
  using is_gtest_matcher = void;

  explicit HasSymbolsMatcher(std::vector<std::string> expectedSymbols)
      : expectedSymbols_(std::move(expectedSymbols)) {}

  bool MatchAndExplain( // NOLINT
      const ParseResult& result,
      std::ostream* listener) const {
    auto actualSymbols = symbolNames(result.getSymbolTable().getSymbols());
    if (listener != nullptr) {
      std::vector<std::string> extraSymbols(actualSymbols.size());
      auto end = std::set_difference(
          actualSymbols.begin(),
          actualSymbols.end(),
          expectedSymbols_.begin(),
          expectedSymbols_.end(),
          extraSymbols.begin());
      extraSymbols.resize(end - extraSymbols.begin());
      if (!extraSymbols.empty()) {
        *listener << std::endl << "          with missing symbols: ";
        for (const auto& symbol : extraSymbols) {
          *listener << " \"" << symbol << "\"";
        }
      }

      std::vector<std::string> missingSymbols(expectedSymbols_.size());
      end = std::set_difference(
          expectedSymbols_.begin(),
          expectedSymbols_.end(),
          actualSymbols.begin(),
          actualSymbols.end(),
          missingSymbols.begin());
      missingSymbols.resize(end - missingSymbols.begin());
      if (!missingSymbols.empty()) {
        if (!extraSymbols.empty()) {
          *listener << ", and extra symbols: ";
        } else {
          *listener << " with extra symbols: ";
        }
        for (const auto& symbol : missingSymbols) {
          *listener << " \"" << symbol << "\"";
        }
      }
    }
    return actualSymbols == expectedSymbols_;
  }

  void DescribeTo(std::ostream* os) const { // NOLINT
    *os << "has exactly these symbols: "
        << ::testing::PrintToString(expectedSymbols_);
  }

  void DescribeNegationTo(std::ostream* os) const { // NOLINT
    *os << "does not have exactly these symbols: "
        << ::testing::PrintToString(expectedSymbols_);
  }

 private:
  const std::vector<std::string> expectedSymbols_;
};

::testing::Matcher<const ParseResult&> HasSymbols( // NOLINT
    std::vector<std::string> expectedSymbols) {
  return HasSymbolsMatcher(std::move(expectedSymbols));
}

class WhenSerializedMatcher {
 public:
  using is_gtest_matcher = void;

  explicit WhenSerializedMatcher(
      ::testing::Matcher<const std::string&> stringMatcher)
      : stringMatcher_(std::move(stringMatcher)) {}

  bool MatchAndExplain( // NOLINT
      const ParseResult& result,
      ::testing::MatchResultListener* listener) const {
    std::string outputText =
        SymbolTablePrinter::outputToText(result.getSymbolTable());
    return MatchPrintAndExplain(outputText, stringMatcher_, listener);
  }

  void DescribeTo(::std::ostream* os) const { // NOLINT
    *os << "matches after serializing ";
    stringMatcher_.DescribeTo(os);
  }

  void DescribeNegationTo(::std::ostream* os) const { // NOLINT
    *os << "does not match after serializing ";
    stringMatcher_.DescribeTo(os);
  }

 private:
  ::testing::Matcher<const std::string&> stringMatcher_;
};

::testing::Matcher<const ParseResult&> WhenSerialized( // NOLINT
    ::testing::Matcher<const std::string&> stringMatcher) {
  return WhenSerializedMatcher(std::move(stringMatcher));
}

class HasErrorsMatcher {
 public:
  using is_gtest_matcher = void;

  explicit HasErrorsMatcher(std::vector<std::string> expectedErrors)
      : expectedErrors_(std::move(expectedErrors)) {}

  bool MatchAndExplain( // NOLINT
      const ParseResult& result,
      std::ostream* /* listener */) const {
    return result.getAllErrors() == expectedErrors_;
  }

  void DescribeTo(std::ostream* os) const { // NOLINT
    *os << "has exactly these symbols: "
        << ::testing::PrintToString(expectedErrors_);
  }

  void DescribeNegationTo(std::ostream* os) const { // NOLINT
    *os << "does not have exactly these symbols: "
        << ::testing::PrintToString(expectedErrors_);
  }

 private:
  const std::vector<std::string> expectedErrors_;
};

::testing::Matcher<const ParseResult&> HasErrors( // NOLINT
    std::vector<std::string> expectedErrors) {
  return HasErrorsMatcher(std::move(expectedErrors));
}

class EqSquashingWhitespaceMatcher {
 public:
  using is_gtest_matcher = void;

  explicit EqSquashingWhitespaceMatcher(std::string expectedString)
      : expectedString_(std::move(expectedString)) {}

  bool MatchAndExplain( // NOLINT
      const std::string& str,
      std::ostream* /* listener */) const {
    return stringEqSquashingWhitespace(str, expectedString_);
  }

  void DescribeTo(std::ostream* os) const { // NOLINT
    *os << "equals squashing whitespace "
        << ::testing::PrintToString(expectedString_);
  }

  void DescribeNegationTo(std::ostream* os) const { // NOLINT
    *os << "does not equal squashing whitespace "
        << ::testing::PrintToString(expectedString_);
  }

 private:
  std::string expectedString_;
};

::testing::Matcher<const std::string&> EqSquashingWhitespace( // NOLINT
    std::string expectedString) {
  return EqSquashingWhitespaceMatcher(std::move(expectedString));
}

} // namespace io::substrait::textplan
