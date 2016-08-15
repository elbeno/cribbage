#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <vector>

using namespace std;

enum class Suit : uint8_t {
  Spades,
  Hearts,
  Diamonds,
  Clubs
};

const char* card_symbols =
  "🂡🂢🂣🂤🂥🂦🂧🂨🂩🂪🂫🂭🂮"
  "🂱🂲🂳🂴🂵🂶🂷🂸🂹🂺🂻🂽🂾"
  "🃁🃂🃃🃄🃅🃆🃇🃈🃉🃊🃋🃍🃎"
  "🃑🃒🃓🃔🃕🃖🃗🃘🃙🃚🃛🃝🃞";

const char* suit_symbols[] =
{
  "♣",
  "♦",
  "♥",
  "♠"
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

Suit& operator++(Suit& s)
{
  switch (s)
  {
    case Suit::Spades: s = Suit::Hearts; break;
    case Suit::Hearts: s = Suit::Diamonds; break;
    case Suit::Diamonds: s = Suit::Clubs; break;
    case Suit::Clubs: s = Suit::Spades; break;
  }
  return s;
}

struct Card {
  uint8_t value;
  Suit suit;
};

using Deck = vector<Card>;

ostream& operator<<(ostream& s, const Card& c)
{
  s << value_symbols[c.value]
    << suit_symbols[static_cast<uint8_t>(c.suit)];
  return s;
}

ostream& operator<<(ostream& s, const Deck& d)
{
  for (const auto& c : d)
  {
    s << c << endl;
  }
  return s;
}

Deck make_deck()
{
  const uint8_t Ace = 1;
  const uint8_t King = 13;

  Suit s = Suit::Clubs;
  uint8_t v = 1;

  vector<Card> d;
  generate_n(back_inserter(d), 52,
             [&] () mutable {
               Card c { v, s };
               ++v; if (v > King) v = Ace;
               ++s;
               return c;
             });
  return d;
}

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

void shuffle_deck(vector<Card>& deck)
{
  static RNG rng;
  shuffle(deck.begin(), deck.end(), rng.gen);
}

int main(void)
{
  auto deck = make_deck();
  shuffle_deck(deck);
  cout << deck;
  for (int i = 0; i < 52; ++i)
  {
    string s(&card_symbols[i*4], &card_symbols[i*4+4]);
    cout << s << ' ';
  }
}

