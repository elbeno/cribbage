#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <tuple>
#include <utility>
#include <vector>

using namespace std;

#define TESTINATOR_MAIN
#include <testinator.h>

//------------------------------------------------------------------------------

enum class Suit : uint8_t
{
  Spades,
  Hearts,
  Diamonds,
  Clubs
};

// When making a deck, it's going to be convenient to increment suits and wrap
// around
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

// Individual card symbols
const char* card_symbols =
  "🂡🂢🂣🂤🂥🂦🂧🂨🂩🂪🂫🂭🂮"
  "🂱🂲🂳🂴🂵🂶🂷🂸🂹🂺🂻🂽🂾"
  "🃁🃂🃃🃄🃅🃆🃇🃈🃉🃊🃋🃍🃎"
  "🃑🃒🃓🃔🃕🃖🃗🃘🃙🃚🃛🃝🃞";

//------------------------------------------------------------------------------

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
  uint8_t value = 1;
  Suit suit = Suit::Spades;
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

// Outputting a vector of cards is just outputting each card separated by a
// space
ostream& operator<<(ostream& s, const vector<Card>& v)
{
  auto it = v.cbegin();
  if (it != v.cend()) {
    cout << *it;
    for (++it; it != v.cend(); ++it)
    {
      s << ' ' << *it;
    }
  }
  return s;
}

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

// Dealing off the bottom :)
Hand deal_hand(Deck& deck)
{
  Hand h;
  auto it = deck.cend() - 4;
  copy(it, deck.cend(), back_inserter(h));
  deck.erase(it, deck.cend());
  return h;
}

//------------------------------------------------------------------------------

enum class ScoreType : uint8_t
{
  Fifteen,
  Pair,
  Run,
  Flush,
  Nob
};

// A score is a type and a vector of cards that make that score. The integral
// value of the score can be inferred from the type and if necessary the size of
// the vector.
struct Score
{
  ScoreType type;
  vector<Card> cards;
};

int print_score(ostream& os, const vector<Score>& scores)
{
  int total = 0;
  for (const auto& s : scores)
  {
    switch (s.type)
    {
      case ScoreType::Fifteen:
        total += 2;
        os << "Fifteen " << total << ": " << s.cards << '\n';
        break;
      case ScoreType::Pair:
      {
        switch (s.cards.size())
        {
          case 2:
            total += 2;
            os << "2 for a pair (" << s.cards << "), " << total << '\n';
            break;
          case 3:
            total += 6;
            os << "6 for threes (" << s.cards << "), " << total << '\n';
            break;
          case 4:
            total += 12;
            os << "12 for fours (" << s.cards << "), " << total << '\n';
            break;

          default: break;
        }
        break;
      }
      case ScoreType::Run:
        total += s.cards.size();
        os << s.cards.size() << " for a run (" << s.cards << "), " << total << '\n';
        break;
      case ScoreType::Flush:
        total += s.cards.size();
        os << s.cards.size() << " for a flush (" << s.cards << "), " << total << '\n';
        break;
      case ScoreType::Nob:
        ++total;
        os << "1 for his nob (" << s.cards << "), " << total << '\n';
        break;

      default: break;
    }
  }
  os << "Total " << total << '\n';
  return total;
}

//------------------------------------------------------------------------------

vector<Score> score_ns(vector<Card>::const_iterator first,
                       vector<Card>::const_iterator last,
                       int n,
                       vector<Card>& ns)
{
  if (first == last || n < 0) return {};

  vector<Score> result;

  int this_val = play_value(*first);
  if (this_val == n) {
    // if this card makes 15, this is one solution
    result.push_back({ScoreType::Fifteen, ns});
    result.back().cards.push_back(*first);
  } else {
    // other solutions that involve this card
    ns.push_back(*first);
    vector<Score> solns = score_ns(first+1, last, n - this_val, ns);
    ns.pop_back();
    move(solns.begin(), solns.end(), back_inserter(result));
  }

  // solutions that don't involve this card
  vector<Score> solns = score_ns(first+1, last, n, ns);
  move(solns.begin(), solns.end(), back_inserter(result));

  return result;
}

vector<Score> fifteens_score(const Hand& h)
{
  vector<Card> v;
  return score_ns(h.cbegin(), h.cend(), 15, v);
}

//------------------------------------------------------------------------------

vector<Score> pairs_score(const Hand& h)
{
  vector<Score> result;
  auto cur = h.cbegin();
  for (auto it = h.cbegin() + 1; cur != h.cend(); ++it)
  {
    if (it == h.cend() || cur->value != it->value) {
      // a 'pair' is any set of >=2 cards
      if (it - cur >= 2) {
        result.push_back({ScoreType::Pair, vector<Card>{cur, it}});
      }
      cur = it;
    }
  }
  return result;
}

//------------------------------------------------------------------------------

void runs_score(vector<Card>::const_iterator first,
                vector<Card>::const_iterator last,
                vector<Card>& run,
                vector<vector<Card>>& scoring_runs)
{
  // detect a run
  if (first == last) {
    if (run.size() < 3) return;
    auto it = adjacent_find(run.cbegin(), run.cend(),
                            [] (const Card& x, const Card& y) {
                              return y.value - x.value != 1;
                            });
    if (it == run.cend()) {
      scoring_runs.push_back(run);
    }
    return;
  }

  // runs including this card
  run.push_back(*first);
  runs_score(first+1, last, run, scoring_runs);
  run.pop_back();

  // runs not including this card
  runs_score(first+1, last, run, scoring_runs);
}

vector<Score> runs_score(const Hand& h)
{
  vector<Card> run;
  vector<vector<Card>> scoring_runs;
  runs_score(h.cbegin(), h.cend(), run, scoring_runs);

  // runs may contain other runs (a run of four contains two runs of three for
  // instance) - so we just count the runs of the greatest size
  sort(scoring_runs.begin(), scoring_runs.end(),
       [] (const vector<Card>& x, const vector<Card>& y) {
         return x.size() >= y.size();
       });

  vector<Score> result;
  if (!scoring_runs.empty()) {
    auto it = find_if(scoring_runs.cbegin(), scoring_runs.cend(),
                      [&] (const vector<Card>& v) {
                        return v.size() != scoring_runs[0].size(); });
    for (auto i = scoring_runs.cbegin(); i != it; ++i)
    {
      result.push_back({ScoreType::Run, *i});
    }
  }
  return result;
}

//------------------------------------------------------------------------------

vector<Score> flush_score(const Hand& h, const Card& starter)
{
  // a flush may be all the hand cards, or the hand cards and the starter
  auto suit = h[0].suit;
  if (all_of(h.cbegin()+1, h.cend(),
             [&] (const Card& c) { return c.suit == suit; }))
  {
    vector<Score> result{1, {ScoreType::Flush, h}};
    if (starter.suit == suit) {
      result.back().cards.push_back(starter);
    }
    return result;
  }
  return {};
}

//------------------------------------------------------------------------------

vector<Score> nob_score(const Hand& h, const Card& starter)
{
  const uint8_t Jack = 11;
  Card c{Jack, starter.suit};
  auto it = find(h.cbegin(), h.cend(), c);
  if (it != h.cend()) {
    return {{ScoreType::Nob, vector<Card>{1, c}}};
  }
  return {};
}

//------------------------------------------------------------------------------

int compute_score(const Hand& h, const Card& starter)
{
  Hand h5(h);
  h5.push_back(starter);
  cout << h5 << endl;

  auto scores = fifteens_score(h5);

  stable_sort(h5.begin(), h5.end());

  auto pairs = pairs_score(h5);
  move(pairs.begin(), pairs.end(), back_inserter(scores));

  auto runs = runs_score(h5);
  move(runs.begin(), runs.end(), back_inserter(scores));

  auto flushes = flush_score(h, starter);
  move(flushes.begin(), flushes.end(), back_inserter(scores));

  auto nob = nob_score(h, starter);
  move(nob.begin(), nob.end(), back_inserter(scores));

  return print_score(cout, scores);
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
