#include <algorithm>
#include <iostream>
#include <vector>

struct Seq {
  std::vector<long long> v;
  bool closed = false;
  bool has_diff = false;
  long long diff = 0;
};

bool prefix_ok(Seq &s) {
  if (s.v.size() < 2) {
    return true;
  }

  s.has_diff = true;
  s.diff = s.v[1] - s.v[0];

  for (size_t i = 2; i < s.v.size(); ++i) {
    if (s.v[i] - s.v[i - 1] != s.diff) {
      return false;
    }
  }

  return true;
}

void normalize(Seq &s, long long x) {
  if (s.closed || !s.has_diff) {
    return;
  }

  long long expected = s.v.back() + s.diff;

  if (expected < x) {
    s.closed = true;
  }
}

bool can_take(const Seq &s, long long x) {
  if (s.closed) {
    return false;
  }

  if (s.v.empty()) {
    return true;
  }

  if (!s.has_diff) {
    return x >= s.v.back();
  }

  return s.v.back() + s.diff == x;
}

void add_value(Seq &s, long long x) {
  if (s.v.empty()) {
    s.v.push_back(x);
    return;
  }

  if (!s.has_diff) {
    s.diff = x - s.v.back();
    s.has_diff = true;
  }

  s.v.push_back(x);
}

bool finish(int pos, const std::vector<long long> &a, Seq first, Seq second,
            Seq &ans_first, Seq &ans_second) {
  const int n = static_cast<int>(a.size());

  while (pos < n) {
    long long x = a[pos];

    normalize(first, x);
    normalize(second, x);

    bool can_first = can_take(first, x);
    bool can_second = can_take(second, x);

    if (!can_first && !can_second) {
      return false;
    }

    if (can_first && !can_second) {
      add_value(first, x);
      ++pos;
      continue;
    }

    if (!can_first && can_second) {
      add_value(second, x);
      ++pos;
      continue;
    }

    {
      Seq nf = first;
      Seq ns = second;
      add_value(nf, x);

      if (finish(pos + 1, a, nf, ns, ans_first, ans_second)) {
        return true;
      }
    }

    {
      Seq nf = first;
      Seq ns = second;
      add_value(ns, x);

      if (finish(pos + 1, a, nf, ns, ans_first, ans_second)) {
        return true;
      }
    }

    return false;
  }

  if (first.v.empty()) {
    if (second.v.size() < 2) {
      return false;
    }

    first.v.push_back(second.v.back());
    second.v.pop_back();
  }

  if (second.v.empty()) {
    if (first.v.size() < 2) {
      return false;
    }

    second.v.push_back(first.v.back());
    first.v.pop_back();
  }

  ans_first = first;
  ans_second = second;

  return true;
}

int main() {
  int n;
  std::cin >> n;

  std::vector<long long> t(n);

  for (int i = 0; i < n; ++i) {
    std::cin >> t[i];
  }

  std::sort(t.begin(), t.end());

  int pref = std::min(n, 6);

  for (int mask = 0; mask < (1 << pref); ++mask) {
    Seq first;
    Seq second;

    for (int i = 0; i < pref; ++i) {
      if (mask & (1 << i)) {
        first.v.push_back(t[i]);
      } else {
        second.v.push_back(t[i]);
      }
    }

    if (!prefix_ok(first) || !prefix_ok(second)) {
      continue;
    }

    Seq ans_first;
    Seq ans_second;

    if (finish(pref, t, first, second, ans_first, ans_second)) {
      std::cout << ans_first.v.size() << '\n';

      for (long long x : ans_first.v) {
        std::cout << x << ' ';
      }

      std::cout << '\n';

      std::cout << ans_second.v.size() << '\n';

      for (int i = static_cast<int>(ans_second.v.size()) - 1; i >= 0; --i) {
        std::cout << ans_second.v[i] << ' ';
      }

      std::cout << '\n';

      return 0;
    }
  }

  std::cout << -1 << '\n';

  return 0;
}