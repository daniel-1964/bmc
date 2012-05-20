// Copyright (C) 2012  Mario Lang <mlang@delysid.org>
//
// Distributed under the terms of the GNU General Public License version 3.
// (see accompanying file LICENSE.txt or copy at
//  http://www.gnu.org/licenses/gpl-3.0-standalone.html)

#ifndef BMC_LILYPOND_HPP_INCLUDED
#define BMC_LILYPOND_HPP_INCLUDED

#include "ambiguous.hpp"
#include "output_format.hpp"

namespace music { namespace lilypond {

/**
 * \brief Generate LilyPond input from (compiled) braille music.
 */
class generator: public boost::static_visitor<void>
{
  std::ostream& os;
  bool const layout, midi, include_locations;
public:
  /**
   * \brief Construct a LilyPond source code generator.
   *
   * \param stream The character stream used to output LilyPond sourcec code to.
   * \param layout Indicate if visual notation should be generated by LilyPond.
   * \param midi   Indicate if MIDI performance data should be generated by LilyPond
   */
  generator( std::ostream& stream
           , bool layout = true
           , bool midi = true
           , bool include_locations = false);

  /**
   * \brief Generate LilyPond source code for the given braille score.
   */
  void operator() (braille::ambiguous::score const& score) const;

private:
  void operator() ( braille::ambiguous::part const& part
                  , braille::ambiguous::score const& score
                  ) const;

public:
  result_type operator() (braille::ambiguous::measure const&) const;
  result_type operator() (braille::ambiguous::key_and_time_signature const&) const;
private:
  void operator() (braille::ambiguous::voice const&) const;
  void operator() (braille::ambiguous::partial_measure const&) const;
  void operator() (braille::ambiguous::partial_voice const&) const;
public:
  /** @name Operators for visiting music::braille::sign objects */
  /** @{ */
  result_type operator() (braille::ambiguous::barline const&) const;
  result_type operator() (braille::ambiguous::simile const&) const;
  result_type operator() (braille::ambiguous::value_distinction const&) const;
  result_type operator() (braille::hand_sign const&) const;
  result_type operator() (braille::ambiguous::rest const&) const;
  result_type operator() (braille::ambiguous::note const&) const;
  result_type operator() (braille::ambiguous::chord const&) const;
  /** @} */

private: // utilities
  void ly_accidental(int) const;
  void ly_clef(std::string const&) const;
  void ly_finger(braille::fingering_list const&) const;
  void ly_key(key_signature const&) const;
  void ly_rhythm(braille::ambiguous::rhythmic_data const&) const;
  void ly_octave(int) const;
  void ly_partial(rational const&) const;
  void ly_pitch_step(diatonic_step) const;
};

}}

inline std::ostream&
operator <<
( std::ostream& stream
, music::braille::ambiguous::score const& score
) {
  switch (music::get_output_format(stream)) {
  case music::output_format::lilypond: {
    bool const locations = music::get_lilypond_flags(stream) == music::lilypond_flags::include_locations;
    music::lilypond::generator generate(stream, true, true, locations);
    generate(score);
    break;
  }
  default:
    BOOST_ASSERT(false);
  }
  return stream;
}

#endif
