// SPDX-FileCopyrightText: 2024 Johann Klähn <johann@jklaehn.de>
//
// SPDX-License-Identifier: MIT

#include <llvm/Testing/Support/Error.h>

#include <gmock/gmock.h>

// HACK: This is missing from libLLVM.so on Fedora.
inline llvm::detail::ErrorHolder llvm::detail::TakeError(llvm::Error Err) {
  std::vector<std::shared_ptr<llvm::ErrorInfoBase>> Infos;
  llvm::handleAllErrors(std::move(Err),
                        [&Infos](std::unique_ptr<ErrorInfoBase> Info) {
                          Infos.emplace_back(std::move(Info));
                        });
  return {std::move(Infos)};
}

namespace genpybind {

// HACK: Upstream ValueMatchesMono<T>::MatchAndExplain is broken...
template <typename T>
class ValueMatchesMono : public ::testing::MatcherInterface<
                             const ::llvm::detail::ExpectedHolder<T> &> {
public:
  explicit ValueMatchesMono(const ::testing::Matcher<T> &matcher)
      : matcher(matcher) {}

  bool
  MatchAndExplain(const ::llvm::detail::ExpectedHolder<T> &holder,
                  ::testing::MatchResultListener *listener) const override {
    if (!holder.Success())
      return false;

    return matcher.MatchAndExplain(*holder.Exp, listener);
  }

  void DescribeTo(std::ostream *os) const override {
    *os << "succeeded with value (";
    matcher.DescribeTo(os);
    *os << ")";
  }

  void DescribeNegationTo(std::ostream *os) const override {
    *os << "did not succeed or value (";
    matcher.DescribeNegationTo(os);
    *os << ")";
  }

private:
  ::testing::Matcher<T> matcher;
};

template <typename M> class ValueMatchesPoly {
public:
  explicit ValueMatchesPoly(const M &matcher) : matcher(matcher) {}

  template <typename T>
  operator ::testing::Matcher<const ::llvm::detail::ExpectedHolder<T> &>()
      const {
    return MakeMatcher(
        new ValueMatchesMono<T>(::testing::SafeMatcherCast<T>(matcher)));
  }

private:
  M matcher;
};

template <typename M> auto TheValue(M matcher) {
  return ValueMatchesPoly<M>(matcher);
}

} // namespace genpybind
