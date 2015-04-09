// Copyright (C) 2011  Mario Lang <mlang@delysid.org>
//
// Distributed under the terms of the GNU General Public License version 3.
// (see accompanying file LICENSE.txt or copy at
//  http://www.gnu.org/licenses/gpl-3.0-standalone.html)

#ifndef BMC_MUSIC_HPP
#define BMC_MUSIC_HPP

#include <cstdint>
#include <boost/config.hpp>
#include "bmc/math.hpp"

namespace bmc {

typedef boost::rational<std::int32_t> rational;

rational const zero = rational();

inline rational augmentation_dots_factor(unsigned dots)
{ return dots ? rational{(1 << (dots + 1)) - 1, 1 << dots} : rational{1}; }

inline rational tuplet_number_to_ratio(unsigned number)
{
  BOOST_ASSERT(number > 0);
  rational factor{1};
  if (number > 2)
    factor.assign(number - 1, number);
  else if (number == 2) // duplet
    factor.assign(number + 1, number);
  return factor;
}

/**
 * \brief Represents a time signature with numerator and denominator.
 *
 * Arithmetic with time signatures is useful, so derive from rational.
 * However, care has to be taken to avoid the implicit simplification of
 * <code>boost::rational</code>.
 */
class time_signature: public rational {
  /**
   * \brief <code>boost::rational</code> simplifies during construction, so we
   *        need to preserve the original numerator and denominator.
   */
  int_type d_num, d_den;
public:
  time_signature(int_type n = 4, int_type d = 4)
  : rational(n, d), d_num(n), d_den(d) {}
  using rational::operator==; bool operator==(time_signature const&) const;
  int_type numerator() const { return d_num; }
  int_type denominator() const { return d_den; }

  friend
  std::ostream &operator<< ( std::ostream &os
                           , time_signature const &signature
                           )
  { return os << signature.numerator() << '/' << signature.denominator(); }
};

enum diatonic_step { C = 0, D, E, F, G, A, B };
unsigned BOOST_CONSTEXPR_OR_CONST steps_per_octave = B + 1;
enum accidental {
  natural, flat, double_flat, triple_flat,
           sharp, double_sharp, triple_sharp
};

typedef signed short key_signature;

enum articulation {
  appoggiatura, short_appoggiatura,
  short_trill, extended_short_trill,
  turn_between_notes, turn_above_or_below_note,
  inverted_turn_between_notes, inverted_turn_above_or_below_note,
  staccato, staccatissimo, mezzo_staccato,
  agogic_accent, accent,
  mordent, extended_mordent,
  arpeggio_up, arpeggio_up_multi_staff,
  arpeggio_down, arpeggio_down_multi_staff
};

enum interval {
  unison = 0,
  second = 1,
  third = 2,
  fourth = 3,
  fifth = 4,
  sixth = 5,
  seventh = 6,
  octave = 7
};

}

#endif
