#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <tuple>
#include <type_traits>
#include <vector>

using namespace std;

#define TESTINATOR_MAIN
#include <testinator.h>

//------------------------------------------------------------------------------

enum class Suit : uint8_t {
  Spades,
  Hearts,
  Diamonds,
  Clubs
};

Suit& operator++(Suit& s)
{
  switch (s)
  {
    case Suit::Spades: s = Suit::Hearts; break;
    case Suit::Hearts: s = Suit::Diamonds; break;
    case Suit::Diamonds: s = Suit::Clubs; break;
    case Suit::Clubs: s = Suit::Spades; break;
    default: break;
  }
  return s;
}

//------------------------------------------------------------------------------

const char* card_symbols =
  "🂡🂢🂣🂤🂥🂦🂧🂨🂩🂪🂫🂭🂮"
  "🂱🂲🂳🂴🂵🂶🂷🂸🂹🂺🂻🂽🂾"
  "🃁🃂🃃🃄🃅🃆🃇🃈🃉🃊🃋🃍🃎"
  "🃑🃒🃓🃔🃕🃖🃗🃘🃙🃚🃛🃝🃞";

const char* suit_symbols[] =
{
  "♠",
  "♥",
  "♦",
  "♣"
};

const char* value_symbols[] =
{
  "",
  "A",
  "2",
  "3",
  "4",
  "5",
  "6",
  "7",
  "8",
  "9",
  "10",
  "J",
  "Q",
  "K"
};

//------------------------------------------------------------------------------

struct Card {
  uint8_t value;
  Suit suit;
};

bool operator==(const Card& x, const Card& y)
{
  return x.value == y.value && x.suit == y.suit;
}

bool operator<(const Card& x, const Card& y)
{
  auto as_tuple = [] (const Card& c) {
    return tie(c.value, c.suit);
  };
  return as_tuple(x) < as_tuple(y);
}

uint8_t play_value(const Card& c)
{
  return c.value >= 10 ? 10 : c.value;
}

ostream& operator<<(ostream& s, const Card& c)
{
  s << value_symbols[c.value]
    << suit_symbols[static_cast<uint8_t>(c.suit)];
  return s;
}

using Hand = vector<Card>;

//------------------------------------------------------------------------------

struct RNG
{
  RNG() {
    array<int, mt19937::state_size> seed_data;
    random_device r;
    generate_n(seed_data.data(), seed_data.size(), ref(r));
    seed_seq seq(begin(seed_data), end(seed_data));
    gen.seed(seq);
  }
  mt19937 gen;
};

//------------------------------------------------------------------------------

using Deck = vector<Card>;

ostream& operator<<(ostream& s, const Deck& d)
{
  auto it = d.cbegin();
  if (it != d.cend()) {
    cout << *it;
    for (++it; it != d.cend(); ++it)
    {
      s << ' ' << *it;
    }
  }
  return s;
}

Deck make_deck()
{
  const uint8_t Ace = 1;
  const uint8_t King = 13;
  const size_t decksize = 52;

  Suit s = Suit::Clubs;
  uint8_t v = 1;

  vector<Card> d;
  d.reserve(decksize);
  generate_n(back_inserter(d), decksize,
             [&] () mutable {
               Card c { v, s };
               ++v; if (v > King) v = Ace;
               ++s;
               return c;
             });
  return d;
}

void shuffle_deck(Deck& deck)
{
  static RNG rng;
  shuffle(deck.begin(), deck.end(), rng.gen);
}

Hand deal_hand(Deck& deck)
{
  Hand h;
  auto it = deck.cend() - 4;
  copy(it, deck.cend(), back_inserter(h));
  deck.erase(it, deck.cend());
  return h;
}

//------------------------------------------------------------------------------

void score_ns(vector<Card>::const_iterator first,
              vector<Card>::const_iterator last,
              int n,
              vector<Card>& ns,
              int& score)
{
  if (first == last || n < 0) return;

  int this_val = play_value(*first);
  if (this_val == n) {
    score += 2;
    cout << "Fifteen " << score << ": " << ns << ' ' << *first << endl;
  }

  ns.push_back(*first);
  score_ns(first+1, last, n - this_val, ns, score);
  ns.pop_back();

  score_ns(first+1, last, n, ns, score);
}

int fifteens_score(const Hand& h)
{
  vector<Card> v;
  int score = 0;
  score_ns(h.cbegin(), h.cend(), 15, v, score);
  return score;
}

//------------------------------------------------------------------------------

void pairs_score(const Hand& h, int& score)
{
  auto cur = h.cbegin();
  for (auto it = h.cbegin() + 1; cur != h.cend(); ++it)
  {
    if (it == h.cend() || cur->value != it->value) {
      switch (it - cur) {
        case 2:
          score += 2;
          cout << "2 for a pair " << cur[0]
               << ' ' << cur[1]
               << ", " << score << endl;
          break;
        case 3:
          score += 6;
          cout << "6 for threes " << cur[0]
               << ' ' << cur[1]
               << ' ' << cur[2]
               << ", " << score << endl;
          break;
        case 4:
          score += 12;
          cout << "12 for fours " << cur[0]
               << ' ' << cur[1]
               << ' ' << cur[2]
               << ' ' << cur[3]
               << ", " << score << endl;
          break;
        default:
          break;
      }
      cur = it;
    }
  }
}

//------------------------------------------------------------------------------

void runs_score(vector<Card>::const_iterator first,
                vector<Card>::const_iterator last,
                vector<Card>& run,
                vector<vector<Card>>& scoring_runs)
{
  auto s = run.size();
  if (first == last) {
    if (s < 3) return;
    auto val = run.cbegin()->value;
    for (auto it = run.cbegin()+1; it != run.cend(); ++it)
    {
      if (it->value - val != 1) return;
      val = it->value;
    }
    scoring_runs.push_back(run);
    return;
  }

  runs_score(first+1, last, run, scoring_runs);
  run.push_back(*first);
  runs_score(first+1, last, run, scoring_runs);
  run.pop_back();
}

void runs_score(const Hand& h, int& score)
{
  vector<Card> run;
  vector<vector<Card>> scoring_runs;
  runs_score(h.cbegin(), h.cend(), run, scoring_runs);

  sort(scoring_runs.begin(), scoring_runs.end(),
       [] (const vector<Card>& x, const vector<Card>& y) {
         return x.size() >= y.size();
       });
  if (!scoring_runs.empty()) {
    auto it = find_if(scoring_runs.cbegin(), scoring_runs.cend(),
                      [&] (const vector<Card>& v) {
                        return v.size() != scoring_runs[0].size(); });
    for (auto i = scoring_runs.cbegin(); i != it; ++i) {
      score += i->size();
      cout << i->size() << " for the run " << *i << ", " << score << endl;
    }
  }
}

//------------------------------------------------------------------------------

void flush_score(const Hand& h, const Card& starter, int& score)
{
  auto suit = h[0].suit;
  if (all_of(h.cbegin()+1, h.cend(),
             [&] (const Card& c) { return c.suit == suit; }))
  {
    score += 4;
    if (starter.suit == suit) {
      ++score;
      cout << "5 for the flush " << h << ' ' << starter << ", " << score << endl;
    } else {
      cout << "4 for the flush " << h << ", " << score << endl;
    }
  }
}

//------------------------------------------------------------------------------

void nob_score(const Hand& h, const Card& starter, int& score)
{
  const uint8_t Jack = 11;
  Card c{Jack, starter.suit};
  auto it = find(h.cbegin(), h.cend(), c);
  if (it != h.cend()) {
    ++score;
    cout << "One for his nob " << *it << ", " << score << endl;
  }
}

//------------------------------------------------------------------------------

int compute_score(const Hand& h, const Card& starter)
{
  Hand h5(h);
  h5.push_back(starter);
  cout << h5 << endl;

  auto s = fifteens_score(h5);

  sort(h5.begin(), h5.end());
  pairs_score(h5, s);
  runs_score(h5, s);

  flush_score(h, starter, s);
  nob_score(h, starter, s);
  return s;
}

//------------------------------------------------------------------------------

DEF_TEST(Zero, Hands)
{
  Hand h = { {2, Suit::Spades},
             {4, Suit::Spades},
             {6, Suit::Spades},
             {8, Suit::Hearts} };
  Card starter = { 10, Suit::Diamonds };
  return compute_score(h, starter) == 0;
}

DEF_TEST(OnePair, Pairs)
{
  Hand h = { {2, Suit::Spades},
             {2, Suit::Hearts},
             {6, Suit::Spades},
             {8, Suit::Hearts} };
  Card starter = { 10, Suit::Diamonds };
  return compute_score(h, starter) == 2;
}

DEF_TEST(TwoPairs, Pairs)
{
  Hand h = { {2, Suit::Spades},
             {2, Suit::Hearts},
             {6, Suit::Spades},
             {10, Suit::Hearts} };
  Card starter = { 10, Suit::Diamonds };
  return compute_score(h, starter) == 4;
}

DEF_TEST(Threes, Pairs)
{
  Hand h = { {2, Suit::Spades},
             {2, Suit::Hearts},
             {2, Suit::Diamonds},
             {8, Suit::Hearts} };
  Card starter = { 10, Suit::Diamonds };
  return compute_score(h, starter) == 6;
}

DEF_TEST(ThreesAndTwo, Pairs)
{
  Hand h = { {2, Suit::Spades},
             {2, Suit::Hearts},
             {2, Suit::Diamonds},
             {10, Suit::Hearts} };
  Card starter = { 10, Suit::Diamonds };
  return compute_score(h, starter) == 8;
}

DEF_TEST(Fours, Pairs)
{
  Hand h = { {2, Suit::Spades},
             {2, Suit::Hearts},
             {6, Suit::Diamonds},
             {2, Suit::Clubs} };
  Card starter = { 2, Suit::Diamonds };
  return compute_score(h, starter) == 12;
}

DEF_TEST(NoFlush, Flushes)
{
  Hand h = { {2, Suit::Spades},
             {4, Suit::Spades},
             {6, Suit::Spades},
             {8, Suit::Clubs} };
  Card starter = { 10, Suit::Spades };
  return compute_score(h, starter) == 0;
}

DEF_TEST(Four, Flushes)
{
  Hand h = { {2, Suit::Spades},
             {4, Suit::Spades},
             {6, Suit::Spades},
             {8, Suit::Spades} };
  Card starter = { 10, Suit::Diamonds };
  return compute_score(h, starter) == 4;
}

DEF_TEST(Five, Flushes)
{
  Hand h = { {2, Suit::Spades},
             {4, Suit::Spades},
             {6, Suit::Spades},
             {8, Suit::Spades} };
  Card starter = { 10, Suit::Spades };
  return compute_score(h, starter) == 5;
}

DEF_TEST(Nob, Nob)
{
  Hand h = { {2, Suit::Hearts},
             {4, Suit::Spades},
             {6, Suit::Spades},
             {11, Suit::Spades} };
  Card starter = { 10, Suit::Spades };
  return compute_score(h, starter) == 1;
}

DEF_TEST(NoNob, Nob)
{
  Hand h = { {2, Suit::Hearts},
             {4, Suit::Spades},
             {6, Suit::Spades},
             {10, Suit::Spades} };
  Card starter = { 11, Suit::Spades };
  return compute_score(h, starter) == 0;
}

DEF_PROPERTY(Nineteen, Hands, mt19937::result_type seed)
{
  mt19937 gen(seed);
  auto d = make_deck();
  shuffle(d.begin(), d.end(), gen);
  Hand h = deal_hand(d);
  auto s = compute_score(h, d[0]);
  return s != 19 && s != 25 && s != 26 && s != 27;
}

DEF_TEST(Run3, Runs)
{
  Hand h = { {10, Suit::Hearts},
             {11, Suit::Clubs},
             {12, Suit::Diamonds},
             {1, Suit::Spades} };
  Card starter = { 2, Suit::Spades };
  return compute_score(h, starter) == 3;
}

DEF_TEST(Run4, Runs)
{
  Hand h = { {10, Suit::Hearts},
             {11, Suit::Clubs},
             {12, Suit::Diamonds},
             {13, Suit::Spades} };
  Card starter = { 2, Suit::Spades };
  return compute_score(h, starter) == 4;
}

DEF_TEST(DoubleRun, Runs)
{
  Hand h = { {10, Suit::Hearts},
             {11, Suit::Clubs},
             {11, Suit::Diamonds},
             {12, Suit::Spades} };
  Card starter = { 2, Suit::Spades };
  return compute_score(h, starter) == 8;
}

DEF_TEST(Run5, Runs)
{
  Hand h = { {10, Suit::Hearts},
             {11, Suit::Clubs},
             {12, Suit::Diamonds},
             {13, Suit::Spades} };
  Card starter = { 9, Suit::Spades };
  return compute_score(h, starter) == 5;
}

DEF_TEST(Max, Hands)
{
  Hand h = { {5, Suit::Hearts},
             {5, Suit::Clubs},
             {5, Suit::Diamonds},
             {11, Suit::Spades} };
  Card starter = { 5, Suit::Spades };
  return compute_score(h, starter) == 29;
}

DEF_TEST(PairAndRun, Hands)
{
  Hand h = { {1, Suit::Hearts},
             {1, Suit::Clubs},
             {9, Suit::Diamonds},
             {10, Suit::Spades} };
  Card starter = { 11, Suit::Spades };
  return compute_score(h, starter) == 5;
}
